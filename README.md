# nestdaq-eicrecon-zmq-plugin

JANA2/EICrecon plugin that receives NestDAQ `TimeFrameBuilder` output over ZeroMQ and exposes decoded hits as `edm4eic::RawTrackerHitCollection` objects inside a `JEvent`.

## Data flow

```text
STFBFilePlayer
    PUSH
      |
      v
TimeFrameBuilder in   tcp://127.0.0.1:5500
TimeFrameBuilder out  tcp://127.0.0.1:5501  (PUSH / bind)
      |
      v
NestDAQZmqSource      (PULL / connect)
      |
      v
edm4eic::RawTrackerHitCollection
      |
      v
JANA2 / EICrecon factories and processors
```

The current default output port from `TimeFrameBuilder` is **5501**.

## Repository layout

```text
.
├── include/
│   ├── AmQStrTdcData.h
│   ├── HeartbeatFrameHeader.h
│   ├── SubTimeFrameHeader.h
│   └── TimeFrameHeader.h
├── scripts/
│   ├── build.sh
│   ├── run-common.sh
│   ├── run-docker.sh
│   ├── run-apptainer.sh
│   └── run.sh
├── src/
│   └── nestdaq_zmq_source.cc
├── topology/
│   └── topology.sh
├── THIRD_PARTY_LICENSES.md
└── README.md
```

## 1. macOS / Docker / eic-shell

Install eic-shell:

```bash
mkdir -p ~/eic
cd ~/eic
curl --location https://get.epic-eic.org | bash
```

Clone this repository:

```bash
cd ~/eic
git clone https://github.com/nobukoba/nestdaq-eicrecon-zmq-plugin.git
cd nestdaq-eicrecon-zmq-plugin
```

Enter eic-shell:

```bash
~/eic/eic-shell
```

Build the plugin inside eic-shell:

```bash
./scripts/build.sh
```

Run it:

```bash
./scripts/run-docker.sh
```

The Docker launcher uses this endpoint by default:

```text
tcp://host.docker.internal:5501
```

This is required when `TimeFrameBuilder` runs on the macOS host and EICrecon runs inside Docker.

To override the host or port:

```bash
NESTDAQ_HOST=host.docker.internal ZMQ_PORT=5501 ./scripts/run-docker.sh
```

## 2. Linux / Apptainer

Clone the repository on the Linux host:

```bash
git clone https://github.com/nobukoba/nestdaq-eicrecon-zmq-plugin.git
cd nestdaq-eicrecon-zmq-plugin
```

Enter the EIC Apptainer environment, then build:

```bash
./scripts/build.sh
```

Run:

```bash
./scripts/run-apptainer.sh
```

The Apptainer launcher uses this endpoint by default:

```text
tcp://127.0.0.1:5501
```

This assumes Apptainer is using the host network namespace, which is the normal behavior when it is started without `--net`.

To override:

```bash
NESTDAQ_HOST=127.0.0.1 ZMQ_PORT=5501 ./scripts/run-apptainer.sh
```

## 3. NestDAQ topology

The important endpoint is the `TimeFrameBuilder` output:

```bash
endpoint TimeFrameBuilder out \
    type push \
    method bind \
    enable-uds false \
    portRangeMin 5501 \
    portRangeMax 5501
```

`STFBFilePlayer` must also be linked to `TimeFrameBuilder in`:

```bash
endpoint STFBFilePlayer out \
    type push \
    method connect \
    autoSubChannel true

endpoint TimeFrameBuilder in \
    type pull \
    method bind

link STFBFilePlayer out TimeFrameBuilder in
```

A complete example is provided in:

```text
topology/topology.sh
```

When the topology is correct, the TimeFrameBuilder log should contain something like:

```text
Attached channel in[0]  to tcp://127.0.0.1:5500 (bind) (pull)
Attached channel out[0] to tcp://127.0.0.1:5501 (bind) (push)
```

## 4. Build output

After a successful build:

```text
build/nestdaq_zmq_source.so
```

The build links against JANA2, EDM4eic, PODIO, ROOT, and libzmq from the EIC software environment.

## 5. Runtime output

On startup, the source prints the endpoint, for example:

```text
[NestDAQ] ZMQ PULL connect: tcp://host.docker.internal:5501
```

This debug version also prints each received ZeroMQ multipart part:

```text
[ZMQ RECEIVE] 1234 bytes more=1
```

If the TCP connection is established but no `[ZMQ RECEIVE]` line appears, `TimeFrameBuilder` has not sent a message yet.

## 6. Data mapping

The source follows the HRTDC mapping used by the experimental NestDAQ `EDM4eicSink` implementation.

Only HRTDC data from FEM ID:

```text
0xc0a80a29
```

and FEM types `2` or `5` are converted to the three output collections:

```text
HRTDC channel  0-15 -> TOFBarrelADCTDC1, cellID 0-15
HRTDC channel 16-31 -> TOFBarrelADCTDC2, cellID 0-15
HRTDC channel 32-47 -> TOFBarrelADCTDC3, cellID 0-15
```

A downstream JANA2/EICrecon component can retrieve a collection with:

```cpp
const auto* hits =
    event->GetCollection<edm4eic::RawTrackerHit>(
        "TOFBarrelADCTDC1"
    );
```

The intended processing model is:

```text
NestDAQZmqSource
      -> RawTrackerHitCollection
      -> JANA2/EICrecon Factory
      -> reconstructed/calibrated collection
      -> next Factory or Processor
```

## 7. Troubleshooting

Check that `TimeFrameBuilder` is listening on 5501:

```bash
ss -ltnp | grep 5501
```

Check for an established connection:

```bash
ss -tnp | grep 5501
```

For Docker on macOS, `127.0.0.1` inside the container is the container itself. Use:

```text
host.docker.internal:5501
```

For ordinary Apptainer without `--net`, use:

```text
127.0.0.1:5501
```

## Third-party headers

The NestDAQ data-format headers under `include/` are derived from `spadi-alliance/nestdaq-user-impl` and are included so the plugin can be built without a separate NestDAQ source checkout. See `THIRD_PARTY_LICENSES.md`.
