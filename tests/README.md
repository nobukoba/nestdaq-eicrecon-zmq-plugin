# Decoder tests

The standalone decoder test does not require JANA or EICrecon:

```sh
c++ -std=c++20 -Iinclude src/NestDAQDecoder.cc tests/test_decoder.cc -o /tmp/test-decoder
/tmp/test-decoder
```

For the RARiS run 20 capture obtained from the SPADI integration tests, build
`tests/test_capture.cc` in the same way and provide the captured multipart
stream (a little-endian 32-bit length and more flag followed by each part):

```sh
c++ -std=c++20 -Iinclude src/NestDAQDecoder.cc tests/test_capture.cc -o /tmp/test-capture
/tmp/test-capture /path/to/capture.dat
```

The latter expects all 2,815 complete TFs and 68,956 raw hit words from the
three sources. It tests the decoder only; the current EDM converter selects one
FEM and must not be expected to output the same total.

Compare a RARiS capture and EICrecon log produced with `NESTDAQ_FEM_ID=0xc0a80a28`:

```sh
python3 tests/compare_capture_log.py capture.dat eicrecon.log
```
