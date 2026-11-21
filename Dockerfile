# The selected EIC software release is fixed by digest for repeatable builds.
FROM eicweb/eic_xl@sha256:37e7fb527c8b3421bf2d353f4fc3146b1efb07a3a07ebd01bbced8417d29add3
WORKDIR /opt/nestdaq-eicrecon-zmq-plugin
COPY include/ include/
COPY src/ src/
COPY scripts/ scripts/
RUN ./scripts/build.sh
ENTRYPOINT ["/opt/nestdaq-eicrecon-zmq-plugin/scripts/run-eicrecon-apptainer.sh"]
