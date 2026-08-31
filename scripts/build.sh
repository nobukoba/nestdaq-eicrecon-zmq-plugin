#!/bin/bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${REPO_ROOT}/src"
BUILD_DIR="${REPO_ROOT}/build"
INCLUDE_DIR="${REPO_ROOT}/include"

mkdir -p "${BUILD_DIR}"

c++ \
  "${SRC_DIR}/NestDAQZmqSource.cc" \
  "${SRC_DIR}/NestDAQDecoder.cc" \
  "${SRC_DIR}/EDM4eicConverter.cc" \
  "${SRC_DIR}/RawHitProcessor.cc" \
  "${SRC_DIR}/Plugin.cc" \
  -std=c++20 \
  -fPIC \
  -shared \
  -I/opt/local/include \
  -I"${INCLUDE_DIR}" \
  $(root-config --cflags) \
  -L/opt/local/lib \
  -Wl,-rpath,/opt/local/lib \
  -lJANA \
  -ledm4eic \
  -lpodio \
  $(root-config --libs) \
  $(pkg-config --cflags --libs libzmq) \
  -pthread \
  -o "${BUILD_DIR}/nestdaq_zmq_source.so"

echo "Built: ${BUILD_DIR}/nestdaq_zmq_source.so"

echo
echo "Linked libraries:"
if command -v ldd >/dev/null 2>&1; then
  ldd "${BUILD_DIR}/nestdaq_zmq_source.so" | grep -E 'JANA|edm4eic|podio|zmq' || true
fi
