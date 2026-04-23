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
    local script_name="${LVGL_PIPELINE_NAME:-pipeline.sh}"

    cat <<EOF
Usage: tools/${script_name} <command> [args]

Commands:
  doctor
                 Check local dependencies and whether the bundled demo can run
  quickstart
                 Run the bundled demo task end-to-end and write artifacts under workspace/tasks/demo_v1/
  init <task-dir> [--page-id ID] [--page-name NAME] [--profile PATH]
                 Create a task workspace and seed task.json
  draft-html <task.json>
                 Draft HTML from image input for image-based tasks
  generate <task.json>
                 Generate LVGL page code from task/profile input
  render-ref <task.json>
                 Render input HTML into the task reference image
  sync [task.json]
                 Emit generated-page bridge files for all tasks, or only the specified task
  lint <task.json>
                 Run portability lint on generated or mapped source files
  refine <task.json>
                 Run automatic screenshot-driven source refinement until pass/max_iterations
  run <task.json>
                 Run portability lint, execute the workspace task bridge, then refine on validation failure
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
    doctor)
        python3 "${SCRIPT_DIR}/m1-doctor.py"
        ;;
    quickstart)
        demo_task="${SCRIPT_DIR}/../workspace/tasks/demo_v1/task.json"
        python3 "${SCRIPT_DIR}/m1-doctor.py"
        python3 "${SCRIPT_DIR}/m1-task-run.py" --task "${demo_task}"
        echo ""
        echo "Quickstart finished."
        echo "Report: ${SCRIPT_DIR}/../workspace/tasks/demo_v1/artifacts/report.json"
        echo "Screenshot: ${SCRIPT_DIR}/../workspace/tasks/demo_v1/artifacts/current.png"
        ;;
    init)
        shift
        python3 "${SCRIPT_DIR}/m1-task-init.py" "$@"
        ;;
    draft-html)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/m1-image-to-html.py" --task "${task_json}"
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
        task_json="${2:-}"
        if [[ -n "${task_json}" ]]; then
            build_dir="$(python3 -c '
import json
import re
import sys
from pathlib import Path

task_path = Path(sys.argv[1]).resolve()
task = json.loads(task_path.read_text(encoding="utf-8"))
task_key = task.get("task_id") or task_path.parent.name or task["page_id"]
task_key = re.sub(r"[^a-z0-9]+", "-", task_key.strip().lower()).strip("-") or "task"
print((Path(sys.argv[2]).resolve() / task_key).as_posix())
' "${task_json}" "${SCRIPT_DIR}/../runtime_project/build")"
            mkdir -p "${build_dir}"
            python3 "${SCRIPT_DIR}/sync-generated-pages.py" \
                --task-json "${task_json}" \
                --registry-c "${build_dir}/generated_page_registry.c" \
                --registry-h "${build_dir}/generated_page_registry.h" \
                --cmake-out "${build_dir}/generated_page_sources.cmake"
        else
            mkdir -p "${SCRIPT_DIR}/../runtime_project/build"
            python3 "${SCRIPT_DIR}/sync-generated-pages.py" \
                --tasks-root "${SCRIPT_DIR}/../workspace/tasks" \
                --registry-c "${SCRIPT_DIR}/../runtime_project/build/generated_page_registry.c" \
                --registry-h "${SCRIPT_DIR}/../runtime_project/build/generated_page_registry.h" \
                --cmake-out "${SCRIPT_DIR}/../runtime_project/build/generated_page_sources.cmake"
        fi
        ;;
    lint)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/m1-portability-lint.py" --task "${task_json}"
        ;;
    refine)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/m1-refine-page.py" --task "${task_json}"
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

        set +e
        python3 "${SCRIPT_DIR}/m1-task-run.py" --task "${task_json}"
        run_status=$?
        set -e

        if [[ "${run_status}" -eq 2 ]]; then
            python3 "${SCRIPT_DIR}/m1-refine-page.py" --task "${task_json}"
        elif [[ "${run_status}" -ne 0 ]]; then
            exit "${run_status}"
        fi
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
            echo "Usage: tools/${LVGL_PIPELINE_NAME:-pipeline.sh} fetch <url> <task.json>" >&2
            exit 1
        fi
        input_dir="$(dirname "${task_json}")/input"
        mkdir -p "${input_dir}"
        curl -fsSL "${url}" -o "${input_dir}/index.html"
        echo "Downloaded ${url} -> ${input_dir}/index.html"
        ;;
    clean)
        echo "Cleaning build directory..."
        rm -rf "${SCRIPT_DIR}/../runtime_project/build"
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
