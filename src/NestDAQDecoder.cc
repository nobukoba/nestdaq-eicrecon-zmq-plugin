#include "NestDAQDecoder.h"

#include "AmQStrTdcData.h"
#include "HeartbeatFrameHeader.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"

#include <cstring>
#include <iomanip>
#include <iostream>

NestDAQTimeFrameData NestDAQDecoder::decode(
    const std::vector<std::vector<std::byte>>& parts
) const {
    NestDAQTimeFrameData decoded;

    SubTimeFrame::Header current_stf{};
    bool have_stf = false;

    for (const auto& part : parts) {
        if (part.size() < sizeof(std::uint64_t)) {
            continue;
        }

        std::uint64_t magic = 0;
        std::memcpy(&magic, part.data(), sizeof(magic));

        if (magic == TimeFrame::MAGIC) {
            if (part.size() < sizeof(TimeFrame::Header)) {
                continue;
            }

            TimeFrame::Header header{};
            std::memcpy(&header, part.data(), sizeof(header));
            decoded.time_frame_id = header.timeFrameId;

            std::cout
                << "[TF]"
                << " id=" << header.timeFrameId
                << " length=" << header.length
                << " type=" << header.type
                << " numSource=" << header.numSource
                << std::endl;
            continue;
        }

        if (magic == SubTimeFrame::MAGIC) {
            if (part.size() < sizeof(SubTimeFrame::Header)) {
                continue;
            }

            std::memcpy(
                &current_stf,
                part.data(),
                sizeof(current_stf)
            );
            have_stf = true;

            std::cout
                << "[STF]"
                << " tfid=" << current_stf.timeFrameId
                << " femType=" << current_stf.femType
                << " femId=0x" << std::hex << current_stf.femId
                << std::dec
                << " length=" << current_stf.length
                << std::endl;
            continue;
        }

        if (magic != HeartbeatFrame::MAGIC || !have_stf) {
            continue;
        }

        if (part.size() < sizeof(HeartbeatFrame::Header)) {
            continue;
        }

        const std::size_t payload_offset = sizeof(HeartbeatFrame::Header);
        const std::size_t payload_size = part.size() - payload_offset;
        const std::size_t word_size = sizeof(AmQStrTdc::Data::Bits);
        const std::size_t nwords = payload_size / word_size;
        const auto* payload = part.data() + payload_offset;

        std::cout
            << "[HBF]"
            << " femType=" << current_stf.femType
            << " femId=0x" << std::hex << current_stf.femId
            << std::dec
            << " words=" << nwords
            << std::endl;

        for (std::size_t i = 0; i < nwords; ++i) {
            AmQStrTdc::Data::Bits bits{};
            std::memcpy(
                &bits,
                payload + i * word_size,
                word_size
            );

            const bool is_tdc_word =
                bits.head == AmQStrTdc::Data::Data ||
                bits.head == AmQStrTdc::Data::Trailer ||
                bits.head == AmQStrTdc::Data::ThrottlingT1Start ||
                bits.head == AmQStrTdc::Data::ThrottlingT1End ||
                bits.head == AmQStrTdc::Data::ThrottlingT2Start ||
                bits.head == AmQStrTdc::Data::ThrottlingT2End;

            if (!is_tdc_word) {
                continue;
            }

            NestDAQTdcHit hit;
            hit.fem_type = current_stf.femType;
            hit.fem_id = current_stf.femId;

            if (
                current_stf.femType == 2 ||
                current_stf.femType == 5
            ) {
                hit.channel = bits.hrch;
                hit.charge = static_cast<std::int32_t>(bits.hrtot);
                hit.timestamp = static_cast<std::int32_t>(bits.hrtdc);
                decoded.tdc_hits.push_back(hit);
            }
        }
    }

    return decoded;
}
