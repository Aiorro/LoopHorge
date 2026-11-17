#pragma once
#include "engine/ProjectJson.h"
namespace projectFiles {
struct ReadResult {std::string text;juce::String error;};
// Message/control thread only. Read at most32MiB+1, even if a file grows after stat.
inline ReadResult read(const juce::File& file,size_t maximum=composer::jsonDetail::maximumBytes){
 try{
  juce::FileInputStream input(file);if(!input.openedOk())return {{},"Cannot open project file"};
  maximum=std::min(maximum,composer::jsonDetail::maximumBytes);const auto size=input.getTotalLength();if(size<=0||size>juce::int64(maximum))return {{},"File exceeds requested size limit or is empty"};
  std::string text(size_t(size),'\0');size_t offset=0;
  while(offset<text.size()){const int n=input.read(text.data()+offset,int(text.size()-offset));if(n<=0)return {{},"Project file changed or could not be read"};offset+=size_t(n);}
  char extra;if(input.read(&extra,1)!=0)return {{},"Project file changed or exceeds its declared size"};
  return {std::move(text),{}};
 }catch(const std::bad_alloc&){return {{},"Insufficient memory to open project"};}
}
// Shared with legacy MIDI export: publish only a verified complete staging file.
inline juce::String replaceChecked(const juce::File& file,std::string_view bytes){
 if(bytes.empty()||bytes.size()>composer::jsonDetail::maximumBytes)return "File exceeds 32 MiB or is empty";
 juce::TemporaryFile temporary(file);
 {
  juce::FileOutputStream output(temporary.getFile());
  if(!output.openedOk())return "Cannot create temporary save file";
  const bool complete=output.write(bytes.data(),bytes.size());
  output.flush();
  if(!complete||output.getStatus().failed()||output.getPosition()!=juce::int64(bytes.size())||temporary.getFile().getSize()!=juce::int64(bytes.size()))return "Cannot write complete file; original preserved";
 }
 return temporary.overwriteTargetFileWithTemporary()?juce::String{}:juce::String("Cannot replace saved file");
}
inline juce::String write(const juce::File& file,std::string_view text){
 if(text.empty()||text.size()>composer::jsonDetail::maximumBytes)return "Project exceeds 32 MiB or is empty";
 try{
  auto project=composer::decodeProjectJson(text);
  if(!project.project){auto legacy=wire::document(text);Schedule checked;if(!wire::integer(legacy["version"],1,1)||!wire::schedule(legacy["idea"],checked))return "Cannot save an invalid project";}
  return replaceChecked(file,text);
 }catch(const std::bad_alloc&){return "Insufficient memory to save project";}
}
}
