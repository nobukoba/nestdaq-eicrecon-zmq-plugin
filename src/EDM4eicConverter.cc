#include "EDM4eicConverter.h"

#include <edm4eic/RawTrackerHitCollection.h>
#include <podio/Frame.h>

#include <iostream>
#include <memory>

void EDM4eicConverter::fill_event(
    const NestDAQTimeFrameData& decoded,
    JEvent& event
) const {
    auto out_hits1 =
        std::make_unique<edm4eic::RawTrackerHitCollection>();
    auto out_hits2 =
        std::make_unique<edm4eic::RawTrackerHitCollection>();
    auto out_hits3 =
        std::make_unique<edm4eic::RawTrackerHitCollection>();

    for (const auto& hit : decoded.tdc_hits) {
        if (hit.fem_id != 0xc0a80a29) {
            continue;
        }

        if (hit.channel <= 15) {
            out_hits1->create(
                hit.channel,
                hit.charge,
                hit.timestamp
            );
        } else if (hit.channel <= 31) {
            out_hits2->create(
                hit.channel - 16,
                hit.charge,
                hit.timestamp
            );
        } else if (hit.channel <= 47) {
            out_hits3->create(
                hit.channel - 32,
                hit.charge,
                hit.timestamp
            );
        }
    }

    auto frame = std::make_unique<podio::Frame>();

    frame->put(
        std::move(out_hits1),
        "TOFBarrelADCTDC1"
    );
    frame->put(
        std::move(out_hits2),
        "TOFBarrelADCTDC2"
    );
    frame->put(
        std::move(out_hits3),
        "TOFBarrelADCTDC3"
    );

    const auto& coll1 =
        frame->get<edm4eic::RawTrackerHitCollection>(
            "TOFBarrelADCTDC1"
        );
    const auto& coll2 =
        frame->get<edm4eic::RawTrackerHitCollection>(
            "TOFBarrelADCTDC2"
        );
    const auto& coll3 =
        frame->get<edm4eic::RawTrackerHitCollection>(
            "TOFBarrelADCTDC3"
        );

    event.InsertCollectionAlreadyInFrame<decltype(coll1[0])>(
        &coll1,
        "TOFBarrelADCTDC1"
    );
    event.InsertCollectionAlreadyInFrame<decltype(coll2[0])>(
        &coll2,
        "TOFBarrelADCTDC2"
    );
    event.InsertCollectionAlreadyInFrame<decltype(coll3[0])>(
        &coll3,
        "TOFBarrelADCTDC3"
    );

    std::cout
        << "[EDM4eic]"
        << " event=" << decoded.time_frame_id
        << " TOF1=" << coll1.size()
        << " TOF2=" << coll2.size()
        << " TOF3=" << coll3.size()
        << std::endl;

    event.Insert(frame.release());
}
