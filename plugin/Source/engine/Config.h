#pragma once
#include "Validation.h"

namespace composer {
enum class Style { trap,boomBap,rnb,ambient,nuMetal };
struct Modulation {int bar=4,root=0;Scale scale=Scale::major;};
struct Config {
    int root=0;Scale scale=Scale::naturalMinor;Style style=Style::nuMetal;
    int bpm=100,bars=8,numerator=4,denominator=4;
    uint32_t seed=0;
    int complexity=50,humanization=20,swing=0,harmonicBars=0;
    bool modalInterchange=false;
    bool drums=false,ghostNotes=true,drumFills=true;
    std::optional<Modulation> modulation;
};
inline const char* styleName(Style style){
    switch(style){case Style::trap:return "trap";case Style::boomBap:return "boom-bap";
        case Style::rnb:return "r&b";case Style::ambient:return "ambient";case Style::nuMetal:return "nu-metal";}
    return "unknown";
}
inline ValidationResult validateConfig(const Config& c){
    auto invalid=[](){return ValidationResult{ValidationCode::identity,"Invalid native composer configuration"};};
    if(c.root<0||c.root>11||c.scale<Scale::major||c.scale>Scale::blues||c.style<Style::trap||c.style>Style::nuMetal
       ||c.bpm<40||c.bpm>240||(c.bars!=4&&c.bars!=8&&c.bars!=16&&c.bars!=32)
       ||c.numerator<1||c.numerator>12||(c.denominator!=2&&c.denominator!=4&&c.denominator!=8&&c.denominator!=16)
       ||c.complexity<0||c.complexity>100||c.humanization<0||c.humanization>100||c.swing<0||c.swing>100
       ||(c.harmonicBars!=0&&c.harmonicBars!=1&&c.harmonicBars!=2&&c.harmonicBars!=4))return invalid();
    if(c.modulation&&(c.modulation->bar<4||c.modulation->bar>=c.bars||c.modulation->bar%4!=0
       ||c.modulation->root<0||c.modulation->root>11||c.modulation->scale<Scale::major||c.modulation->scale>Scale::blues))return invalid();
    const int harmonicBars=c.harmonicBars?c.harmonicBars:(c.style==Style::ambient?2:1);
    if(c.modalInterchange&&c.bars/harmonicBars<2)return {ValidationCode::harmony,"Modal interchange needs at least two harmonic spans for borrowing and resolution"};
    return {};
}
namespace musicDetail {
inline uint32_t mix(uint32_t x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
inline uint32_t seedFor(uint32_t seed,uint32_t role,uint32_t bar){return mix(seed^mix(role)^mix(bar+0x9e3779b9u));}
struct Random {
    uint32_t state;
    explicit Random(uint32_t seed):state(seed?seed:1){}
    uint32_t next(){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
    int offset(int maximum){return maximum>0?int(next()%uint32_t(maximum*2+1))-maximum:0;}
};
}
}
