#pragma once

#include "NestDAQData.h"

#include <cstddef>
#include <vector>

class NestDAQDecoder {
public:
    NestDAQTimeFrameData decode(
        const std::vector<std::vector<std::byte>>& parts
    ) const;
};
