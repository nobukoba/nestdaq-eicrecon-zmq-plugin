#!/bin/bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLUGIN_DIR="${REPO_ROOT}/build"

export NESTDAQ_HOST="${NESTDAQ_HOST:-host.docker.internal}"
export ZMQ_PORT="${ZMQ_PORT:-5501}"
export JANA_PLUGIN_PATH="${PLUGIN_DIR}:${JANA_PLUGIN_PATH:-}"

echo "NestDAQ endpoint: tcp://${NESTDAQ_HOST}:${ZMQ_PORT}"
echo "JANA_PLUGIN_PATH: ${JANA_PLUGIN_PATH}"

eicrecon \
  -Pplugins=nestdaq_zmq_source \
  -Pplugins_to_ignore=log,dd4hep,evaluator,acts,algorithms_init,pid_lut,richgeo,rootfile,beam,reco,tracking,pid,global_pid_lut,EEMC,BEMC,FEMC,EHCAL,BHCAL,FHCAL,B0ECAL,ZDC,BTRK,BVTX,PFRICH,DIRC,DRICH,ECTRK,MPGD,B0TRK,RPOTS,FOFFMTRK,BTOF,ECTOF,LOWQ2,LUMISPECCAL,podio,janatop,particle_flow \
  -Pnthreads=1 \
  -Pjana:loglevel=WARN
