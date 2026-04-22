#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export LVGL_PIPELINE_NAME="pipeline.sh"
exec "${SCRIPT_DIR}/m1-pipeline.sh" "$@"
