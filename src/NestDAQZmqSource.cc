#include "NestDAQZmqSource.h"

#include <zmq.h>

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

NestDAQZmqSource::NestDAQZmqSource(
    std::string resource_name,
    JApplication* app
)
    : JEventSource(std::move(resource_name), app) {
    SetCallbackStyle(
        jana::components::JComponent::CallbackStyle::ExpertMode
    );
}

NestDAQZmqSource::~NestDAQZmqSource() {
    if (m_socket != nullptr) {
        zmq_close(m_socket);
        m_socket = nullptr;
    }
    if (m_context != nullptr) {
        zmq_ctx_term(m_context);
        m_context = nullptr;
    }
}

void NestDAQZmqSource::Open() {
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

JEventSource::Result NestDAQZmqSource::Emit(JEvent& event) {
    std::vector<std::vector<std::byte>> parts;

    while (true) {
        zmq_msg_t msg;
        zmq_msg_init(&msg);

        const int received = zmq_msg_recv(&msg, m_socket, 0);

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

        std::cout
            << "[ZMQ RECEIVE] "
            << size
            << " bytes"
            << " more=" << more
            << std::endl;

        zmq_msg_close(&msg);

        if (!more) {
            break;
        }
    }

    auto decoded = m_decoder.decode(parts);

    if (decoded.time_frame_id == 0) {
        decoded.time_frame_id = ++m_fallback_event_number;
    }

    event.SetRunNumber(1);
    event.SetEventNumber(decoded.time_frame_id);

    m_converter.fill_event(decoded, event);

    return Result::Success;
}
