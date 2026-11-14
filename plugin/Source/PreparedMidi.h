#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "engine/Playback.h"
#if !defined(LOOPFORGE_VERIFIED_MIDI_LAYOUT) || LOOPFORGE_VERIFIED_MIDI_LAYOUT != 1
 #error "Link loopforge_prepared_midi after its pinned JUCE layout verification"
#endif

namespace composer {
// Ordered short channel messages only, empty caller buffer reserved outside callback.
// This raw layout is guarded against both pinned JUCE source/header hashes at configure.
class PreparedMidiSink {
public:
    explicit PreparedMidiSink(juce::MidiBuffer& output)noexcept:buffer(output),failed(!output.isEmpty()){}
    bool operator()(MidiPacket packet,int sample)noexcept{
        const int type=packet.status&0xf0;const int size=(type==0xc0||type==0xd0)?2:3;
        if(failed||messages>=2048||packet.status<0x80||packet.status>=0xf0||packet.size!=size||packet.data1>127
           ||(size==3&&packet.data2>127)||sample<lastSample||sample<0){failed=true;return false;}
        constexpr size_t header=sizeof(juce::int32)+sizeof(juce::uint16);
        static_assert(header==6);
        std::array<juce::uint8,header+3> bytes{};
        juce::writeUnaligned<juce::int32>(bytes.data(),sample);
        juce::writeUnaligned<juce::uint16>(bytes.data()+sizeof(juce::int32),uint16_t(size));
        bytes[header]=packet.status;bytes[header+1]=packet.data1;bytes[header+2]=packet.data2;
        // MidiBuffer::addEvent rescans all previous events, even for sorted appends.
        buffer.data.addArray(bytes.data(),int(header)+size);lastSample=sample;++messages;return true;
    }
    bool rejected()const noexcept{return failed;}
private:
    juce::MidiBuffer& buffer;int lastSample=-1,messages=0;bool failed=false;
};
}
