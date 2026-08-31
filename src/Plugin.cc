#include "NestDAQZmqSource.h"
#include "RawHitProcessor.h"

#include <JANA/JApplication.h>

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
