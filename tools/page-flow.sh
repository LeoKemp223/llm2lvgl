#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export LVGL_PAGE_FLOW_NAME="page-flow.sh"
exec "${SCRIPT_DIR}/m1-page-flow.sh" "$@"
