#include "../Source/Processor.h"
#include "../Source/engine/RecipeJson.h"
#include <iostream>
#define check(x) do{if(!(x)){std::cerr<<__LINE__<<": " #x "\n";std::abort();}}while(false)
using namespace composer;
std::string bytes(Processor& p){juce::MemoryBlock block;p.getStateInformation(block);return {static_cast<const char*>(block.getData()),block.getSize()};}
void idle(Processor& p){for(int i=0;i<2000&&p.busy;++i)juce::Thread::sleep(5);check(!p.busy);}
int main(){juce::ScopedJuceInitialiser_GUI init;auto pStorage=std::make_unique<Processor>();auto& p=*pStorage;
 GenerationRecipe first;first.direction="16 bars energy100";first.seed=UINT32_MAX;first.manual.values.mood.energy=0;first.manual.pinned.set(size_t(Field::energy));
 const auto before=p.snapshot().revision;check(p.setComposerDraft(first)&&p.snapshot().revision==before);auto state=bytes(p);auto decoded=decodeProjectJson(state);check(decoded.project&&decoded.project->generationRecipeJson&&state.find(R"("version":3)")!=std::string::npos);
 auto draft=p.composerDraft(UINT64_MAX);check(draft&&draft->json&&decodeRecipeJson(*draft->json).recipe->seed==UINT32_MAX);
 auto restoredStorage=std::make_unique<Processor>();auto& restored=*restoredStorage;check(restored.restoreStateInformation(state.data(),int(state.size())));auto loaded=restored.composerDraft(UINT64_MAX);check(loaded&&loaded->json==draft->json&&bytes(restored)==state);
 check(p.requestComposer(first.direction,first.seed,first.manual));auto next=first;next.manual.values.mood.energy=80;next.direction="8 bars energy20";check(p.setComposerDraft(next));idle(p);check(p.composerSummary().contains("energy=0 [manual]"));
 auto saved=bytes(p);auto project=decodeProjectJson(saved);check(project.project&&project.project->composition&&project.project->composition->endTick.value==30720);auto recipe=decodeRecipeJson(*project.project->generationRecipeJson);check(recipe.recipe&&recipe.recipe->direction==next.direction&&recipe.recipe->manual.values.mood.energy==80);
 auto invalid=next;invalid.manual.values.mood.energy=101;check(!p.setComposerDraft(invalid)&&bytes(p)==saved);
 auto reopenedStorage=std::make_unique<Processor>();auto& reopened=*reopenedStorage;check(reopened.restoreStateInformation(saved.data(),int(saved.size())));check(reopened.composerDraft(UINT64_MAX)->json==encodeRecipeJson(next).json);
 project.project->generationRecipeJson.reset();auto v2=encodeProjectJson(*project.project);check(v2&&v2.json.find(R"("version":2)")!=std::string::npos);check(reopened.restoreStateInformation(v2.json.data(),int(v2.json.size())));check(!reopened.composerDraft(UINT64_MAX)->json&&bytes(reopened)==v2.json);
 check(!reopened.setComposerDraft(next,loaded->epoch)&&bytes(reopened)==v2.json);
 auto unsupported=first.manual;unsupported.pinned.set(size_t(Field::bpm));unsupported.values.recipe.bpm=90;check(reopened.requestComposer("4 bars",42,unsupported));idle(reopened);check(reopened.composerSummary().contains("bpm=90 [manual]"));
 project.project->generationRecipeJson="{}";check(!encodeProjectJson(*project.project));
 // A valid compactly-escaped old file may exceed the writer cap when normalized.
 {auto song=*project.project->composition;song.chords.clear();const std::string marker(1024,'X');
  for(int i=0;i<5600;++i){Chord h;h.id=100000+i;h.start={i};h.duration={1};h.root=0;h.pitchClasses={0,4,7};h.voicing={60,64,67};h.explanation=marker;song.chords.push_back(std::move(h));}
  auto normal=encodeCompositionJson(song);check(normal);std::string escaped;for(int i=0;i<1024;++i)escaped+="\\n";
  std::string compact;size_t begin=0;for(;;){auto at=normal.json.find(marker,begin);if(at==std::string::npos){compact.append(normal.json,begin,std::string::npos);break;}compact.append(normal.json,begin,at-begin);compact+=escaped;begin=at+marker.size();}
  const std::string input="{\"version\":2,\"activeMode\":\"composer\",\"hostStartQuarterBeat\":0,\"legacyStateJson\":null,\"composition\":"+compact+"}";
  auto largeStorage=std::make_unique<Processor>();auto& large=*largeStorage;check(large.restoreStateInformation(input.data(),int(input.size())));check(!large.setComposerDraft(first));check(bytes(large)==input);
 }
 // Publishing a larger Legacy result must also reserve room for the newer draft overlay.
 {auto song=*project.project->composition;song.chords.clear();for(int i=0;i<5000;++i){Chord h;h.id=100000+i;h.start={i};h.duration={1};h.root=0;h.pitchClasses={0,4,7};h.voicing={60,64,67};h.explanation=std::string(1024,'\n');song.chords.push_back(std::move(h));}
  const std::string legacy=R"({"version":1,"idea":{"metadata":{"bars":4,"bpm":120},"layers":{"bass":[{"pitch":38,"start":0,"duration":4,"velocity":90}],"chords":[],"melody":[]}}})";
  const auto params=juce::JSON::parse(R"({"provider":"offline","bars":8,"seed":42})");auto ideas=offline::generate(params);juce::var generated(new juce::DynamicObject);generated.getDynamicObject()->setProperty("version",1);generated.getDynamicObject()->setProperty("idea",ideas[0]);
  const auto nextLegacy=juce::JSON::toString(generated).toStdString();auto escaped=[](const std::string& value){jsonDetail::Writer w;w.string(value);return w.output.size();};
  auto largeDraft=first;largeDraft.direction=std::string(4000,' ')+"4 bars";auto oldRecipe=encodeRecipeJson(first);auto nextRecipe=encodeRecipeJson(largeDraft);check(oldRecipe&&nextRecipe);
  const auto delta=escaped(nextLegacy)-escaped(legacy);const auto extra=escaped(nextRecipe.json)-escaped(oldRecipe.json);const auto margin=delta+extra/2;check(margin>nextRecipe.json.size()*6+128);
  ProjectState near;near.activeMode=ProjectMode::legacy;near.legacyStateJson=legacy;near.composition=std::move(song);near.generationRecipeJson=oldRecipe.json;auto base=encodeProjectJson(near);check(base&&base.json.size()+margin<jsonDetail::maximumBytes);
  near.composition->legacyJson=std::string(jsonDetail::maximumBytes-margin-base.json.size(),' ');auto encoded=encodeProjectJson(near);check(encoded&&encoded.json.size()==jsonDetail::maximumBytes-margin);
  auto ownerStorage=std::make_unique<Processor>();auto& owner=*ownerStorage;check(owner.restoreStateInformation(encoded.json.data(),int(encoded.json.size()))&&owner.setComposerDraft(largeDraft));const auto previous=bytes(owner);const auto revision=owner.snapshot().revision;
  owner.request(juce::JSON::toString(params));idle(owner);check(owner.snapshot().revision==revision&&bytes(owner)==previous);
 }
 std::cout<<"host recipe state, pre-generation draft, in-flight edits, v1/v2 compatibility and invalid preservation passed\n";
}
