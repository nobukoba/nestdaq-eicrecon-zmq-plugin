#pragma once

#include "EDM4eicConverter.h"
#include "NestDAQDecoder.h"

#include <JANA/JApplication.h>
#include <JANA/JEventSource.h>

#include <cstdint>
#include <string>

class NestDAQZmqSource : public JEventSource {
public:
    explicit NestDAQZmqSource(
        std::string resource_name,
        JApplication* app
    );

    ~NestDAQZmqSource() override;

    void Open() override;
    Result Emit(JEvent& event) override;

private:
    void* m_context = nullptr;
    void* m_socket = nullptr;
    std::string m_address;
    std::uint64_t m_fallback_event_number = 0;

    NestDAQDecoder m_decoder;
    EDM4eicConverter m_converter;
};
