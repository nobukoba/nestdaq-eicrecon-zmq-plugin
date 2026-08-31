#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>

#include <edm4eic/RawTrackerHitCollection.h>
#include <podio/Frame.h>

#include <zmq.h>

#include "TimeFrameHeader.h"
#include "SubTimeFrameHeader.h"
#include "HeartbeatFrameHeader.h"
#include "AmQStrTdcData.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class NestDAQZmqSource : public JEventSource {
public:
    explicit NestDAQZmqSource(std::string resource_name, JApplication* app)
        : JEventSource(std::move(resource_name), app) {
        SetCallbackStyle(
            jana::components::JComponent::CallbackStyle::ExpertMode
        );
    }

    ~NestDAQZmqSource() override {
        if (m_socket != nullptr) {
            zmq_close(m_socket);
            m_socket = nullptr;
        }
        if (m_context != nullptr) {
            zmq_ctx_term(m_context);
            m_context = nullptr;
        }
    }

    void Open() override {
        m_context = zmq_ctx_new();
        if (m_context == nullptr) {
            throw std::runtime_error(
                std::string("zmq_ctx_new failed: ") +
                zmq_strerror(zmq_errno())
            );
        }

        m_socket = zmq_socket(m_context, ZMQ_PULL);
        if (m_socket == nullptr) {
            throw std::runtime_error(
                std::string("zmq_socket failed: ") +
                zmq_strerror(zmq_errno())
            );
        }

        const int timeout_ms = 100;
        if (zmq_setsockopt(
                m_socket,
                ZMQ_RCVTIMEO,
                &timeout_ms,
                sizeof(timeout_ms)) != 0) {
            throw std::runtime_error(
                std::string("zmq_setsockopt failed: ") +
                zmq_strerror(zmq_errno())
            );
        }

        const char* host_env = std::getenv("NESTDAQ_HOST");
        const char* port_env = std::getenv("ZMQ_PORT");

        const std::string host =
            host_env != nullptr ? host_env : "127.0.0.1";
        const std::string port =
            port_env != nullptr ? port_env : "5501";

        m_address = "tcp://" + host + ":" + port;

        if (zmq_connect(m_socket, m_address.c_str()) != 0) {
            throw std::runtime_error(
                std::string("zmq_connect failed: ") +
                zmq_strerror(zmq_errno())
            );
        }

        std::cout
            << "[NestDAQ] ZMQ PULL connect: "
            << m_address
            << std::endl;
    }

    Result Emit(JEvent& event) override {
        std::vector<std::vector<std::byte>> parts;

        while (true) {
            zmq_msg_t msg;
            zmq_msg_init(&msg);

            const int received =
                zmq_msg_recv(&msg, m_socket, 0);

            if (received >= 0) {
                int debug_more = 0;
                size_t debug_more_size = sizeof(debug_more);
                zmq_getsockopt(
                    m_socket,
                    ZMQ_RCVMORE,
                    &debug_more,
                    &debug_more_size
                );

                std::cout
                    << "[ZMQ RECEIVE] "
                    << received
                    << " bytes"
                    << " more=" << debug_more
                    << std::endl;
            }

            if (received < 0) {
                const int err = zmq_errno();
                zmq_msg_close(&msg);

                if (err == EAGAIN) {
                    return Result::FailureTryAgain;
                }

                std::cerr
                    << "[NestDAQ] zmq_msg_recv failed: "
                    << zmq_strerror(err)
                    << std::endl;
                return Result::FailureTryAgain;
            }

            const std::size_t size = zmq_msg_size(&msg);
            const auto* data =
                static_cast<const std::byte*>(zmq_msg_data(&msg));

            parts.emplace_back(data, data + size);

            int more = 0;
            std::size_t more_size = sizeof(more);

            if (zmq_getsockopt(
                    m_socket,
                    ZMQ_RCVMORE,
                    &more,
                    &more_size) != 0) {
                const int err = zmq_errno();
                zmq_msg_close(&msg);

                std::cerr
                    << "[NestDAQ] ZMQ_RCVMORE failed: "
                    << zmq_strerror(err)
                    << std::endl;
                return Result::FailureTryAgain;
            }

            zmq_msg_close(&msg);

            if (!more) {
                break;
            }
        }

        auto out_hits1 =
            std::make_unique<edm4eic::RawTrackerHitCollection>();
        auto out_hits2 =
            std::make_unique<edm4eic::RawTrackerHitCollection>();
        auto out_hits3 =
            std::make_unique<edm4eic::RawTrackerHitCollection>();

        std::uint64_t time_frame_id = ++m_fallback_event_number;

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
                time_frame_id = header.timeFrameId;

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

            const std::size_t payload_offset =
                sizeof(HeartbeatFrame::Header);
            const std::size_t payload_size =
                part.size() - payload_offset;
            const std::size_t word_size =
                sizeof(AmQStrTdc::Data::Bits);
            const std::size_t nwords =
                payload_size / word_size;
            const auto* payload =
                part.data() + payload_offset;

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

                if (
                    current_stf.femType != 2 &&
                    current_stf.femType != 5
                ) {
                    continue;
                }

                if (current_stf.femId != 0xc0a80a29) {
                    continue;
                }

                const std::uint64_t channel = bits.hrch;
                const std::int32_t charge =
                    static_cast<std::int32_t>(bits.hrtot);
                const std::int32_t timestamp =
                    static_cast<std::int32_t>(bits.hrtdc);

                if (channel <= 15) {
                    out_hits1->create(channel, charge, timestamp);
                } else if (channel <= 31) {
                    out_hits2->create(channel - 16, charge, timestamp);
                } else if (channel <= 47) {
                    out_hits3->create(channel - 32, charge, timestamp);
                }
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

        const auto n1 = coll1.size();
        const auto n2 = coll2.size();
        const auto n3 = coll3.size();

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

        event.Insert(frame.release());
        event.SetRunNumber(1);
        event.SetEventNumber(time_frame_id);

        std::cout
            << "[NestDAQ -> JANA]"
            << " Event=" << time_frame_id
            << " TOF1=" << n1
            << " TOF2=" << n2
            << " TOF3=" << n3
            << std::endl;

        return Result::Success;
    }

private:
    void* m_context = nullptr;
    void* m_socket = nullptr;
    std::string m_address;
    std::uint64_t m_fallback_event_number = 0;
};

class RawHitProcessor : public JEventProcessor {
public:
    void Process(
        const std::shared_ptr<const JEvent>& event
    ) override {
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
};

extern "C"
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);

    app->Add(
        new NestDAQZmqSource(
            "nestdaq-zmq",
            app
        )
    );

    app->Add(new RawHitProcessor());
}
