#pragma once
#include "Prepared.h"

namespace composer {
enum class PublishStatus {published,busy,budgetExceeded};
struct PublishedView {const PreparedSong* song=nullptr;uint64_t tag=0;bool changed=false;};

// One serialized producer and one non-reentrant consumer; destruction/reset require quiescence.
class PreparedMailbox {
public:
    explicit PreparedMailbox(size_t retainedBudget=limits::documentBytes)noexcept:budget(std::min(retainedBudget,limits::documentBytes)){}
    PreparedMailbox(const PreparedMailbox&)=delete;PreparedMailbox& operator=(const PreparedMailbox&)=delete;
    // Producer-only. A failed call does not move the candidate or alter active/pending data.
    PublishStatus publish(PreparedSong&& song,uint64_t tag)noexcept{
        reclaim();
        for(size_t index=0;index<slots.size();++index){
            auto& slot=slots[index];int expected=free;
            if(!slot.state.compare_exchange_strong(expected,writing,std::memory_order_acq_rel))continue;
            const size_t previous=slot.song?slot.song->storageBytes():0;
            const size_t remaining=retained-previous;
            if(song.storageBytes()>budget-remaining){slot.state.store(free,std::memory_order_release);return PublishStatus::budgetExceeded;}
            const auto bytes=song.storageBytes();slot.song.emplace(std::move(song));slot.tag=tag;retained=remaining+bytes;
            slot.state.store(ready,std::memory_order_release);
            const int old=pending.exchange(int(index),std::memory_order_acq_rel);
            if(old>=0)slots[size_t(old)].state.store(free,std::memory_order_release);
            return PublishStatus::published;
        }
        return PublishStatus::busy;
    }
    // Producer-only tombstone; old active storage remains until the consumer returns it.
    void clear()noexcept{
        const int old=pending.exchange(clearToken,std::memory_order_acq_rel);
        if(old>=0)slots[size_t(old)].state.store(free,std::memory_order_release);
        reclaim();
    }
    // Producer-only reclamation; the callback never resets an optional/vector.
    void reclaim()noexcept{
        for(auto& slot:slots){int expected=free;if(!slot.state.compare_exchange_strong(expected,writing,std::memory_order_acq_rel))continue;
            if(slot.song){retained-=slot.song->storageBytes();slot.song.reset();}
            slot.state.store(free,std::memory_order_release);
        }
    }
    size_t retainedBytes()const noexcept{return retained;} // Producer-only accounting.
    // Consumer-only; returned pointer is valid until the next beginBlock call.
    PublishedView beginBlock()noexcept{
        const int next=pending.exchange(noToken,std::memory_order_acq_rel);
        if(next!=noToken){
            const int old=active;active=next==clearToken?-1:next;
            if(active>=0)slots[size_t(active)].state.store(inUse,std::memory_order_release);
            if(old>=0)slots[size_t(old)].state.store(free,std::memory_order_release);
        }
        if(active<0)return {nullptr,0,next!=noToken};
        const auto& slot=slots[size_t(active)];return {&*slot.song,slot.tag,next!=noToken};
    }
    // Control/producer thread only after both producer and consumer have stopped.
    void resetStopped()noexcept{
        pending.store(noToken,std::memory_order_relaxed);active=-1;
        for(auto& slot:slots){slot.song.reset();slot.tag=0;slot.state.store(free,std::memory_order_release);}
        retained=0;
    }
private:
    static_assert(std::atomic<int>::is_always_lock_free);
    static constexpr int free=0,writing=1,ready=2,inUse=3,noToken=-1,clearToken=-2;
    struct Slot {std::optional<PreparedSong> song;uint64_t tag=0;std::atomic<int> state{free};};
    std::array<Slot,3> slots;std::atomic<int> pending{noToken};int active=-1;
    const size_t budget;size_t retained=0;
};
}
