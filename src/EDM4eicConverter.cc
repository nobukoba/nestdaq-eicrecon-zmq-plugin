#include "EDM4eicConverter.h"

#include <edm4eic/RawTrackerHitCollection.h>
#include <podio/Frame.h>

#include <iostream>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

EDM4eicConverter::EDM4eicConverter() : m_fem_id(0xc0a80a29) {
    const char* value = std::getenv("NESTDAQ_FEM_ID");
    if (!value) return;
    const std::string text(value);
    if (text.empty() || text[0] == '-' || text[0] == '+')
        throw std::invalid_argument("NESTDAQ_FEM_ID must be a decimal or 0x-prefixed 32-bit integer");
    std::size_t end = 0;
    const auto id = std::stoull(text, &end, 0);
    if (end != text.size() || id > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("NESTDAQ_FEM_ID must be a decimal or 0x-prefixed 32-bit integer");
    m_fem_id = static_cast<std::uint32_t>(id);
}

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
        if (hit.fem_id != m_fem_id) {
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
