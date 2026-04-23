#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TASKS_ROOT="${SCRIPT_DIR}/../workspace/tasks"

pass_count=0
fail_count=0
skip_count=0
failed_tasks=()

if [[ ! -d "${TASKS_ROOT}" ]]; then
    echo "No tasks directory found at ${TASKS_ROOT}"
    exit 0
fi

for task_json in "${TASKS_ROOT}"/*/task.json; do
    if [[ ! -f "${task_json}" ]]; then
        continue
    fi

    task_id="$(basename "$(dirname "${task_json}")")"
    echo "--- Running: ${task_id} ---"

    if ! "${SCRIPT_DIR}/pipeline.sh" run "${task_json}"; then
        echo "FAIL: ${task_id} (pipeline error)"
        fail_count=$((fail_count + 1))
        failed_tasks+=("${task_id}")
        continue
    fi

    report="$(dirname "${task_json}")/artifacts/report.json"
    if [[ ! -f "${report}" ]]; then
        echo "SKIP: ${task_id} (no report.json)"
        skip_count=$((skip_count + 1))
        continue
    fi

    passed=$(python3 -c "
import json, sys
r = json.loads(open(sys.argv[1]).read())
print('true' if r.get('pass', False) else 'false')
" "${report}")

    if [[ "${passed}" == "true" ]]; then
        echo "PASS: ${task_id}"
        pass_count=$((pass_count + 1))
    else
        echo "FAIL: ${task_id} (validation)"
        fail_count=$((fail_count + 1))
        failed_tasks+=("${task_id}")
    fi
done

echo ""
echo "=== Regression Summary ==="
echo "Pass: ${pass_count}  Fail: ${fail_count}  Skip: ${skip_count}"

if [[ ${fail_count} -gt 0 ]]; then
    echo "Failed tasks: ${failed_tasks[*]}"
    exit 1
fi

exit 0
