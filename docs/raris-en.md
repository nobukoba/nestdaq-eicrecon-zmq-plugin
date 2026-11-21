# Connecting SPADI and EICrecon in separate containers

The SPADI RARiS recipe sends TimeFrameBuilder output through a ZeroMQ PUSH
socket; this plugin receives it with PULL. Use the RARiS recipe in
`spadi-containers-second-trial` on the sending side. The SPADI changes used for
validation were merged into `main` in [PR #8](https://github.com/nobukoba/spadi-containers-second-trial/pull/8).

Start an existing EIC image, mount this repository, and compile the plugin in
that environment. The resulting `build/nestdaq_zmq_source.so` is saved in the
mounted working directory.

## Preparation

On the host, clone the repository:

```sh
git clone https://github.com/nobukoba/nestdaq-eicrecon-zmq-plugin.git
cd nestdaq-eicrecon-zmq-plugin
```

The EIC environment was tested with `eicweb/eic_xl:26.11.0-stable`. Use a
login shell (`bash -l`) inside the container to load its environment. Rebuild
the plugin when changing the EIC version.

## Docker

Start the SPADI container named `spadi-raris` and run the RARiS recipe's
`run-start.sh`. In a second host shell, start the EIC container:

```sh
docker run --rm -it --network container:spadi-raris \
  -v "$PWD:/work" -w /work --entrypoint /bin/bash \
  eicweb/eic_xl:26.11.0-stable -l
```

Build and start the receiver inside the container:

```sh
./scripts/build.sh
export NESTDAQ_HOST=127.0.0.1 ZMQ_PORT=5501
export NESTDAQ_FEM_ID=0xc0a80a28
./scripts/run-eicrecon-apptainer.sh
```

The two containers share a network namespace via `--network container:spadi-raris`,
so the receiver can reach the sender bound to `127.0.0.1:5501`. Linux host
networking is another option. Separate Docker network namespaces have separate
loopback addresses and cannot directly reach a sender bound to its loopback.

## Apptainer

Obtain an EIC SIF and start it from the repository on the host:

```sh
apptainer shell --bind "$PWD:/work" --pwd /work /path/to/eic_xl.sif
bash -l
cd /work
./scripts/build.sh
export NESTDAQ_HOST=127.0.0.1 ZMQ_PORT=5501
export NESTDAQ_FEM_ID=0xc0a80a28
./scripts/run-eicrecon-apptainer.sh
```

Apptainer normally shares the host network. If using `--net`, configure an
endpoint reachable from the receiver.

## Replay and results

Start the receiver before using SPADI web controls INIT DEVICE, INIT TASK, and
RUN. The FEM selection defaults to `0xc0a80a29`; RARiS run 20 has hits on
`0xc0a80a28`. The converter maps only channels 0–47 of the selected FEM into
the existing three `TOFBarrelADCTDC` collections. The collection names and
example cell IDs are not validated RARiS detector geometry or calibration.

With a 5 ms replay delay, run 20 produced 2,815 TFs and 68,956 hits; all
per-event collection counts matched the recorded replay stream. PUSH/PULL
distributes messages between receivers and does not duplicate them. Run
ARTEMIS and EICrecon separately when comparing their results.
