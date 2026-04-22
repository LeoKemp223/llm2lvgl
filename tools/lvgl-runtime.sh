#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export LVGL_RUNTIME_NAME="lvgl-runtime.sh"
exec "${SCRIPT_DIR}/lvgl-m1-real.sh" "$@"
