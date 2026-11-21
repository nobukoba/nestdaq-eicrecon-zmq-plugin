# Connecting SPADI and EICrecon in separate containers

The SPADI RARiS recipe sends TimeFrameBuilder output through a ZeroMQ PUSH
socket; this plugin receives it with PULL. Use the RARiS recipe in
`spadi-containers-second-trial` on the sending side.

A Linux Apptainer test used separate EIC `26.11.0-stable` and SPADI
containers with a 5 ms wait: 2,815 TFs and 68,956 hits were received, and
all per-event collection counts matched the recorded replay stream.

When both containers share the Linux host network, connect to `127.0.0.1:5501`.
Apptainer normally shares the host network. Separate Docker network namespaces
have different loopback addresses: use host networking or a reachable address
and port. A sender bound to `127.0.0.1` is not reachable from another network.

Build this repository inside the EIC environment:

```sh
./scripts/build.sh
export NESTDAQ_HOST=127.0.0.1 ZMQ_PORT=5501
export NESTDAQ_FEM_ID=0xc0a80a28
./scripts/run-eicrecon-apptainer.sh
```

Start the receiver before using the SPADI web controls INIT DEVICE, INIT TASK,
and RUN. The FEM selection defaults to the previous `0xc0a80a29`; RARiS run 20
has hits on `0xc0a80a28`. The converter maps only channels 0–47 of the selected
FEM into the existing three `TOFBarrelADCTDC` collections. The collection names
and example cell IDs are not validated RARiS detector geometry or calibration.

PUSH/PULL distributes messages between receivers; it does not duplicate them.
Run ARTEMIS and EICrecon separately when comparing their results.

To compile the plugin once into a Docker image, run from this repository:

```sh
docker build -t nestdaq-eicrecon:local .
docker run --rm --network host -e NESTDAQ_FEM_ID=0xc0a80a28 nestdaq-eicrecon:local
```

This uses Linux host networking. Alternatively, if the SPADI Docker container
is named `spadi-raris`, share its network namespace while keeping separate
containers:

```sh
docker run --rm --network container:spadi-raris \
  -e NESTDAQ_FEM_ID=0xc0a80a28 nestdaq-eicrecon:local
```

This was tested on Docker Desktop with compiler calls blocked: the built image
received 2,815 TFs and 68,956 hits, with matching per-event counts. Converting
the built image to SIF has not yet been verified.
