#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

task_legacy_ref() {
    local task_path="$1"
    python3 -c '
import json
import sys
from pathlib import Path

task = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
value = task.get("compat", {}).get("legacy_page_flow_task")
print("" if value is None else value)
' "${task_path}"
}

task_is_legacy_compat() {
    local task_path="$1"
    python3 -c '
import json
import sys
from pathlib import Path

task = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
print("true" if task.get("compat", {}).get("legacy_page_flow_task") is not None else "false")
' "${task_path}"
}

usage() {
    cat <<'EOF'
Usage: tools/m1-pipeline.sh <command> [args]

Commands:
  init <task-dir> [--page-id ID] [--page-name NAME] [--profile PATH]
                 Create a task workspace and seed task.json
  generate <task.json>
                 Generate LVGL page code from task/profile/HTML input
  render-ref <task.json>
                 Render input HTML into the task reference image
  sync
                 Scan workspace tasks and emit generated-page bridge files for the M1 build
  lint <task.json>
                 Run portability lint on generated or mapped source files
  run <task.json>
                 Run portability lint, then execute the workspace task bridge
  export <task.json>
                 Export generated page files into a portable bundle
  fetch <url> <task.json>
                 Download HTML from URL into the task input directory
  clean
                 Remove the build directory
  validate <task.json>
                 Run screenshot + diff validation only (no generate/build)
EOF
}

cmd="${1:-}"

case "${cmd}" in
    init)
        shift
        python3 "${SCRIPT_DIR}/m1-task-init.py" "$@"
        ;;
    generate)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/m1-generate-page.py" --task "${task_json}"
        ;;
    render-ref)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/m1-render-html-ref.py" --task "${task_json}"
        ;;
    sync)
        mkdir -p "${SCRIPT_DIR}/../m1_real_project/build"
        python3 "${SCRIPT_DIR}/m1-sync-generated-pages.py" \
            --tasks-root "${SCRIPT_DIR}/../workspace/tasks" \
            --registry-c "${SCRIPT_DIR}/../m1_real_project/build/generated_page_registry.c" \
            --registry-h "${SCRIPT_DIR}/../m1_real_project/build/generated_page_registry.h" \
            --cmake-out "${SCRIPT_DIR}/../m1_real_project/build/generated_page_sources.cmake"
        ;;
    lint)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/m1-portability-lint.py" --task "${task_json}"
        ;;
    run)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi

        if [[ "$(task_is_legacy_compat "${task_json}")" == "false" ]]; then
            python3 "${SCRIPT_DIR}/m1-generate-page.py" --task "${task_json}"

            # Verify generated output exists
            output_c=$(python3 -c "
import json, sys
from pathlib import Path
t = json.loads(Path(sys.argv[1]).read_text())
print((Path(sys.argv[1]).parent / t['generation']['output_c']).resolve())
" "${task_json}")
            if [[ ! -f "${output_c}" ]]; then
                echo "ERROR: generate did not produce output file: ${output_c}" >&2
                exit 1
            fi
        fi

        python3 "${SCRIPT_DIR}/m1-portability-lint.py" --task "${task_json}"
        python3 "${SCRIPT_DIR}/m1-task-run.py" --task "${task_json}"
        ;;
    export)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/m1-export-page.py" --task "${task_json}"
        ;;
    fetch)
        url="${2:-}"
        task_json="${3:-}"
        if [[ -z "${url}" || -z "${task_json}" ]]; then
            echo "Usage: tools/m1-pipeline.sh fetch <url> <task.json>" >&2
            exit 1
        fi
        input_dir="$(dirname "${task_json}")/input"
        mkdir -p "${input_dir}"
        curl -fsSL "${url}" -o "${input_dir}/index.html"
        echo "Downloaded ${url} -> ${input_dir}/index.html"
        ;;
    clean)
        echo "Cleaning build directory..."
        rm -rf "${SCRIPT_DIR}/../m1_real_project/build"
        echo "Build directory removed."
        ;;
    validate)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/m1-task-run.py" --task "${task_json}"
        ;;
    *)
        usage
        exit 1
        ;;
esac
