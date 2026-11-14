#include "../Source/engine/Publication.h"
#include "AllocationGuard.h"
#include <thread>
#include <iostream>
#include <cstdlib>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
PreparedSong song(uint64_t tag){
    Composition c;c.id="publication";c.endTick={480+int64_t(tag%4)*480};c.tempos={{{0},500000}};c.meters={{{0},4,4}};c.keys={{{0},0,Scale::major}};
    Track t;t.id=1;t.name="Track";t.notes={{2,int(tag%128),int(tag%127)+1,{0},{480}}};c.tracks={t};auto result=prepareComposition(c);check(result.song);return std::move(*result.song);
}
void coherent(PublishedView view){if(view.song){check(view.song->endTick().value==480+int64_t(view.tag%4)*480);auto q=view.song->chase({0});check(q.noteCount==1&&q.notes[0].key==view.tag%128&&q.notes[0].velocity==view.tag%127+1);}}
int main(){
    PreparedMailbox mailbox;auto view=mailbox.beginBlock();check(!view.song&&!view.changed&&view.tag==0);
    auto first=song(1);check(mailbox.publish(std::move(first),1)==PublishStatus::published);auto second=song(2);check(mailbox.publish(std::move(second),2)==PublishStatus::published);
    allocationGuard::allocations=allocationGuard::frees=0;allocationGuard::active=true;view=mailbox.beginBlock();coherent(view);allocationGuard::active=false;
    check(view.changed&&view.tag==2&&allocationGuard::allocations==0&&allocationGuard::frees==0);auto stable=view.song;
    view=mailbox.beginBlock();check(view.song==stable&&!view.changed);auto third=song(3);check(mailbox.publish(std::move(third),3)==PublishStatus::published);coherent(view);
    allocationGuard::active=true;view=mailbox.beginBlock();coherent(view);allocationGuard::active=false;check(view.tag==3&&allocationGuard::allocations==0&&allocationGuard::frees==0);
    auto before=mailbox.retainedBytes();allocationGuard::active=true;mailbox.reclaim();allocationGuard::active=false;check(mailbox.retainedBytes()<before&&mailbox.retainedBytes()==view.song->storageBytes());check(allocationGuard::allocations==0&&allocationGuard::frees>0);allocationGuard::frees=0;
    mailbox.clear();allocationGuard::active=true;view=mailbox.beginBlock();allocationGuard::active=false;check(!view.song&&view.changed&&allocationGuard::frees==0);mailbox.reclaim();check(mailbox.retainedBytes()==0);
    auto four=song(4);check(mailbox.publish(std::move(four),4)==PublishStatus::published);mailbox.clear();auto five=song(5);check(mailbox.publish(std::move(five),5)==PublishStatus::published);view=mailbox.beginBlock();check(view.song&&view.tag==5);coherent(view);
    mailbox.resetStopped();check(mailbox.retainedBytes()==0&&!mailbox.beginBlock().song);
    auto limited=song(1);auto bytes=limited.storageBytes();PreparedMailbox small(bytes);check(small.publish(std::move(limited),1)==PublishStatus::published);auto active=small.beginBlock();
    auto candidate=song(2);check(small.publish(std::move(candidate),2)==PublishStatus::budgetExceeded);check(candidate.chase({0}).notes[0].key==2&&small.beginBlock().song==active.song&&small.retainedBytes()==bytes);
    small.clear();small.beginBlock();small.reclaim();check(small.publish(std::move(candidate),2)==PublishStatus::published);check(small.beginBlock().tag==2);
    PreparedMailbox concurrent;std::atomic<bool> done=false;std::atomic<uint64_t> observed=0;
    std::thread consumer([&]{uint64_t last=0;allocationGuard::allocations=allocationGuard::frees=0;allocationGuard::active=true;
        while(!done.load(std::memory_order_acquire)){auto current=concurrent.beginBlock();coherent(current);if(current.song){check(current.tag>=last);last=current.tag;}}
        auto current=concurrent.beginBlock();coherent(current);if(current.song)last=current.tag;allocationGuard::active=false;check(allocationGuard::allocations==0&&allocationGuard::frees==0);observed=last;
    });
    std::thread producer([&]{for(uint64_t tag=1;tag<=20000;++tag){auto next=song(tag);PublishStatus status;do{status=concurrent.publish(std::move(next),tag);if(status==PublishStatus::busy)std::this_thread::yield();}while(status==PublishStatus::busy);check(status==PublishStatus::published);if(tag%7==0)concurrent.clear();}done.store(true,std::memory_order_release);});
    producer.join();consumer.join();check(observed==20000);concurrent.resetStopped();check(concurrent.retainedBytes()==0);
    std::cout<<"three-slot latest/clear/budget/reclaim and20000concurrent tagged publications; no consumer new/delete passed\n";
}
