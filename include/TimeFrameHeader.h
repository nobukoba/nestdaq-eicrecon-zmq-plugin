#ifndef TimeFrameHeader_h
#define TimeFrameHeader_h

#include <cstdint>

namespace TimeFrame {
namespace v0 {
constexpr uint64_t MAGIC {0x444145482d465440};
struct Header {
    uint64_t magic {MAGIC};
    uint32_t timeFrameId {0};
    uint32_t numSource {0};
    uint64_t length {0};
};
}
inline namespace v1 {
constexpr uint64_t MAGIC {0x004d5246454d4954};
constexpr uint16_t META {0x0001};
constexpr uint16_t SLICE {0x0002};
constexpr uint16_t TF_COMPLETE {0x0000};
constexpr uint16_t TF_INCOMPLETE {0x8000};
constexpr uint16_t TF_TIMEOUT {0x4000};
constexpr uint16_t TF_DEPTH_LIMIT {0x2000};
#pragma pack(2)
struct Header {
    uint64_t magic {MAGIC};
    uint32_t length {0};
    uint16_t hLength {24};
    uint16_t type {0};
    uint32_t timeFrameId {0};
    uint32_t numSource {0};
};
} // namespace v1
} // namespace TimeFrame
#endif
