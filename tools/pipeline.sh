#!/usr/bin/env bash

set -euo pipefail

export PYTHONUNBUFFERED=1

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
  analyze <task.json> [--confirm]
                 Analyze input elements and validation strategy before generation
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
  webui [--host HOST] [--port PORT]
                 Launch the Web UI (requires: pip install -r requirements-webui.txt)
EOF
}

cmd="${1:-}"

case "${cmd}" in
    doctor)
        python3 "${SCRIPT_DIR}/doctor.py"
        ;;
    quickstart)
        demo_task="${SCRIPT_DIR}/../workspace/tasks/demo_v1/task.json"
        python3 "${SCRIPT_DIR}/doctor.py"
        python3 "${SCRIPT_DIR}/task-run.py" --task "${demo_task}"
        echo ""
        echo "Quickstart finished."
        echo "Report: ${SCRIPT_DIR}/../workspace/tasks/demo_v1/artifacts/report.json"
        echo "Screenshot: ${SCRIPT_DIR}/../workspace/tasks/demo_v1/artifacts/current.png"
        ;;
    init)
        shift
        python3 "${SCRIPT_DIR}/task-init.py" "$@"
        ;;
    analyze)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        if [[ "${3:-}" == "--confirm" ]]; then
            python3 "${SCRIPT_DIR}/analyze-page.py" --task "${task_json}" --confirm
        else
            python3 "${SCRIPT_DIR}/analyze-page.py" --task "${task_json}"
        fi
        ;;
    draft-html)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/image-to-html.py" --task "${task_json}"
        ;;
    generate)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/generate-page.py" --task "${task_json}"
        ;;
    render-ref)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/render-html-ref.py" --task "${task_json}"
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
        python3 "${SCRIPT_DIR}/portability-lint.py" --task "${task_json}"
        ;;
    refine)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/refine-page.py" --task "${task_json}"
        ;;
    run)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi

        if [[ "$(task_is_legacy_compat "${task_json}")" == "false" ]]; then
            python3 -c '
import json
import sys
from pathlib import Path

task_path = Path(sys.argv[1])
task = json.loads(task_path.read_text(encoding="utf-8"))
analysis = task.get("analysis", {})
if analysis.get("enabled", False) and analysis.get("require_user_confirm", False):
    output = task_path.parent / analysis.get("output", "analysis/analysis.json")
    if not output.is_file():
        raise SystemExit(f"Task requires analysis before run: {output}")
    if not analysis.get("confirmed", False):
        raise SystemExit("Task analysis must be confirmed before run.")
' "${task_json}"

            if [[ "$(python3 -c '
import json
import sys
from pathlib import Path
task = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
print(task.get("input", {}).get("source_type", "html"))
' "${task_json}")" == "image" ]]; then
                python3 "${SCRIPT_DIR}/image-to-html.py" --task "${task_json}"
                python3 "${SCRIPT_DIR}/asset-plan.py" --task "${task_json}"
                python3 "${SCRIPT_DIR}/asset-extract.py" --task "${task_json}"
            fi

            python3 "${SCRIPT_DIR}/generate-page.py" --task "${task_json}"

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

        python3 "${SCRIPT_DIR}/portability-lint.py" --task "${task_json}"

        # Pixel validation needs an HTML-rendered reference. Image-heavy or
        # manual-review tasks should not depend on a renderer.
        if [[ "$(python3 -c '
import json
import sys
from pathlib import Path
task = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
print(task.get("validation", {}).get("mode", "pixel"))
' "${task_json}")" == "pixel" ]]; then
            python3 "${SCRIPT_DIR}/render-html-ref.py" --task "${task_json}"
        fi

        set +e
        python3 "${SCRIPT_DIR}/task-run.py" --task "${task_json}"
        run_status=$?
        set -e

        if [[ "${run_status}" -eq 2 ]]; then
            python3 "${SCRIPT_DIR}/refine-page.py" --task "${task_json}"
        elif [[ "${run_status}" -ne 0 ]]; then
            # Build or other failure — attempt refine which handles build errors internally
            echo "Task run failed (exit ${run_status}), attempting refine..."
            set +e
            python3 "${SCRIPT_DIR}/refine-page.py" --task "${task_json}"
            refine_status=$?
            set -e
            if [[ "${refine_status}" -ne 0 ]]; then
                exit "${refine_status}"
            fi
        fi
        ;;
    export)
        task_json="${2:-}"
        if [[ -z "${task_json}" ]]; then
            usage
            exit 1
        fi
        python3 "${SCRIPT_DIR}/export-page.py" --task "${task_json}"
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
        python3 "${SCRIPT_DIR}/task-run.py" --task "${task_json}"
        ;;
    webui)
        shift
        python3 "${SCRIPT_DIR}/webui.py" "$@"
        ;;
    *)
        usage
        exit 1
        ;;
esac
