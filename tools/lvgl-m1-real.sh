#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_DIR="${ROOT_DIR}/m1_real_project"
BUILD_DIR="${PROJECT_DIR}/build"
BIN_PATH="${BUILD_DIR}/lvgl_m1_demo"

configure() {
    cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}"
}

build() {
    cmake --build "${BUILD_DIR}" -j"$(nproc)"
}

run_gui() {
    "${BIN_PATH}"
}

run_headless() {
    SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software "${BIN_PATH}"
}

run_headless_screenshot() {
    local output_path="${1:-${PROJECT_DIR}/artifacts/homepage.png}"
    mkdir -p "$(dirname "${output_path}")"
    LVGL_SCREENSHOT_OUT="${output_path}" SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software "${BIN_PATH}"
}

run_headless_full_screenshot() {
    local output_path="${1:-${PROJECT_DIR}/artifacts/homepage-full.png}"
    mkdir -p "$(dirname "${output_path}")"
    LVGL_SCREENSHOT_FULL_OUT="${output_path}" SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software "${BIN_PATH}"
}

list_pages() {
    "${BIN_PATH}" --list-pages
}

usage() {
    cat <<'EOF'
Usage: tools/lvgl-m1-real.sh <command>

Commands:
  configure      Configure the standalone M1 LVGL project
  build          Build the standalone M1 LVGL project
  rebuild        Clean and rebuild the standalone M1 LVGL project
  run            Run the standalone M1 LVGL project with a real window
  run-headless   Run the standalone M1 LVGL project headlessly
  screenshot     Run headlessly, save a PNG screenshot, then exit
  screenshot-full
                 Run headlessly, save a full content PNG screenshot, then exit
  list-pages     Print the registered page ids

Environment:
  M1_PAGE        Select the active page id for run/screenshot commands
EOF
}

cmd="${1:-}"

case "${cmd}" in
    configure)
        configure
        ;;
    build)
        build
        ;;
    rebuild)
        rm -rf "${BUILD_DIR}"
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
