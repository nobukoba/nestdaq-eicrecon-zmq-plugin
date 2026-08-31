#pragma once

#include <cstdint>
#include <vector>

struct NestDAQTdcHit {
    std::uint32_t fem_type = 0;
    std::uint32_t fem_id = 0;
    std::uint32_t channel = 0;
    std::int32_t charge = 0;
    std::int32_t timestamp = 0;
};

struct NestDAQTimeFrameData {
    std::uint64_t time_frame_id = 0;
    std::vector<NestDAQTdcHit> tdc_hits;
};
