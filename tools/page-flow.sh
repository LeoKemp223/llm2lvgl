#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_DIR="${ROOT_DIR}/runtime_project"
TASKS_DIR="${PROJECT_DIR}/workflow/tasks"
VALIDATOR="${SCRIPT_DIR}/page-validate.py"
RUNTIME_RUNNER="${SCRIPT_DIR}/lvgl-runtime.sh"

CURRENT_ITERATION=""

resolve_task_path() {
    local task_ref="$1"

    if [[ -f "${task_ref}" ]]; then
        python3 -c 'from pathlib import Path; import sys; print(Path(sys.argv[1]).resolve())' "${task_ref}"
        return
    fi

    if [[ -f "${TASKS_DIR}/${task_ref}.json" ]]; then
        python3 -c 'from pathlib import Path; import sys; print(Path(sys.argv[1]).resolve())' "${TASKS_DIR}/${task_ref}.json"
        return
    fi

    echo "Task not found: ${task_ref}" >&2
    exit 1
}

require_reference_if_needed() {
    local task_path="$1"
    local reference_path
    local stop_on_missing_reference

    reference_path="$(task_abs_path "${task_path}" "reference_image")"
    stop_on_missing_reference="$(task_value "${task_path}" "failure_policy.stop_on_missing_reference")"

    if [[ ! -f "${reference_path}" ]]; then
        echo "Missing reference image: ${reference_path}" >&2
        if [[ "${stop_on_missing_reference}" == "True" || "${stop_on_missing_reference}" == "true" ]]; then
            exit 20
        fi
    fi
}

enforce_iteration_policy() {
    local task_path="$1"
    local max_iterations
    local iteration

    max_iterations="$(task_value "${task_path}" "failure_policy.max_iterations")"
    iteration="${CURRENT_ITERATION:-1}"

    if [[ "${iteration}" -gt "${max_iterations}" ]]; then
        echo "Iteration limit exceeded: current=${iteration}, max=${max_iterations}" >&2
        exit 21
    fi
}

task_value() {
    local task_path="$1"
    local key="$2"

    python3 -c '
import json
import sys
from pathlib import Path

task = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
value = task
for part in sys.argv[2].split("."):
    value = value[part]
print(value)
' "${task_path}" "${key}"
}

task_abs_path() {
    local task_path="$1"
    local key="$2"

    python3 -c '
import json
import sys
from pathlib import Path

task_path = Path(sys.argv[1]).resolve()
task = json.loads(task_path.read_text(encoding="utf-8"))
value = task
for part in sys.argv[2].split("."):
    value = value[part]
print((task_path.parent.parent.parent / value).resolve())
' "${task_path}" "${key}"
}

ensure_build() {
    if [[ ! -x "${PROJECT_DIR}/build/lvgl_runtime_demo" ]]; then
        "${RUNTIME_RUNNER}" configure
    fi

    "${RUNTIME_RUNNER}" build
}

run_screenshots() {
    local task_path="$1"
    local page_id
    local artifacts_dir
    local current_path
    local full_path

    page_id="$(task_value "${task_path}" "entry_page")"
    artifacts_dir="$(task_abs_path "${task_path}" "artifacts_dir")"
    current_path="${artifacts_dir}/current.png"
    full_path="${artifacts_dir}/full.png"

    mkdir -p "${artifacts_dir}"

    LVGL_PAGE="${page_id}" "${RUNTIME_RUNNER}" screenshot "${current_path}"
    LVGL_PAGE="${page_id}" "${RUNTIME_RUNNER}" screenshot-full "${full_path}"
}

run_validation() {
    local task_path="$1"
    local artifacts_dir
    local current_path
    local diff_path
    local report_path
    local stop_on_validation_script_error

    artifacts_dir="$(task_abs_path "${task_path}" "artifacts_dir")"
    current_path="${artifacts_dir}/current.png"
    diff_path="${artifacts_dir}/diff.png"
    report_path="${artifacts_dir}/report.json"
    stop_on_validation_script_error="$(task_value "${task_path}" "failure_policy.stop_on_validation_script_error")"

    set +e
    python3 "${VALIDATOR}" \
        --task "${task_path}" \
        --current "${current_path}" \
        --diff "${diff_path}" \
        --report "${report_path}"
    validator_status=$?
    set -e

    if [[ ${validator_status} -eq 0 || ${validator_status} -eq 2 ]]; then
        return ${validator_status}
    fi

    if [[ "${stop_on_validation_script_error}" == "True" || "${stop_on_validation_script_error}" == "true" ]]; then
        exit ${validator_status}
    fi

    return ${validator_status}
}

print_task_info() {
    local task_path="$1"
    local page_name
    local page_id
    local entry_page
    local reference_image
    local artifacts_dir

    page_name="$(task_value "${task_path}" "page_name")"
    page_id="$(task_value "${task_path}" "page_id")"
    entry_page="$(task_value "${task_path}" "entry_page")"
    reference_image="$(task_abs_path "${task_path}" "reference_image")"
    artifacts_dir="$(task_abs_path "${task_path}" "artifacts_dir")"

    cat <<EOF
task_path=${task_path}
page_id=${page_id}
page_name=${page_name}
entry_page=${entry_page}
reference_image=${reference_image}
artifacts_dir=${artifacts_dir}
EOF
}

usage() {
    local script_name="${LVGL_PAGE_FLOW_NAME:-page-flow.sh}"

    cat <<EOF
Usage: tools/${script_name} <command> <task>

Commands:
  info        Print resolved task information
  build       Build the LVGL runtime project for the task
  screenshot  Capture viewport and full-content screenshots for the task
  validate    Run screenshot validation for the task
  run         Build, screenshot, and validate the task in one pass
  loop        Run one bounded automation iteration: loop <task> <iteration>
EOF
}

command_name="${1:-}"
task_ref="${2:-}"
CURRENT_ITERATION="${3:-${LVGL_LOOP_ITERATION:-${M1_LOOP_ITERATION:-1}}}"

if [[ -z "${command_name}" ]]; then
    usage
    exit 1
fi

if [[ "${command_name}" != "help" && -z "${task_ref}" ]]; then
    usage
    exit 1
fi

case "${command_name}" in
    help)
        usage
        ;;
    info)
        task_path="$(resolve_task_path "${task_ref}")"
        print_task_info "${task_path}"
        ;;
    build)
        task_path="$(resolve_task_path "${task_ref}")"
        print_task_info "${task_path}"
        require_reference_if_needed "${task_path}"
        ensure_build
        ;;
    screenshot)
        task_path="$(resolve_task_path "${task_ref}")"
        require_reference_if_needed "${task_path}"
        ensure_build
        run_screenshots "${task_path}"
        ;;
    validate)
        task_path="$(resolve_task_path "${task_ref}")"
        require_reference_if_needed "${task_path}"
        run_validation "${task_path}"
        ;;
    run|loop)
        task_path="$(resolve_task_path "${task_ref}")"
        print_task_info "${task_path}"
        require_reference_if_needed "${task_path}"
        if [[ "${command_name}" == "loop" ]]; then
            enforce_iteration_policy "${task_path}"
            echo "iteration=${CURRENT_ITERATION}"
        fi
        ensure_build
        run_screenshots "${task_path}"
        run_validation "${task_path}"
        ;;
    *)
        usage
        exit 1
        ;;
esac
