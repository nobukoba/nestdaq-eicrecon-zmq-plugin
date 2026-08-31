Source modules are intentionally separated by responsibility:

- `NestDAQZmqSource.cc`: ZeroMQ transport and JANA event-source lifecycle.
- `NestDAQDecoder.cc`: NestDAQ TF/STF/HBF and TDC decoding into an intermediate data model.
- `EDM4eicConverter.cc`: detector mapping and conversion into EDM4eic/PODIO collections.
- `RawHitProcessor.cc`: example downstream analysis using only EDM4eic collections.
- `Plugin.cc`: JANA plugin registration only.
