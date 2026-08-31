#ifndef AmQStrTdcData_h
#define AmQStrTdcData_h

#include <cstdint>

namespace AmQStrTdc::Data {
struct Word { uint8_t d[8]; };
namespace v0 {
struct Bits {
    union {
        uint8_t d[8];
        struct { uint64_t raw : 64; };
        struct { uint64_t com_rsv : 58; uint64_t head : 6; };
        struct { uint64_t zero_t1 : 16; uint64_t tdc : 19; uint64_t tot : 16; uint64_t ch : 7; uint64_t dtype : 6; };
        struct { uint64_t hrtdc : 29; uint64_t hrtot : 22; uint64_t hrch : 7; uint64_t hrdtype : 6; };
        struct { uint64_t zero_h : 24; uint64_t hbframe : 16; uint64_t hbspilln : 8; uint64_t hbflag : 10; uint64_t htype : 6; };
        struct { uint64_t zero_s : 24; uint64_t hbfrco : 16; uint64_t spilln : 8; uint64_t spflag : 10; uint64_t stype : 6; };
    };
};
enum HeadTypes { Data=0x0B, Heartbeat=0x1C, Trailer=0x0D, SpillOn=0x18, SpillEnd=0x14 };
} // namespace v0

inline namespace v1 {
struct Bits {
    union {
        uint8_t d[8];
        struct { uint64_t raw : 64; };
        struct { uint64_t com_rsv : 58; uint64_t head : 6; };
        struct { uint64_t zero_t1 : 15; uint64_t tdc : 19; uint64_t tot : 16; uint64_t ch : 8; uint64_t dtype : 6; };
        struct { uint64_t hrtdc : 29; uint64_t hrtot : 22; uint64_t hrch : 7; uint64_t hrdtype : 6; };
        struct { uint64_t hbframe : 24; uint64_t toffset : 16; uint64_t hbflag : 16; uint64_t reserve1 : 2; uint64_t hbtype1 : 6; };
        struct { uint64_t transSize : 20; uint64_t geneSize : 20; uint64_t userReg : 16; uint64_t reserve2 : 2; uint64_t hbtype2 : 6; };
    };
};
enum HeadTypes {
    Data=0x0B,
    Heartbeat=0x1C,
    Heartbeat2nd=0x1E,
    Trailer=0x0D,
    ThrottlingT1Start=0x19,
    ThrottlingT1End=0x11,
    ThrottlingT2Start=0x1A,
    ThrottlingT2End=0x12,
    SpillOn=0x18,
    SpillEnd=0x14
};
} // namespace v1
} // namespace AmQStrTdc::Data
#endif
