#include "RawHitProcessor.h"

#include <edm4eic/RawTrackerHit.h>
#include <edm4eic/RawTrackerHitCollection.h>

#include <iostream>

void RawHitProcessor::Process(
    const std::shared_ptr<const JEvent>& event
) {
    const auto* hits1 =
        event->GetCollection<edm4eic::RawTrackerHit>(
            "TOFBarrelADCTDC1"
        );
    const auto* hits2 =
        event->GetCollection<edm4eic::RawTrackerHit>(
            "TOFBarrelADCTDC2"
        );
    const auto* hits3 =
        event->GetCollection<edm4eic::RawTrackerHit>(
            "TOFBarrelADCTDC3"
        );

    std::cout
        << "[Processor]"
        << " event=" << event->GetEventNumber()
        << " TOF1=" << hits1->size()
        << " TOF2=" << hits2->size()
        << " TOF3=" << hits3->size()
        << std::endl;
}
