#pragma once
#include "Validation.h"
#include <algorithm>
#include <limits>

namespace composer {
struct LoopEditResult {std::optional<Composition> composition;std::string error;};
namespace loopEditDetail {
inline const Track* track(const Composition& c,Id id){auto i=std::find_if(c.tracks.begin(),c.tracks.end(),[&](const auto& t){return t.id==id;});return i==c.tracks.end()?nullptr:&*i;}
inline bool note(const Track& t,const Note& n,Tick end){return n.pitch>=t.lowPitch&&n.pitch<=t.highPitch&&n.pitch>=0&&n.pitch<=127&&n.velocity>=1&&n.velocity<=127&&detail::span(n.start,n.duration,end);}
inline size_t count(const Composition& c){size_t total=0;for(const auto& t:c.tracks)total+=t.notes.size();return total;}
inline void sort(Track& t){std::sort(t.notes.begin(),t.notes.end(),[](const auto& a,const auto& b){if(a.start!=b.start)return a.start<b.start;if(a.pitch!=b.pitch)return a.pitch<b.pitch;return a.id<b.id;});}
struct Ids {
 std::set<Id> used;Id next=1;
 explicit Ids(const Composition& c){for(const auto& s:c.sections)used.insert(s.id);for(const auto& h:c.chords)used.insert(h.id);for(const auto& t:c.tracks){used.insert(t.id);for(const auto& n:t.notes)used.insert(n.id);for(const auto& cc:t.controls)used.insert(cc.id);}if(!used.empty()&&*used.rbegin()!=std::numeric_limits<Id>::max())next=*used.rbegin()+1;}
 Id take(){for(;;){auto id=next;next=next==std::numeric_limits<Id>::max()?1:next+1;if(used.insert(id).second)return id;}}
};
inline LoopEditResult finish(Composition c){auto checked=validate(c);if(!checked)return {{},checked.message};return {std::move(c),{}};}
}
inline LoopEditResult editLoopNote(const Composition& source,Id trackId,Id noteId,const std::optional<Note>& replacement){
 try{
  auto checked=validate(source);if(!checked)return {{},checked.message};
  const auto* target=loopEditDetail::track(source,trackId);if(!target)return {{},"Track not found"};
  const auto found=std::find_if(target->notes.begin(),target->notes.end(),[&](const auto& n){return n.id==noteId;});
  if((noteId&&found==target->notes.end())||(!noteId&&!replacement))return {{},"Note not found"};
  if(replacement&&!loopEditDetail::note(*target,*replacement,source.endTick))return {{},"Invalid note"};
  if(!noteId&&loopEditDetail::count(source)>=limits::notes)return {{},"Note limit reached"};
  Composition candidate=source;auto* t=const_cast<Track*>(loopEditDetail::track(candidate,trackId));
  if(!noteId){loopEditDetail::Ids ids(source);auto n=*replacement;n.id=ids.take();t->notes.push_back(n);}
  else{auto at=t->notes.begin()+(found-target->notes.begin());if(replacement){auto n=*replacement;n.id=noteId;*at=n;}else t->notes.erase(at);}
  loopEditDetail::sort(*t);return loopEditDetail::finish(std::move(candidate));
 }catch(const std::bad_alloc&){return {{},"Edit memory"};}
}
inline LoopEditResult replaceLoopRange(const Composition& source,Id trackId,Tick start,Tick end,const std::vector<Note>& replacements){
 try{
  auto checked=validate(source);if(!checked)return {{},checked.message};
  const auto* target=loopEditDetail::track(source,trackId);if(!target||start.value<0||start>=end||end>source.endTick)return {{},"Invalid selection"};
  if(replacements.size()>limits::notes)return {{},"Note limit reached"};
  for(const auto& n:replacements)if(n.start<start||n.start>=end||!loopEditDetail::note(*target,n,end))return {{},"Invalid replacement"};
  auto selected=[&](const Note& n){return n.start>=start&&n.start<end;};
  const auto removed=size_t(std::count_if(target->notes.begin(),target->notes.end(),selected));
  if(replacements.size()>limits::notes-(loopEditDetail::count(source)-removed))return {{},"Note limit reached"};
  loopEditDetail::Ids ids(source);Composition candidate=source;auto* t=const_cast<Track*>(loopEditDetail::track(candidate,trackId));
  t->notes.erase(std::remove_if(t->notes.begin(),t->notes.end(),selected),t->notes.end());
  for(auto n:replacements){n.id=ids.take();t->notes.push_back(n);}
  loopEditDetail::sort(*t);return loopEditDetail::finish(std::move(candidate));
 }catch(const std::bad_alloc&){return {{},"Edit memory"};}
}
}
