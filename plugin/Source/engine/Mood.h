#pragma once
#include <algorithm>
#include <optional>

namespace composer {
struct Mood {
    int darkness=50,energy=50,melancholy=50,tension=50,aggression=50,atmosphere=50,complexity=50;
};
enum class Contour {seeded,rising,falling};
struct Performance {
    int octaveOffset=0,velocityOffset=0,gatePercent=100,chordGatePercent=100;
    int bassStride=1,leadStride=1,extraChordVoices=0,tensionOffset=0;
    Contour contour=Contour::seeded;
};
inline bool validPerformance(const Performance& p){
    return p.octaveOffset>=-1&&p.octaveOffset<=1&&p.velocityOffset>=-32&&p.velocityOffset<=32
        &&p.gatePercent>=50&&p.gatePercent<=150&&p.chordGatePercent>=50&&p.chordGatePercent<=150
        &&p.bassStride>=1&&p.bassStride<=2&&p.leadStride>=1&&p.leadStride<=2
        &&p.extraChordVoices>=-1&&p.extraChordVoices<=2&&p.tensionOffset>=-50&&p.tensionOffset<=50
        &&p.contour>=Contour::seeded&&p.contour<=Contour::falling;
}
inline bool validMood(const Mood& m){
    for(int value:{m.darkness,m.energy,m.melancholy,m.tension,m.aggression,m.atmosphere,m.complexity})if(value<0||value>100)return false;
    return true;
}
inline std::optional<Performance> deriveMood(const Mood& m){
    if(!validMood(m))return {};
    Performance p;
    p.octaveOffset=m.darkness>=75?-1:m.darkness<=25?1:0;
    p.velocityOffset=(m.energy-50)/5+(m.aggression-50)/3;
    p.gatePercent=std::clamp(100-(m.aggression-50)/2+(m.melancholy-50)/5,50,150);
    p.chordGatePercent=100+m.atmosphere-50;
    p.bassStride=m.energy<25?2:1;p.leadStride=m.energy<25||m.atmosphere>=75?2:1;
    p.extraChordVoices=m.tension>90?2:m.tension>70?1:m.tension<30?-1:0;
    p.tensionOffset=m.tension-50;
    p.contour=m.melancholy>=75?Contour::falling:m.melancholy<=25?Contour::rising:Contour::seeded;
    return p;
}
}
