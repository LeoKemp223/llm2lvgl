#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_DIR="${ROOT_DIR}/runtime_project"
DEFAULT_BUILD_DIR="${PROJECT_DIR}/build"
TASKS_ROOT="${ROOT_DIR}/workspace/tasks"
DEP_ROOT="${ROOT_DIR}/.deps/sdl2-image/root/usr"
PKG_CONFIG_PATH_VALUE="${DEP_ROOT}/lib/x86_64-linux-gnu/pkgconfig"
LIB_DIR="${DEP_ROOT}/lib/x86_64-linux-gnu"

sync_env_aliases() {
    if [[ -n "${LVGL_PAGE:-}" && -z "${M1_PAGE:-}" ]]; then
        export M1_PAGE="${LVGL_PAGE}"
    fi
    if [[ -n "${LVGL_BUILD_DIR:-}" && -z "${M1_BUILD_DIR:-}" ]]; then
        export M1_BUILD_DIR="${LVGL_BUILD_DIR}"
    fi
    if [[ -n "${LVGL_TASK_JSON:-}" && -z "${M1_TASK_JSON:-}" ]]; then
        export M1_TASK_JSON="${LVGL_TASK_JSON}"
    fi
    if [[ -n "${LVGL_VIEWPORT_WIDTH:-}" && -z "${M1_VIEWPORT_WIDTH:-}" ]]; then
        export M1_VIEWPORT_WIDTH="${LVGL_VIEWPORT_WIDTH}"
    fi
    if [[ -n "${LVGL_VIEWPORT_HEIGHT:-}" && -z "${M1_VIEWPORT_HEIGHT:-}" ]]; then
        export M1_VIEWPORT_HEIGHT="${LVGL_VIEWPORT_HEIGHT}"
    fi

    if [[ -n "${M1_PAGE:-}" ]]; then
        export LVGL_PAGE="${M1_PAGE}"
    fi
    if [[ -n "${M1_BUILD_DIR:-}" ]]; then
        export LVGL_BUILD_DIR="${M1_BUILD_DIR}"
    fi
    if [[ -n "${M1_TASK_JSON:-}" ]]; then
        export LVGL_TASK_JSON="${M1_TASK_JSON}"
    fi
    if [[ -n "${M1_VIEWPORT_WIDTH:-}" ]]; then
        export LVGL_VIEWPORT_WIDTH="${M1_VIEWPORT_WIDTH}"
    fi
    if [[ -n "${M1_VIEWPORT_HEIGHT:-}" ]]; then
        export LVGL_VIEWPORT_HEIGHT="${M1_VIEWPORT_HEIGHT}"
    fi
}

env_with_local_sdl() {
    if [[ -f "${PKG_CONFIG_PATH_VALUE}/SDL2_image.pc" ]]; then
        env \
            PKG_CONFIG_PATH="${PKG_CONFIG_PATH_VALUE}${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}" \
            LD_LIBRARY_PATH="${LIB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
            "$@"
        return
    fi

    env "$@"
}

current_build_dir() {
    printf '%s\n' "${LVGL_BUILD_DIR:-${M1_BUILD_DIR:-${DEFAULT_BUILD_DIR}}}"
}

current_bin_path() {
    printf '%s/lvgl_runtime_demo\n' "$(current_build_dir)"
}

infer_task_json_from_page() {
    local resolved

    if [[ -n "${LVGL_TASK_JSON:-${M1_TASK_JSON:-}}" || -z "${LVGL_PAGE:-${M1_PAGE:-}}" ]]; then
        return
    fi

    resolved="$(python3 -c '
import json
import sys
from pathlib import Path

tasks_root = Path(sys.argv[1]).resolve()
page_id = sys.argv[2]
matches = []

for task_path in sorted(tasks_root.glob("*/task.json")):
    task = json.loads(task_path.read_text(encoding="utf-8"))
    if task.get("compat", {}).get("legacy_page_flow_task") is not None:
        continue
    if task.get("page_id") == page_id:
        matches.append(str(task_path.resolve()))

if len(matches) > 1:
    raise SystemExit(f"Multiple tasks found for page_id={page_id}: {matches}")

print(matches[0] if matches else "")
' "${TASKS_ROOT}" "${LVGL_PAGE:-${M1_PAGE}}")"

    if [[ -n "${resolved}" ]]; then
        export M1_TASK_JSON="${resolved}"
        export LVGL_TASK_JSON="${resolved}"
    fi
}

infer_build_dir_from_task() {
    local resolved

    if [[ -n "${LVGL_BUILD_DIR:-${M1_BUILD_DIR:-}}" || -z "${LVGL_TASK_JSON:-${M1_TASK_JSON:-}}" ]]; then
        return
    fi

    resolved="$(python3 -c '
import json
import re
import sys
from pathlib import Path

task_path = Path(sys.argv[1]).resolve()
build_root = Path(sys.argv[2]).resolve()
task = json.loads(task_path.read_text(encoding="utf-8"))
task_key = task.get("task_id") or task_path.parent.name or task["page_id"]
task_key = re.sub(r"[^a-z0-9]+", "-", task_key.strip().lower()).strip("-") or "task"
print((build_root / task_key).as_posix())
' "${LVGL_TASK_JSON:-${M1_TASK_JSON}}" "${DEFAULT_BUILD_DIR}")"

    export M1_BUILD_DIR="${resolved}"
    export LVGL_BUILD_DIR="${resolved}"
}

apply_task_viewport_env() {
    local viewport width height

    if [[ -z "${LVGL_TASK_JSON:-${M1_TASK_JSON:-}}" ]]; then
        return
    fi

    if [[ -n "${LVGL_VIEWPORT_WIDTH:-${M1_VIEWPORT_WIDTH:-}}" && -n "${LVGL_VIEWPORT_HEIGHT:-${M1_VIEWPORT_HEIGHT:-}}" ]]; then
        return
    fi

    viewport="$(python3 -c '
import json
import sys
from pathlib import Path

task = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
viewport = task["target"]["viewport"]
print(f"{int(viewport['\''width'\''])} {int(viewport['\''height'\''])}")
' "${LVGL_TASK_JSON:-${M1_TASK_JSON}}")"
    read -r width height <<<"${viewport}"

    : "${LVGL_VIEWPORT_WIDTH:=${M1_VIEWPORT_WIDTH:-${width}}}"
    : "${LVGL_VIEWPORT_HEIGHT:=${M1_VIEWPORT_HEIGHT:-${height}}}"
    export LVGL_VIEWPORT_WIDTH LVGL_VIEWPORT_HEIGHT
    export M1_VIEWPORT_WIDTH="${LVGL_VIEWPORT_WIDTH}"
    export M1_VIEWPORT_HEIGHT="${LVGL_VIEWPORT_HEIGHT}"
}

resolve_task_context() {
    infer_task_json_from_page
    infer_build_dir_from_task
    apply_task_viewport_env
}

ensure_binary_ready() {
    local bin_path
    resolve_task_context
    bin_path="$(current_bin_path)"
    if [[ -x "${bin_path}" ]]; then
        return
    fi

    echo "Binary not found at ${bin_path}; configuring and building..." >&2
    configure
    build
}

configure() {
    local build_dir
    local args=(
        -S "${PROJECT_DIR}"
    )

    resolve_task_context
    build_dir="$(current_build_dir)"
    args+=(-B "${build_dir}")

    if [[ -n "${LVGL_TASK_JSON:-${M1_TASK_JSON:-}}" ]]; then
        args+=("-DLVGL_TASK_JSON=${LVGL_TASK_JSON:-${M1_TASK_JSON}}")
    fi

    env_with_local_sdl cmake "${args[@]}"
}

build() {
    local build_dir
    resolve_task_context
    build_dir="$(current_build_dir)"
    env_with_local_sdl cmake --build "${build_dir}" -j"$(nproc)"
}

run_gui() {
    ensure_binary_ready
    env_with_local_sdl "$(current_bin_path)"
}

run_headless() {
    ensure_binary_ready
    env_with_local_sdl SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software "$(current_bin_path)"
}

run_headless_screenshot() {
    local output_path="${1:-${PROJECT_DIR}/artifacts/homepage.png}"
    ensure_binary_ready
    mkdir -p "$(dirname "${output_path}")"
    env_with_local_sdl \
        LVGL_SCREENSHOT_OUT="${output_path}" \
        SDL_VIDEODRIVER=dummy \
        SDL_RENDER_DRIVER=software \
        "$(current_bin_path)"
}

run_headless_full_screenshot() {
    local output_path="${1:-${PROJECT_DIR}/artifacts/homepage-full.png}"
    ensure_binary_ready
    mkdir -p "$(dirname "${output_path}")"
    env_with_local_sdl \
        LVGL_SCREENSHOT_FULL_OUT="${output_path}" \
        SDL_VIDEODRIVER=dummy \
        SDL_RENDER_DRIVER=software \
        "$(current_bin_path)"
}

list_pages() {
    ensure_binary_ready
    env_with_local_sdl "$(current_bin_path)" --list-pages
}

usage() {
    local script_name="${LVGL_RUNTIME_NAME:-lvgl-runtime.sh}"

    cat <<EOF
Usage: tools/${script_name} <command>

Commands:
  configure      Configure the standalone LVGL runtime project
  build          Build the standalone LVGL runtime project
  rebuild        Clean and rebuild the standalone LVGL runtime project
  run            Run the standalone LVGL runtime project with a real window
  run-headless   Run the standalone LVGL runtime project headlessly
  screenshot     Run headlessly, save a PNG screenshot, then exit
  screenshot-full
                 Run headlessly, save a full content PNG screenshot, then exit
  list-pages     Print the registered page ids

Environment:
  LVGL_PAGE      Preferred page selector for run/screenshot commands.
                 The legacy alias M1_PAGE is still supported.
  LVGL_BUILD_DIR Preferred CMake build directory override.
                 The legacy alias M1_BUILD_DIR is still supported.
  LVGL_TASK_JSON Preferred single-task configure/build target.
                 The legacy alias M1_TASK_JSON is still supported.
                 When task context is inferred, LVGL_VIEWPORT_WIDTH/HEIGHT are auto-filled from target.viewport.
EOF
}

sync_env_aliases
cmd="${1:-}"

case "${cmd}" in
    configure)
        configure
        ;;
    build)
        build
        ;;
    rebuild)
        resolve_task_context
        rm -rf "$(current_build_dir)"
        configure
        build
        ;;
    run)
        run_gui
        ;;
    run-headless)
        run_headless
        ;;
    screenshot)
        run_headless_screenshot "${2:-}"
        ;;
    screenshot-full)
        run_headless_full_screenshot "${2:-}"
        ;;
    list-pages)
        list_pages
        ;;
    *)
        usage
        exit 1
        ;;
esac
