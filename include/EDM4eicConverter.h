#pragma once

#include "NestDAQData.h"

#include <JANA/JEvent.h>

class EDM4eicConverter {
public:
    void fill_event(
        const NestDAQTimeFrameData& decoded,
        JEvent& event
    ) const;
};
