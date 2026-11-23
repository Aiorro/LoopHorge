#include "../Source/engine/Midi.h"
#include "../Source/engine/PromptCompose.h"
#include "../Source/engine/Prompt.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include <set>
using namespace composer;
#define check(x) do {if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
Composition fixture(){
    Composition c;c.id="midi";c.endTick={6240};
    c.tempos={{{0},500000},{{2880},400000}};
    c.meters={{{0},3,4},{{2880},5,8}};
    c.keys={{{0},0,Scale::major},{{2880},2,Scale::dorian}};
    c.sections={{20,"Verse",{0},{2880},SectionRole::verse},{21,"Chorus",{2880},{6240},SectionRole::chorus}};
    Track t;t.id=1;t.name="Bass";t.channel=16;t.program=33;t.bank=Bank{2,7};
    t.notes={{2,60,90,{0},{480}},{3,60,100,{240},{720}},{4,60,110,{960},{480}}};
    t.controls={{5,{0},64,127},{6,{1200},1,75},{7,{6240},66,127},{8,{0},69,127}};
    c.tracks={t};return c;
}
juce::MidiFile decode(const MidiResult& encoded){
    check(encoded);check(encoded.bytes.size()>14);
    juce::MemoryInputStream input(encoded.bytes.data(),encoded.bytes.size(),false);
    juce::MidiFile file;int type=0;check(file.readFrom(input,false,&type));check(type==1&&file.getTimeFormat()==480);return file;
}
using Edge=std::tuple<int64_t,bool,int,int>;
std::vector<Edge> notes(const juce::MidiMessageSequence& s){
    std::vector<Edge> result;for(auto* e:s){auto m=e->message;if(m.isNoteOnOrOff())result.emplace_back(int64_t(m.getTimeStamp()),m.isNoteOn(),m.getNoteNumber(),m.getVelocity());}return result;
}
void auditUserDirections(){
    const auto compose=[](const std::string& direction){
        return generateResolvedLoop(LocalPromptInterpreter{}.interpret(direction,42));
    };
    const std::array<const char*,12> roots{"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
    for(int root=0;root<12;++root)for(bool minor:{false,true}){
        const auto allowed=minor?std::set<int>{0,2,3,5,7,8,10}:std::set<int>{0,2,4,5,7,9,11};
        std::set<std::vector<Edge>> bassPatterns;
        for(const auto* style:{"trap","boom-bap","r&b","ambient","nu-metal"}){
            auto result=compose(std::string(style)+" 4 bars "+roots[root]+(minor?" minor":" major")+" bass chords lead drums");
            check(result.composition);
            const auto& song=*result.composition;
            check(song.style==style && song.keys.front().root==root);
            auto file=decode(encodeCompositionMidi(song));
            check(file.getNumTracks()==5);
            for(int track=1;track<5;++track){
                auto edges=notes(*file.getTrack(track));
                check(!edges.empty());
                if(track==1)bassPatterns.insert(edges);
                for(auto* event:*file.getTrack(track)){
                    auto& m=event->message;
                    if(!m.isNoteOn())continue;
                    if(track==4)check(m.getChannel()==10);
                    else check(allowed.count((m.getNoteNumber()-root+12)%12));
                }
            }
        }
        check(bassPatterns.size()==5);
    }
    for(int bars:{4,8,16}){
        std::vector<Edge> baseline;
        for(int bpm:{60,120,180}){
            auto result=compose(std::to_string(bars)+" bars D minor piano "+std::to_string(bpm)+" bpm");
            check(result.composition);
            auto file=decode(encodeCompositionMidi(*result.composition));
            check(file.getNumTracks()==2 && file.getTrack(1)->getEndTime()==bars*4*480);
            bool tempo=false,piano=false;
            for(auto* e:*file.getTrack(0))if(e->message.isTempoMetaEvent())
                tempo=std::abs(e->message.getTempoSecondsPerQuarterNote()-60.0/bpm)<0.000001;
            for(auto* e:*file.getTrack(1))if(e->message.isProgramChange())
                piano=e->message.getProgramChangeNumber()==0;
            check(tempo&&piano);
            auto edges=notes(*file.getTrack(1));
            if(baseline.empty())baseline=edges;
            check(!edges.empty()&&baseline==edges);
        }
    }
    for(auto* role:{"bass","lead","drums"}){
        auto result=compose(std::string("4 bars ")+role);
        check(result.composition&&result.composition->tracks.size()==1);
        check(std::string(roleName(result.composition->tracks.front().role))==role);
    }
    auto piano=compose("4 bars piano"),chords=compose("4 bars chords");
    check(piano.composition && chords.composition);
    auto pianoFile=decode(encodeCompositionMidi(*piano.composition));
    auto chordsFile=decode(encodeCompositionMidi(*chords.composition));
    check(notes(*pianoFile.getTrack(1))==notes(*chordsFile.getTrack(1)));
    check(!chords.composition->tracks.front().program);
    auto cello=compose("4 bars виолончель");
    check(cello.composition);
    auto celloFile=decode(encodeCompositionMidi(*cello.composition));
    check(celloFile.getNumTracks()==2);
    bool hasCello=false;
    for(auto* event:*celloFile.getTrack(1))
        if(event->message.isProgramChange()&&event->message.getProgramChangeNumber()==42)hasCello=true;
    check(hasCello&&!notes(*celloFile.getTrack(1)).empty());
    auto mixed=compose("4 bars trap70% ambient30%");
    check(!mixed.composition && mixed.error.find("Blended-genre")!=std::string::npos);
    auto endpoint=compose("4 bars trap0% ambient100%"),ambient=compose("4 bars ambient");
    check(endpoint.composition && ambient.composition);
    check(encodeCompositionMidi(*endpoint.composition).bytes==encodeCompositionMidi(*ambient.composition).bytes);
    check(!compose("4 bars guitar").composition);
}
int main(){
    auditUserDirections();
    auto c=fixture();check(validate(c));auto encoded=encodeCompositionMidi(c);auto file=decode(encoded);check(file.getNumTracks()==2);
    for(int i=0;i<2;++i)check(file.getTrack(i)->getEndTime()==6240);
    const std::vector<Edge> expected={{0,true,60,90},{240,false,60,0},{240,true,60,100},{960,false,60,0},{960,true,60,110},{1440,false,60,0}};
    check(notes(*file.getTrack(1))==expected);
    std::vector<std::pair<int,int>> initial;bool named=false,modulation=false;std::array<bool,3> resets{};
    for(auto* e:*file.getTrack(1)){
        auto m=e->message;if(!m.isMetaEvent())check(m.getChannel()==16);
        if(m.isTrackNameEvent())named=m.getTextFromTextMetaEvent()=="Bass";
        if(m.getTimeStamp()==0){if(m.isController())initial.emplace_back(m.getControllerNumber(),m.getControllerValue());if(m.isProgramChange())initial.emplace_back(-1,m.getProgramChangeNumber());}
        if(m.isController()&&m.getControllerNumber()==1)modulation=m.getTimeStamp()==1200&&m.getControllerValue()==75;
        if(m.isController()&&m.getControllerValue()==0&&m.getTimeStamp()==6240){int cc=m.getControllerNumber();if(cc==64)resets[0]=true;if(cc==66)resets[1]=true;if(cc==69)resets[2]=true;}
    }
    check(named&&modulation&&resets[0]&&resets[1]&&resets[2]);check(initial.size()>=3);
    check((initial[0]==std::pair<int,int>{0,2}));check((initial[1]==std::pair<int,int>{32,7}));check((initial[2]==std::pair<int,int>{-1,33}));
    int tempos=0,meters=0,keys=0,markers=0;bool dorian=false;
    for(auto* e:*file.getTrack(0)){
        auto m=e->message;auto tick=int64_t(m.getTimeStamp());
        if(m.isTempoMetaEvent()){check((tick==0&&m.getTempoSecondsPerQuarterNote()==0.5)||(tick==2880&&m.getTempoSecondsPerQuarterNote()==0.4));++tempos;}
        if(m.isTimeSignatureMetaEvent()){int n=0,d=0;m.getTimeSignatureInfo(n,d);check((tick==0&&n==3&&d==4)||(tick==2880&&n==5&&d==8));++meters;}
        if(m.isKeySignatureMetaEvent()){check(m.getKeySignatureNumberOfSharpsOrFlats()==0);++keys;}
        if(m.getMetaEventType()==6){check((tick==0&&m.getTextFromTextMetaEvent()=="Verse")||(tick==2880&&m.getTextFromTextMetaEvent()=="Chorus"));++markers;}
        if(m.isTextMetaEvent()&&tick==2880&&m.getTextFromTextMetaEvent().contains("D dorian"))dorian=true;
    }
    check(tempos==2&&meters==2&&keys==2&&markers==2&&dorian);
    check(encoded.bytes==encodeCompositionMidi(c).bytes);check(c.tracks[0].notes[0].duration.value==480&&c.tracks[0].controls.size()==4);
    auto simultaneous=fixture();simultaneous.tracks[0].notes={{2,60,90,{0},{480}},{3,60,100,{0},{960}}};
    check((notes(*decode(encodeCompositionMidi(simultaneous)).getTrack(1))==std::vector<Edge>{{0,true,60,100},{960,false,60,0}}));
    // Unordered input must sort semantically, without mutating the source.
    std::reverse(c.tracks[0].notes.begin(),c.tracks[0].notes.end());std::reverse(c.tracks[0].controls.begin(),c.tracks[0].controls.end());
    check(notes(*decode(encodeCompositionMidi(c)).getTrack(1))==expected);check(c.tracks[0].notes.front().id==4);
    // A legal high-tempo timeline needs multiple four-byte delta spans.
    c=fixture();c.tempos={{{0},1}};c.meters={{{0},4,4}};c.keys={{{0},0,Scale::major}};c.sections.clear();c.endTick={960000000};
    c.tracks[0].controls.clear();c.tracks[0].notes={{2,60,90,{959999999},{1}}};
    file=decode(encodeCompositionMidi(c));check(file.getTrack(0)->getEndTime()==960000000&&file.getTrack(1)->getEndTime()==960000000);
    check((notes(*file.getTrack(1))==std::vector<Edge>{{959999999,true,60,90},{960000000,false,60,0}}));
    // Count must not wrap after 65535 equal-pitch overlapping onsets.
    c=fixture();c.tracks[0].notes.clear();c.tracks[0].controls.clear();
    for(size_t i=0;i<131072;++i)c.tracks[0].notes.push_back({100+i,60,90,{0},{480}});
    file=decode(encodeCompositionMidi(c));auto dense=notes(*file.getTrack(1));check(dense.size()==2&&std::get<0>(dense.back())==480&&!std::get<1>(dense.back()));
    c.tracks[0].notes.push_back({200000,60,90,{0},{480}});auto bad=encodeCompositionMidi(c);check(!bad&&bad.bytes.empty()&&!bad.error.empty());
    c=fixture();c.ppq=960;bad=encodeCompositionMidi(c);check(!bad&&bad.bytes.empty());
    c=fixture();c.tempos[1].tick={0};check(!encodeCompositionMidi(c));
    // All channels, including two disjoint fixed-GM channel10 tracks.
    c=fixture();c.tracks.clear();c.sections.clear();
    for(int ch=1;ch<=16;++ch){Track t;t.id=100+ch;t.name="Track";t.channel=ch;t.role=ch==10?Role::drums:Role::bass;t.notes={{Id(200+ch),ch==10?36:60,90,{0},{480}}};if(ch==10)t.ownedPitches.set(36);c.tracks.push_back(t);}
    file=decode(encodeCompositionMidi(c));check(file.getNumTracks()==17);for(int ch=1;ch<=16;++ch)for(auto* e:*file.getTrack(ch))if(e->message.isNoteOn())check(e->message.getChannel()==ch);
    c.tracks.erase(c.tracks.begin()+15);Track perc;perc.id=999;perc.name="Percussion";perc.role=Role::percussion;perc.channel=10;perc.ownedPitches.set(42);perc.notes={{1000,42,80,{0},{480}}};c.tracks.push_back(perc);
    file=decode(encodeCompositionMidi(c));check(file.getNumTracks()==17);check(std::get<2>(notes(*file.getTrack(16)).front())==42);
    LocalPromptInterpreter interpreter;auto request=interpreter.interpret("D minor trap 8 bars drums piano energy 75",123,{});auto generated=generateResolvedLoop(request);check(generated.composition);
    file=decode(encodeCompositionMidi(*generated.composition));check(file.getNumTracks()==3);for(int i=0;i<3;++i)check(file.getTrack(i)->getEndTime()==15360);
    // Every supported mode has an exact label even when SMF has no mode field.
    constexpr const char* labels[]={"major","natural minor","harmonic minor","melodic minor","dorian","phrygian","lydian","mixolydian","major pentatonic","minor pentatonic","blues"};
    constexpr int accidentals[]={0,-3,-3,-3,-2,-4,1,-1,0,-3,-3};
    for(int scale=0;scale<=int(Scale::blues);++scale){
        c=fixture();c.keys={{{0},0,Scale(scale)}};file=decode(encodeCompositionMidi(c));bool label=false,signature=false;
        for(auto* e:*file.getTrack(0)){
            auto m=e->message;
            if(m.isTextMetaEvent()&&m.getTextFromTextMetaEvent()==juce::String("LoopForge key: C ")+labels[scale])label=true;
            if(m.isKeySignatureMetaEvent())signature=m.getKeySignatureNumberOfSharpsOrFlats()==accidentals[scale];
        }
        check(label&&signature);
    }
    std::cout<<"composition MIDI maps/tracks/presets/CC, counted overlaps, long deltas, maximum lifetimes and generated loops passed\n";
}
