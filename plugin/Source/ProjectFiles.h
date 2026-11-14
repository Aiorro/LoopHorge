#pragma once
#include "engine/ProjectJson.h"
namespace projectFiles {
struct ReadResult {std::string text;juce::String error;};
// Message/control thread only. Read at most32MiB+1, even if a file grows after stat.
inline ReadResult read(const juce::File& file){
 try{
  juce::FileInputStream input(file);if(!input.openedOk())return {{},"Cannot open project file"};
  const auto size=input.getTotalLength();if(size<=0||size>juce::int64(composer::jsonDetail::maximumBytes))return {{},"Project file must be between 1 byte and 32 MiB"};
  std::string text(size_t(size),'\0');size_t offset=0;
  while(offset<text.size()){const int n=input.read(text.data()+offset,int(text.size()-offset));if(n<=0)return {{},"Project file changed or could not be read"};offset+=size_t(n);}
  char extra;if(input.read(&extra,1)!=0)return {{},"Project file changed or exceeds its declared size"};
  return {std::move(text),{}};
 }catch(const std::bad_alloc&){return {{},"Insufficient memory to open project"};}
}
inline juce::String write(const juce::File& file,std::string_view text){
 if(text.empty()||text.size()>composer::jsonDetail::maximumBytes)return "Project exceeds 32 MiB or is empty";
 try{
  auto project=composer::decodeProjectJson(text);
  if(!project.project){auto legacy=wire::document(text);Schedule checked;if(!wire::integer(legacy["version"],1,1)||!wire::schedule(legacy["idea"],checked))return "Cannot save an invalid project";}
  return file.replaceWithData(text.data(),text.size())?juce::String{}:juce::String("Cannot save project file");
 }catch(const std::bad_alloc&){return "Insufficient memory to save project";}
}
}
