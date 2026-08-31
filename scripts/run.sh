#!/bin/bash
set -euo pipefail

cat <<'EOF'
Choose the runtime-specific launcher:

  Docker / macOS eic-shell:
    ./scripts/run-docker.sh

  Apptainer / Singularity:
    ./scripts/run-apptainer.sh
EOF
