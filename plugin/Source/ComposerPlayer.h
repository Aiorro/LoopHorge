#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "engine/Publication.h"
#include "engine/Playback.h"
#include "PreparedMidi.h"

namespace composer {
struct ComposerPlayerResult {PlaybackResult playback;uint64_t tag=0;bool hasSong=false,outputRejected=false;};

// SPSC callback component; worker/control ownership rules are those of PreparedMailbox.
class ComposerPlayer {
public:
    explicit ComposerPlayer(size_t retainedBudget=limits::documentBytes)noexcept:mailbox(retainedBudget){}
    PublishStatus publish(PreparedSong&& song,uint64_t tag)noexcept{return mailbox.publish(std::move(song),tag);}
    void clear()noexcept{mailbox.clear();} // Worker only; consumer flushes on next nonzero block.
    void reclaim()noexcept{mailbox.reclaim();} // Worker only.
    size_t retainedBytes()const noexcept{return mailbox.retainedBytes();} // Worker only.
    void resetStopped()noexcept{mailbox.resetStopped();playback=Playback{};activeTag=0;hasActive=false;} // Both sides quiescent.
    // Consumer only. Caller must ensure >=32768 MIDI bytes outside the callback.
    ComposerPlayerResult process(const PlaybackBlock& block,juce::MidiBuffer& midi)noexcept{
        midi.clear();
        if(block.samples==0){PlaybackResult r;r.nextPosition=block.position;return {r,activeTag,hasActive};}
        const auto view=mailbox.beginBlock();activeTag=view.tag;hasActive=view.song!=nullptr;
        PreparedMidiSink sink(midi);
        auto r=view.song?playback.render(*view.song,block,sink):playback.stop(sink);
        if(!view.song)r.nextPosition=std::isfinite(block.position)?block.position:0;
        return {r,activeTag,hasActive,sink.rejected()};
    }
private:
    PreparedMailbox mailbox;Playback playback;uint64_t activeTag=0;bool hasActive=false;
};
}
