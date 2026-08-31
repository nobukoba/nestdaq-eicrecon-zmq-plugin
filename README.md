# nestdaq-eicrecon-zmq-plugin

JANA2/EICrecon plugin that receives NestDAQ `TimeFrameBuilder` output over ZeroMQ, decodes the NestDAQ multipart data, converts selected HRTDC hits into `edm4eic::RawTrackerHitCollection`, and passes those collections to downstream JANA2/EICrecon processors.

## Data flow

```text
TimeFrameBuilder
  PUSH / bind : tcp://127.0.0.1:5501
        |
        v
NestDAQZmqSource
  - ZeroMQ PULL/connect
  - receives one complete multipart TimeFrame
        |
        v
NestDAQDecoder
  - parses TF / STF / HBF headers
  - decodes AmQStrTdc words
  - produces NestDAQTimeFrameData
        |
        v
EDM4eicConverter
  - selects the HRTDC/FEM mapping
  - fills edm4eic::RawTrackerHitCollection
  - inserts collections into the JEvent/PODIO Frame
        |
        v
RawHitProcessor
  - example downstream analysis processor
  - reads only EDM4eic collections
```

The separation is intentional: the ZMQ transport layer does not know about detector mapping, the NestDAQ decoder does not depend on EDM4eic, and the analysis processor does not need to know anything about NestDAQ or ZeroMQ.

## Repository layout

```text
.
├── include/
│   ├── NestDAQZmqSource.h
│   ├── NestDAQDecoder.h
│   ├── NestDAQData.h
│   ├── EDM4eicConverter.h
│   ├── RawHitProcessor.h
│   ├── AmQStrTdcData.h
│   ├── HeartbeatFrameHeader.h
│   ├── SubTimeFrameHeader.h
│   └── TimeFrameHeader.h
├── src/
│   ├── NestDAQZmqSource.cc
│   ├── NestDAQDecoder.cc
│   ├── EDM4eicConverter.cc
│   ├── RawHitProcessor.cc
│   └── Plugin.cc
├── scripts/
│   ├── build.sh
│   ├── run-eicrecon-docker.sh
│   └── run-eicrecon-apptainer.sh
├── topology/
│   └── topology.sh
├── THIRD_PARTY_LICENSES.md
└── README.md
```

## Component responsibilities

### `NestDAQZmqSource`

Responsible only for the transport boundary and JANA event-source lifecycle.

It:

- opens a ZeroMQ `PULL` socket;
- connects to `NESTDAQ_HOST:ZMQ_PORT`;
- receives all parts of one ZeroMQ multipart message;
- passes the raw multipart buffers to `NestDAQDecoder`;
- passes decoded data to `EDM4eicConverter`;
- sets the JANA event number from the NestDAQ TimeFrame ID.

### `NestDAQDecoder`

Responsible only for decoding NestDAQ data structures.

It parses:

```text
TimeFrame::Header
SubTimeFrame::Header
HeartbeatFrame::Header
AmQStrTdc::Data::Bits
```

and produces the transport-independent intermediate representation:

```cpp
NestDAQTimeFrameData
```

The decoder does not create EDM4eic objects.

### `EDM4eicConverter`

Responsible for detector mapping and EDM4eic collection creation.

The current mapping follows the experimental NestDAQ `EDM4eicSink` HRTDC mapping. Only FEM ID

```text
0xc0a80a29
```

is converted into:

```text
HRTDC channel  0-15 -> TOFBarrelADCTDC1, cellID 0-15
HRTDC channel 16-31 -> TOFBarrelADCTDC2, cellID 0-15
HRTDC channel 32-47 -> TOFBarrelADCTDC3, cellID 0-15
```

The converter creates a PODIO `Frame`, inserts the three `edm4eic::RawTrackerHitCollection` objects, and registers them in the `JEvent`.

### `RawHitProcessor`

This is an example downstream analysis processor. It only accesses EDM4eic collections:

```cpp
const auto* hits =
    event->GetCollection<edm4eic::RawTrackerHit>(
        "TOFBarrelADCTDC1"
    );
```

Future calibration, clustering, tracking, or detector-specific reconstruction should be added after this boundary using normal JANA2/EICrecon factories/processors rather than adding analysis logic to the ZMQ source.

### `Plugin.cc`

Contains only JANA plugin registration:

```text
NestDAQZmqSource
RawHitProcessor
```

## Build

Clone the repository and enter the EIC software environment:

```bash
git clone https://github.com/nobukoba/nestdaq-eicrecon-zmq-plugin.git
cd nestdaq-eicrecon-zmq-plugin
```

Inside eic-shell:

```bash
./scripts/build.sh
```

The output is:

```text
build/nestdaq_zmq_source.so
```

The plugin links against JANA2, EDM4eic, PODIO, ROOT, and libzmq from the EIC software environment.

## macOS / Docker

Start eic-shell, build, then run:

```bash
~/eic/eic-shell
./scripts/build.sh
./scripts/run-eicrecon-docker.sh
```

The Docker launcher defaults to:

```text
tcp://host.docker.internal:5501
```

Override with:

```bash
NESTDAQ_HOST=host.docker.internal ZMQ_PORT=5501 ./scripts/run-eicrecon-docker.sh
```

## Linux / Apptainer

Inside the EIC Apptainer environment:

```bash
./scripts/build.sh
./scripts/run-eicrecon-apptainer.sh
```

The Apptainer launcher defaults to:

```text
tcp://127.0.0.1:5501
```

This assumes Apptainer is using the host network namespace and was not started with `--net`.

Override with:

```bash
NESTDAQ_HOST=127.0.0.1 ZMQ_PORT=5501 ./scripts/run-eicrecon-apptainer.sh
```

## NestDAQ topology

The important output endpoint is:

```bash
endpoint TimeFrameBuilder out \
    type push \
    method bind \
    enable-uds false \
    portRangeMin 5501 \
    portRangeMax 5501
```

The upstream input side is normally:

```bash
endpoint STFBFilePlayer out \
    type push \
    method connect \
    autoSubChannel true

endpoint TimeFrameBuilder in \
    type pull \
    method bind \
    enable-uds false \
    portRangeMin 5500 \
    portRangeMax 5500

link STFBFilePlayer out TimeFrameBuilder in
```

A complete example is in `topology/topology.sh`.

## Runtime diagnostics

On startup:

```text
[NestDAQ] ZMQ PULL connect: tcp://host.docker.internal:5501
```

For each ZeroMQ multipart part:

```text
[ZMQ RECEIVE] 1234 bytes more=1
```

Decoder diagnostics are separated by layer:

```text
[TF] ...
[STF] ...
[HBF] ...
```

After EDM4eic conversion:

```text
[EDM4eic] event=100212 TOF1=... TOF2=... TOF3=...
```

The example downstream processor prints:

```text
[Processor] event=100212 TOF1=... TOF2=... TOF3=...
```

This makes it easier to identify whether a problem is in ZeroMQ reception, NestDAQ decoding, EDM4eic conversion, or downstream analysis.

## Third-party headers

The NestDAQ data-format headers under `include/` are derived from `spadi-alliance/nestdaq-user-impl` and are included so the plugin can be built without a separate NestDAQ source checkout. See `THIRD_PARTY_LICENSES.md`.
