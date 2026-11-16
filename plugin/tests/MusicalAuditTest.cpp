#include "../Source/engine/PromptCompose.h"
#include <iostream>
#include <set>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<__LINE__<<": " #x "\n";std::abort();}}while(false)
std::vector<int64_t> events(const Composition& c){std::vector<int64_t> v;for(auto& t:c.tracks)for(auto& n:t.notes)for(auto x:{int64_t(n.pitch),int64_t(n.velocity),n.start.value,n.duration.value})v.push_back(x);return v;}
int main(int argc,char** argv){int part=argc>1?std::atoi(argv[1]):0;
 if(!part||part==1)for(int style=0;style<5;++style){Config c;c.scale=Scale::blues;c.style=Style(style);Mood a,b;a.tension=0;b.tension=100;check(events(*generateMoodLoop(c,a).composition)!=events(*generateMoodLoop(c,b).composition));}
 if(!part||part==2){Config c;c.drums=true;c.humanization=0;c.ghostNotes=c.drumFills=false;c.style=Style::boomBap;c.numerator=6;c.denominator=8;auto a=*generateLoop(c).composition;c.numerator=3;c.denominator=4;auto b=*generateLoop(c).composition;check(events(a)!=events(b));std::vector<int64_t> hats,snares;for(auto& n:a.tracks[3].notes)if(n.start.value<1440){if(n.pitch==42)hats.push_back(n.start.value);if(n.pitch==38)snares.push_back(n.start.value);}check(hats==std::vector<int64_t>({0,240,480,720,960,1200}));check(snares==std::vector<int64_t>({720}));}
 if(!part||part==3){Config c;c.swing=100;c.humanization=0;musicDetail::Random a(1),b(1);check(musicDetail::onset(3,8,1920,c,a)==musicDetail::onset(6,16,1920,c,b));}
 if(!part||part==4)for(int style=0;style<5;++style){Config c;c.style=Style(style);c.humanization=0;std::set<std::vector<int64_t>> unique;for(int seed=0;seed<256;++seed){c.seed=seed;auto r=generateLoop(c);check(r.composition);auto v=events(*r.composition);check(v==events(*generateLoop(c).composition));unique.insert(v);}check(unique.size()>=32);}
 if(!part||part==5){Config c;c.humanization=0;c.numerator=6;c.denominator=8;c.complexity=0;auto a=*generateLoop(c).composition;c.complexity=50;auto b=*generateLoop(c).composition;check(events(a)!=events(b));check(a.tracks[2].notes.size()<b.tracks[2].notes.size());std::set<std::vector<int64_t>> rhythms;for(int style=0;style<5;++style){c.style=Style(style);auto g=*generateLoop(c).composition;std::vector<int64_t> onsets;for(auto& n:g.tracks[0].notes)if(n.start.value<1440)onsets.push_back(n.start.value);rhythms.insert(onsets);}check(rhythms.size()>=4);}
 if(!part||part==6)for(auto contour:{Contour::rising,Contour::falling})for(int seed:{1,2,42}){Config c;c.style=Style::trap;c.seed=seed;c.humanization=0;c.performance.contour=contour;auto g=*generateLoop(c).composition;std::vector<int> interior;for(auto& n:g.tracks[2].notes)if(n.start.value>0&&n.start.value<1680)interior.push_back(n.pitch);for(size_t i=1;i<interior.size();++i)check(contour==Contour::rising?interior[i]>=interior[i-1]:interior[i]<=interior[i-1]);}
 if(!part||part==7){
  for(int numerator:{3,5}){
   Config c;c.drums=true;c.ghostNotes=c.drumFills=false;c.humanization=0;c.style=Style::boomBap;c.feel=Feel::half;c.numerator=numerator;c.bars=4;
   auto straight=generateLoop(c);check(straight.composition);c.swing=100;auto swung=generateLoop(c);check(swung.composition&&validate(*swung.composition));
   std::vector<int64_t> snares;for(const auto& n:swung.composition->tracks[3].notes)if(n.pitch==38)snares.push_back(n.start.value);
   check(snares==(numerator==3?std::vector<int64_t>{960,1920,3840,4800}:std::vector<int64_t>{2880,7680}));
  }
  // Global swing grid must not restart at an odd bar. Compare actual notes, not a copied generator.
  for(int style=0;style<5;++style)for(int numerator:{1,3,5,7,9,11})for(int denominator:{2,4,8,16})for(auto feel:{Feel::half,Feel::normal,Feel::doubleTime}){
   Config c;c.drums=true;c.ghostNotes=c.drumFills=false;c.humanization=0;c.numerator=numerator;c.denominator=denominator;c.feel=feel;c.bars=4;c.style=Style(style);
   auto a=generateLoop(c);check(a.composition);c.swing=100;auto b=generateLoop(c);check(b.composition&&validate(*b.composition));
   const int64_t bar=1920*numerator/denominator;int64_t step=numerator>=6&&numerator%3==0?960/denominator:240;
   if(feel==Feel::half)step*=2;if(feel==Feel::doubleTime)step/=2;
   for(size_t track:{size_t(0),size_t(2),size_t(3)}){
    const auto& before=a.composition->tracks[track].notes;const auto& after=b.composition->tracks[track].notes;check(before.size()==after.size());
    for(size_t i=0;i<before.size();++i){const auto tick=before[i].start.value;const auto phase=tick%(2*step);const auto delay=std::min(std::min(phase,2*step-phase)/2,(bar-1-tick%bar)/2);
     check(after[i].start.value==tick+delay);check(after[i].pitch==before[i].pitch);
    }
   }
  }
 }
 if(!part||part==8){
  for(int denominator:{2,4,8,16})for(int style=0;style<5;++style)for(auto feel:{Feel::half,Feel::normal,Feel::doubleTime})for(int swing:{0,50,100})for(int humanization:{0,100})for(uint32_t seed:{0U,42U}){
   Config c;c.numerator=1;c.denominator=denominator;c.style=Style(style);c.feel=feel;c.swing=swing;c.humanization=humanization;c.seed=seed;c.drumFills=false;
   c.ghostNotes=false;auto a=generateDrums(c);c.ghostNotes=true;auto b=generateDrums(c);check(a.track&&b.track);
   const int snare=style==int(Style::ambient)?37:38;bool quiet=false;
   for(const auto& n:b.track->notes)if(n.pitch==snare&&n.velocity<=42){quiet=true;for(const auto& strong:a.track->notes)if(strong.pitch==snare)check(strong.start!=n.start);}
   check(quiet&&b.track->notes.size()>a.track->notes.size());
   for(const auto& strong:a.track->notes){bool preserved=false;for(const auto& n:b.track->notes)if(n.pitch==strong.pitch&&n.start==strong.start&&n.velocity==strong.velocity)preserved=true;check(preserved);}
   auto song=generateLoop(c);check(song.composition&&validate(*song.composition));
  }
 }
 if(!part){
  auto a=generateResolvedLoop(LocalPromptInterpreter{}.interpret("4 bars blues tension0",42));auto b=generateResolvedLoop(LocalPromptInterpreter{}.interpret("4 bars blues tension100",42));check(a.composition&&b.composition&&events(*a.composition)!=events(*b.composition));
  for(int scale=0;scale<11;++scale)for(int voices=3;voices<=7;++voices){Key key{{0},0,Scale(scale)};auto chord=chordPitches(key,0,voices,4);check(chord&&chord->size()==std::min(size_t(voices),scaleIntervals(key.scale).size()));}
  for(auto feel:{Feel::half,Feel::normal,Feel::doubleTime})for(int swing:{0,50,100}){Config c;c.humanization=0;c.feel=feel;c.swing=swing;for(int tick=0;tick<16;++tick){musicDetail::Random a(1),b(2);check(musicDetail::onset(tick,16,1920,c,a)==musicDetail::onset(tick*2,32,1920,c,b));}}
 }
 std::cout<<"Musical audit regression passed\n";
}
