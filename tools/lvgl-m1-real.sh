#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROJECT_DIR="${ROOT_DIR}/m1_real_project"
BUILD_DIR="${PROJECT_DIR}/build"
BIN_PATH="${BUILD_DIR}/lvgl_m1_demo"
DEP_ROOT="${ROOT_DIR}/.deps/sdl2-image/root/usr"
PKG_CONFIG_PATH_VALUE="${DEP_ROOT}/lib/x86_64-linux-gnu/pkgconfig"
LIB_DIR="${DEP_ROOT}/lib/x86_64-linux-gnu"

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

configure() {
    env_with_local_sdl cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}"
}

build() {
    env_with_local_sdl cmake --build "${BUILD_DIR}" -j"$(nproc)"
}

run_gui() {
    env_with_local_sdl "${BIN_PATH}"
}

run_headless() {
    env_with_local_sdl SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software "${BIN_PATH}"
}

run_headless_screenshot() {
    local output_path="${1:-${PROJECT_DIR}/artifacts/homepage.png}"
    mkdir -p "$(dirname "${output_path}")"
    env_with_local_sdl \
        LVGL_SCREENSHOT_OUT="${output_path}" \
        SDL_VIDEODRIVER=dummy \
        SDL_RENDER_DRIVER=software \
        "${BIN_PATH}"
}

run_headless_full_screenshot() {
    local output_path="${1:-${PROJECT_DIR}/artifacts/homepage-full.png}"
    mkdir -p "$(dirname "${output_path}")"
    env_with_local_sdl \
        LVGL_SCREENSHOT_FULL_OUT="${output_path}" \
        SDL_VIDEODRIVER=dummy \
        SDL_RENDER_DRIVER=software \
        "${BIN_PATH}"
}

list_pages() {
    env_with_local_sdl "${BIN_PATH}" --list-pages
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
