#pragma once

#include "NestDAQData.h"

#include <JANA/JEvent.h>

class EDM4eicConverter {
public:
    EDM4eicConverter();
    void fill_event(
        const NestDAQTimeFrameData& decoded,
        JEvent& event
    ) const;
private:
    std::uint32_t m_fem_id;
};
