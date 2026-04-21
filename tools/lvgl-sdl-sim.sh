#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PORT_DIR="${ROOT_DIR}/lv_port_linux_test"
DEP_ROOT="${ROOT_DIR}/.deps/sdl2-image/root/usr"
PKG_CONFIG_PATH_VALUE="${DEP_ROOT}/lib/x86_64-linux-gnu/pkgconfig"
LIB_DIR="${DEP_ROOT}/lib/x86_64-linux-gnu"
BUILD_DIR="${PORT_DIR}/build"
BIN_PATH="${BUILD_DIR}/bin/lvglsim"

if [[ ! -d "${PORT_DIR}" ]]; then
    echo "Missing lv_port_linux_test at ${PORT_DIR}" >&2
    exit 1
fi

if [[ ! -f "${PKG_CONFIG_PATH_VALUE}/SDL2_image.pc" ]]; then
    echo "Missing local SDL2_image pkg-config file at ${PKG_CONFIG_PATH_VALUE}/SDL2_image.pc" >&2
    exit 1
fi

env_with_local_sdl() {
    env \
        PKG_CONFIG_PATH="${PKG_CONFIG_PATH_VALUE}${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}" \
        LD_LIBRARY_PATH="${LIB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
        "$@"
}

configure() {
    env_with_local_sdl cmake -S "${PORT_DIR}" -B "${BUILD_DIR}" -DCONFIG=sdl
}

build() {
    env_with_local_sdl cmake --build "${BUILD_DIR}" -j"$(nproc)"
}

backend_info() {
    env_with_local_sdl "${BIN_PATH}" -B
}

run_gui() {
    env_with_local_sdl "${BIN_PATH}" -b sdl
}

run_headless() {
    env_with_local_sdl \
        SDL_VIDEODRIVER=dummy \
        SDL_RENDER_DRIVER=software \
        "${BIN_PATH}" -b sdl
}

usage() {
    cat <<'EOF'
Usage: tools/lvgl-sdl-sim.sh <command>

Commands:
  configure      Configure lv_port_linux with CONFIG=sdl
  build          Build the SDL simulator
  rebuild        Clean build directory, then configure and build
  backend-info   Print default and supported backends
  run            Run the SDL simulator with the current desktop display
  run-headless   Run the SDL simulator with SDL dummy video driver
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
    backend-info)
        backend_info
        ;;
    run)
        run_gui
        ;;
    run-headless)
        run_headless
        ;;
    *)
        usage
        exit 1
        ;;
esac
