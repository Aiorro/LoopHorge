#pragma once
#include "Validation.h"
#include <cmath>

namespace composer {
// Conversion is non-RT. Validate the tempo domain without allocating or checking unrelated tracks.
inline std::optional<Seconds> secondsAt(const Composition& c,Tick target) {
    if(!detail::validTempos(c)||target.value<0||target>c.endTick)return {};
    return Seconds{detail::secondsUnchecked(c,target)};
}
inline std::optional<Tick> tickAt(const Composition& c,Seconds target) {
    if(!detail::validTempos(c)||!std::isfinite(target.value)||target.value<0)return {};
    double elapsed=0;
    for(size_t i=0;i<c.tempos.size();++i) {
        const auto& point=c.tempos[i];
        const auto end=i+1<c.tempos.size()?c.tempos[i+1].tick:c.endTick;
        const auto length=end.value-point.tick.value;
        const double perTick=point.microsPerQuarter/(480.0*1000000.0);
        const double span=length*perTick;
        if(target.value<=elapsed+span) {
            const auto offset=int64_t(std::llround((target.value-elapsed)/perTick));
            return Tick{point.tick.value+std::min(length,offset)};
        }
        elapsed+=span;
    }
    return {};
}
}
