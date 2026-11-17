#pragma once
#include "Prompt.h"
#include "Generation.h"

namespace composer {
inline GenerationResult generateResolvedLoop(const ResolvedConfig& resolved){
    if(!resolved.valid())return {{},"Resolve direction errors before generation"};
    const auto& request=resolved.config;auto checked=validateDirection(request);if(!checked)return {{},checked.message};
    if(request.mode!=RequestMode::loop||request.duration.unit!=DurationUnit::bars)return {{},"Song/seconds arrangement is not integrated in the loop backend"};
    double bars=request.duration.amount;if(bars!=4&&bars!=8&&bars!=16)return {{},"Loop backend supports exactly4/8/16 bars (maximum16); requested duration was preserved"};
    if(!request.sections.empty())return {{},"Requested section arrangement is not integrated in the loop backend"};
    if(request.blend&&request.blend->primaryWeight>0&&request.blend->primaryWeight<100)return {{},"Blended-genre synthesis is not integrated in the loop backend"};
    for(size_t i=0;i<roleCount;++i)if(request.instruments[i]&&i!=size_t(Role::bass)&&i!=size_t(Role::chords)&&i!=size_t(Role::lead)&&i!=size_t(Role::drums))return {{},"Requested instrument role is not integrated in the loop backend"};
    if(request.instruments[size_t(Role::drums)]&&request.programs[size_t(Role::drums)])return {{},"The loop backend uses a fixed GM drum kit"};
    auto recipe=request.recipe;recipe.bars=int(bars);recipe.drums=request.instruments[size_t(Role::drums)];
    if(request.blend&&request.blend->primaryWeight==0)recipe.style=request.blend->secondary;
    auto result=generateMoodLoop(recipe,request.mood);if(!result.composition)return result;
    auto& tracks=result.composition->tracks;
    tracks.erase(std::remove_if(tracks.begin(),tracks.end(),[&](const Track& t){return !request.instruments[size_t(t.role)];}),tracks.end());
    result.composition->id+="-sel"+std::to_string(request.instruments.to_ulong());
    for(auto& track:tracks){track.program=request.programs[size_t(track.role)];
        if(track.program)result.composition->id+="-p"+std::to_string(int(track.role))+"-"+std::to_string(*track.program);}

    checked=validate(*result.composition);if(!checked)return {{},checked.message};
    return result;
}
}
