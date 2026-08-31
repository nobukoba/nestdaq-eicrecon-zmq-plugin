#pragma once

#include <JANA/JEventProcessor.h>

class RawHitProcessor : public JEventProcessor {
public:
    void Process(
        const std::shared_ptr<const JEvent>& event
    ) override;
};
