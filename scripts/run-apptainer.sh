#!/bin/bash
set -euo pipefail

export NESTDAQ_HOST="${NESTDAQ_HOST:-127.0.0.1}"
export ZMQ_PORT="${ZMQ_PORT:-5501}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/run-common.sh"
