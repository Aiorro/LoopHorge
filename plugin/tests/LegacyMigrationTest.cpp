#include "../Source/engine/LegacyMigration.h"
#include "../Source/engine/CompositionJson.h"
#include <iostream>
#include <cstdlib>

#define check(x) do { if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();} }while(false)
using namespace composer;

int main(int argc,char** argv) {
    check(argc==2);
    auto idea=wire::document(juce::File(argv[1]).loadFileAsString().toStdString());check(idea.isObject());
    auto original=juce::JSON::toString(idea,true);
    auto result=importLegacyIdea(idea);check(result.composition);check(result.error.empty());
    auto c=*result.composition;check(validate(c));
    check(c.schemaVersion==2&&c.ppq==480&&c.seed==42&&c.endTick.value==7680);
    check(c.keys[0].root==2&&c.keys[0].scale==Scale::naturalMinor);
    check(c.style=="nu-metal"&&c.tempos[0].microsPerQuarter==600000);
    check(c.tracks.size()==3&&c.tracks[0].channel==1&&c.tracks[1].channel==2&&c.tracks[2].channel==3);
    check(c.tracks[0].notes[0].pitch==38&&c.tracks[0].notes[0].duration.value==480);
    check(c.tracks[2].notes[0].start.value==240&&c.tracks[2].notes[0].duration.value==240);
    check(c.legacyJson==original.toStdString());check(juce::JSON::toString(idea,true)==original);
    auto saved=encodeCompositionJson(c);check(saved);auto restored=decodeCompositionJson(saved.json);check(restored.composition);
    check(restored.composition->legacyJson==original.toStdString()&&restored.composition->timingNotice==c.timingNotice);
    check(restored.composition->tracks[2].notes[0].id==c.tracks[2].notes[0].id&&restored.composition->tracks[0].notes[0].pitch==38);
    auto repeat=importLegacyIdea(idea);check(repeat.composition->id==c.id);
    check(repeat.composition->tracks[2].notes[0].id==c.tracks[2].notes[0].id);
    check(!result.timingNotice.empty());

    auto state=juce::var(new juce::DynamicObject);state.getDynamicObject()->setProperty("version",1);
    state.getDynamicObject()->setProperty("idea",idea);check(importLegacyState(state).composition);
    for(auto version:{juce::var(true),juce::var(2),juce::var("1"),juce::var(1.5)}){
        state.getDynamicObject()->setProperty("version",version);check(!importLegacyState(state).composition);
    }
    check(!importLegacyState(idea).composition);
    auto rejects=[&](auto mutation){auto bad=idea.clone();mutation(bad);auto rejected=importLegacyIdea(bad);check(!rejected.composition&&!rejected.error.empty());};
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("seed",true);});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("seed",1.5);});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("seed",-1);});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("seed",4294967296.0);});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("key","H");});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("mode","magic");});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("style","unsupported");});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("bpm",true);});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("bpm","100");});
    rejects([](auto& x){x["metadata"].getDynamicObject()->setProperty("bars","4");});
    rejects([](auto& x){x["layers"]["bass"][0].getDynamicObject()->setProperty("pitch",128);});
    rejects([](auto& x){auto* a=x["layers"]["bass"].getArray();for(int i=0;i<801;++i)a->add((*a)[0].clone());});
    rejects([](auto& x){x.getDynamicObject()->setProperty("extra",juce::String::repeatedString("a",4*1024*1024+1));});
    rejects([](auto& x){auto child=juce::var(new juce::DynamicObject);for(int i=0;i<40;++i){auto next=juce::var(new juce::DynamicObject);next.getDynamicObject()->setProperty("child",child);child=next;}x.getDynamicObject()->setProperty("extra",child);});

    auto missing=idea.clone();missing["metadata"].getDynamicObject()->removeProperty("seed");
    result=importLegacyIdea(missing);check(result.composition&&result.composition->seed==0);
    check(result.timingNotice.find("seed")!=std::string::npos);
    auto tiny=idea.clone();auto* n=tiny["layers"]["bass"][0].getDynamicObject();
    n->setProperty("start",15.9999);n->setProperty("duration",.00001);
    auto tinyOriginal=juce::JSON::toString(tiny,true);result=importLegacyIdea(tiny);
    check(result.composition&&result.composition->tracks[0].notes[0].start.value==7679);
    check(result.composition->tracks[0].notes[0].duration.value==1);
    check(result.timingNotice.find("quantized")!=std::string::npos);
    check(juce::JSON::toString(tiny,true)==tinyOriginal);
    auto fractional=idea.clone();fractional["metadata"].getDynamicObject()->setProperty("bpm",87.5);
    result=importLegacyIdea(fractional);check(result.composition&&result.composition->tempos[0].microsPerQuarter==685714);
    check(result.timingNotice.find("tempo")!=std::string::npos);
    std::cout<<"explicit legacy migration: fixture, metadata, source preservation and quantization passed\n";
}
