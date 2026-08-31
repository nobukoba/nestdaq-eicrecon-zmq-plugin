#!/bin/bash
set -euo pipefail

export NESTDAQ_HOST="${NESTDAQ_HOST:-host.docker.internal}"
export ZMQ_PORT="${ZMQ_PORT:-5501}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/run-common.sh"
