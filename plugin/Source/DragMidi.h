#pragma once
#include "MidiExport.h"
#include <filesystem>
#include <mutex>
#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#else
 #include <fcntl.h>
 #include <sys/file.h>
 #include <unistd.h>
 #include <cerrno>
#endif
// A read lease may be released by a native completion thread, independently of the editor.
class DragMidiLease {
public:
 const juce::File file;
 explicit DragMidiLease(juce::File path):file(std::move(path)){
#if JUCE_WINDOWS
  handle=CreateFileW(file.getFullPathName().toWideCharPointer(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
#else
  handle=::open(file.getFullPathName().toRawUTF8(),O_RDONLY|O_NOFOLLOW);if(handle>=0&&::flock(handle,LOCK_SH|LOCK_NB)!=0){::close(handle);handle=-1;}
#endif
 }
 ~DragMidiLease(){
#if JUCE_WINDOWS
  if(valid())CloseHandle(handle);
#else
  if(valid())::close(handle);
#endif
 }
 DragMidiLease(const DragMidiLease&)=delete;DragMidiLease& operator=(const DragMidiLease&)=delete;
 bool valid()const{
#if JUCE_WINDOWS
  return handle!=INVALID_HANDLE_VALUE;
#else
  return handle>=0;
#endif
 }
 static bool removeExpired(const juce::File& file){
#if JUCE_WINDOWS
  return file.deleteFile(); // Active read handles deny deletion, including across processes.
#else
  int fd=::open(file.getFullPathName().toRawUTF8(),O_RDONLY|O_NOFOLLOW);if(fd<0)return false;bool removed=false;if(::flock(fd,LOCK_EX|LOCK_NB)==0)removed=::unlink(file.getFullPathName().toRawUTF8())==0;::close(fd);return removed;
#endif
 }
private:
#if JUCE_WINDOWS
 HANDLE handle=INVALID_HANDLE_VALUE;
#else
 int handle=-1;
#endif
};
class DragMidiCache {
public:
 struct Result{std::shared_ptr<DragMidiLease> lease;juce::String error;};
 explicit DragMidiCache(juce::File directory):root(std::move(directory)){}
 static juce::File defaultRoot(){return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LoopForge/DragMIDI");}
 Result prepare(const juce::var& idea,juce::int64 now=juce::Time::currentTimeMillis())const{
  auto bytes=encodeMidi(idea);if(bytes.isEmpty()||bytes.getSize()>maxBytes)return {{},"Generate a valid idea before dragging"};
  std::unique_lock gate(prepareMutex,std::try_to_lock);if(!gate.owns_lock())return {{},"MIDI cache busy; try again or use Export MIDI"};
  juce::InterProcessLock lock("LoopForgeDrag-"+juce::String::toHexString(root.getFullPathName().hashCode64()));if(!lock.enter(0))return {{},"MIDI cache busy; try again or use Export MIDI"};
  struct Unlock{juce::InterProcessLock& value;~Unlock(){value.exit();}} unlock{lock};
  namespace fs=std::filesystem;std::error_code ec;auto utf8=root.getFullPathName().toStdString();auto path=fs::path(std::u8string(reinterpret_cast<const char8_t*>(utf8.data()),utf8.size()));
  for(auto part=path;!part.empty();part=part.parent_path()){if(fs::is_symlink(fs::symlink_status(part,ec)))return {{},"MIDI cache path must not be a symlink"};ec.clear();if(part==part.root_path())break;}
  fs::create_directories(path,ec);if(ec)return {{},"Cannot create MIDI cache; use Export MIDI"};
#if !JUCE_WINDOWS
  fs::permissions(path,fs::perms::owner_all,fs::perm_options::replace,ec);if(ec)return {{},"Cannot protect MIDI cache; use Export MIDI"};
#endif
  size_t count=0,scanned=0;uint64_t size=0;fs::directory_iterator it(path,ec),end;
  if(ec)return {{},"Cannot read MIDI cache; use Export MIDI"};
  for(;it!=end;it.increment(ec)){
   if(ec||++scanned>4096)return {{},"MIDI cache full; use Export MIDI"};auto entry=*it;auto status=entry.symlink_status(ec);if(ec)return {{},"Cannot inspect MIDI cache; use Export MIDI"};
   auto name=entry.path().filename().u8string();juce::File file=juce::File(juce::String::fromUTF8(reinterpret_cast<const char*>(entry.path().u8string().c_str())));
   if(fs::is_regular_file(status)&&recognized(name)&&file.getLastModificationTime().toMilliseconds()<now-retention&&DragMidiLease::removeExpired(file))continue;
   ++count;if(fs::is_regular_file(status)){auto n=entry.file_size(ec);if(ec||n>maxBytes||size>maxBytes-n)return {{},"MIDI cache full; use Export MIDI"};size+=n;}
  }
  if(ec||count>=256||size>maxBytes-bytes.getSize())return {{},"MIDI cache full; use Export MIDI"};
  auto file=root.getChildFile(juce::Uuid().toString()+".mid");if(!writeExclusive(file,bytes))return {{},"Cannot write MIDI cache; use Export MIDI"};
  auto lease=std::make_shared<DragMidiLease>(file);if(!lease->valid()){file.deleteFile();return {{},"Cannot retain MIDI drag file; use Export MIDI"};}return {std::move(lease),{}};
 }
private:
 inline static std::mutex prepareMutex;
 juce::File root;static constexpr uint64_t maxBytes=16*1024*1024;static constexpr juce::int64 retention=24*60*60*1000LL;
 static bool recognized(const std::u8string& name){if(name.size()!=36||name.substr(32)!=u8".mid")return false;for(size_t i=0;i<32;++i)if(!((name[i]>=u8'0'&&name[i]<=u8'9')||(name[i]>=u8'a'&&name[i]<=u8'f')))return false;return true;}
 static bool writeExclusive(const juce::File& file,const juce::MemoryBlock& bytes){
#if JUCE_WINDOWS
  auto h=CreateFileW(file.getFullPathName().toWideCharPointer(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(h==INVALID_HANDLE_VALUE)return false;DWORD written=0;bool ok=WriteFile(h,bytes.getData(),DWORD(bytes.getSize()),&written,nullptr)&&written==bytes.getSize();CloseHandle(h);
#else
  int h=::open(file.getFullPathName().toRawUTF8(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(h<0)return false;size_t offset=0;while(offset<bytes.getSize()){auto n=::write(h,static_cast<const char*>(bytes.getData())+offset,bytes.getSize()-offset);if(n<0&&errno==EINTR)continue;if(n<=0)break;offset+=size_t(n);}bool ok=offset==bytes.getSize();::close(h);
#endif
  if(!ok)file.deleteFile();return ok;
 }
};
