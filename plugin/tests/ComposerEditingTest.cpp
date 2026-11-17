#include "../Source/Processor.h"
#include "../Source/engine/LoopEditing.h"
#include <iostream>
#define check(x) do{if(!(x)){std::cerr<<__LINE__<<": " #x "\n";std::abort();}}while(false)
using namespace composer;
std::string saved(Processor& p){juce::MemoryBlock b;p.getStateInformation(b);return {static_cast<const char*>(b.getData()),b.getSize()};}
Composition song(Processor& p){auto d=decodeProjectJson(saved(p));check(d.project&&d.project->composition);return *d.project->composition;}
void idle(Processor& p){for(int i=0;i<2000&&p.busy;++i)juce::Thread::sleep(5);check(!p.busy);}
int main(){juce::ScopedJuceInitialiser_GUI init;auto owner=std::make_unique<Processor>();auto& p=*owner;check(p.requestComposer("4 bars bass chords lead drums",42));idle(p);auto original=song(p);const auto track=original.tracks[0].id;const auto id=original.tracks[0].notes[0].id;auto note=original.tracks[0].notes[0];note.velocity=71;
 check(p.editComposerNote(p.snapshot().revision,track,id,note)&&p.canUndo()&&!p.canRedo());auto edited=song(p);check(edited.tracks[0].notes[0].id==id&&edited.tracks[0].notes[0].velocity==71);
 GenerationRecipe next;next.direction="8 bars energy80";next.seed=777;check(p.setComposerDraft(next));check(p.undoEdit());check(encodeCompositionJson(song(p)).json==encodeCompositionJson(original).json);check(decodeRecipeJson(*p.composerDraft(UINT64_MAX)->json).recipe->seed==777);check(p.redoEdit()&&encodeCompositionJson(song(p)).json==encodeCompositionJson(edited).json);
 auto stale=p.snapshot().revision;note.velocity=72;check(p.editComposerNote(stale,track,id,note));const auto before=saved(p);check(!p.editComposerNote(stale,track,id,note)&&saved(p)==before);
 auto beforeRange=song(p);std::vector<Note> notes{{0,beforeRange.tracks[0].notes[0].pitch,90,{0},{120}}};auto expected=replaceLoopRange(beforeRange,track,{0},{480},notes);check(expected.composition);check(p.replaceComposerRange(p.snapshot().revision,track,{0},{480},notes));check(encodeCompositionJson(song(p)).json==encodeCompositionJson(*expected.composition).json);check(p.undoEdit()&&encodeCompositionJson(song(p)).json==encodeCompositionJson(beforeRange).json);
 check(p.selectMode(ProjectMode::legacy)&&!p.canUndo());check(p.selectMode(ProjectMode::composer)&&!p.canUndo());
 for(int i=0;i<40;++i){auto c=song(p);auto n=c.tracks[0].notes[0];n.velocity=n.velocity==80?81:80;check(p.editComposerNote(p.snapshot().revision,track,n.id,n));}
 int undos=0;while(p.undoEdit())++undos;check(undos==32);int redos=0;while(p.redoEdit())++redos;check(redos==32);
 auto dense=original;dense.endTick={30720};for(auto& t:dense.tracks)t.notes.clear();for(size_t i=0;i<8192;++i)dense.tracks[0].notes.push_back({1000000+i,dense.tracks[0].lowPitch,90,{int64_t(i*3)},{1}});ProjectState denseState;denseState.activeMode=ProjectMode::composer;denseState.composition=dense;auto denseJson=encodeProjectJson(denseState);check(denseJson&&p.restoreStateInformation(denseJson.json.data(),int(denseJson.json.size()))&&p.canEditComposer());const auto beforeLimit=saved(p);auto extra=dense.tracks[0].notes[0];extra.start={30000};check(!p.editComposerNote(p.compositionRevision(),dense.tracks[0].id,0,extra)&&saved(p)==beforeLimit&&p.canEditComposer()&&!p.canUndo());
 std::cout<<"Composer note edits, scoped replacements,32step undo/redo, draft preservation and stale guards passed\n";
}
