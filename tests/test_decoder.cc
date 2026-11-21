#include "NestDAQDecoder.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "HeartbeatFrameHeader.h"
#include <cassert>
#include <cstring>
#include <vector>

template<class T> std::vector<std::byte> bytes(const T& value) {
    std::vector<std::byte> out(sizeof value); std::memcpy(out.data(), &value, sizeof value); return out;
}
int main() {
    TimeFrame::Header tf{}; tf.timeFrameId=42;
    SubTimeFrame::Header low{}; low.femType=6; low.femId=123;
    SubTimeFrame::Header high{}; high.femType=5; high.femId=456;
    HeartbeatFrame::Header h{};
    auto payload=[&](std::uint64_t word) { auto p=bytes(h); auto b=bytes(word); p.insert(p.end(),b.begin(),b.end()); return p; };
    std::uint64_t low_word=(std::uint64_t(11)<<58)|(std::uint64_t(12)<<50)|(std::uint64_t(34)<<34)|(std::uint64_t(56)<<15);
    std::uint64_t high_word=(std::uint64_t(11)<<58)|(std::uint64_t(13)<<51)|(std::uint64_t(35)<<29)|57;
    std::uint64_t trailer=std::uint64_t(13)<<58;
    auto result=NestDAQDecoder{}.decode({bytes(tf),bytes(low),payload(low_word),payload(trailer),bytes(high),payload(high_word)});
    assert(result.time_frame_id==42 && result.tdc_hits.size()==2);
    auto a=result.tdc_hits[0], b=result.tdc_hits[1];
    assert(a.fem_id==123 && a.channel==12 && a.charge==34 && a.timestamp==56);
    assert(b.fem_id==456 && b.channel==13 && b.charge==35 && b.timestamp==57);
}
