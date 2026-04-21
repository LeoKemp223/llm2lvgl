#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PORT_DIR="${ROOT_DIR}/lv_port_linux_test"
DEP_ROOT="${ROOT_DIR}/.deps/sdl2-image/root/usr"
PKG_CONFIG_PATH_VALUE="${DEP_ROOT}/lib/x86_64-linux-gnu/pkgconfig"
LIB_DIR="${DEP_ROOT}/lib/x86_64-linux-gnu"
BUILD_DIR="${PORT_DIR}/build-hello"
BIN_PATH="${BUILD_DIR}/bin/lvglsim"

env_with_local_sdl() {
    env \
        PKG_CONFIG_PATH="${PKG_CONFIG_PATH_VALUE}${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}" \
        LD_LIBRARY_PATH="${LIB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
        "$@"
}

configure() {
    env_with_local_sdl cmake -S "${PORT_DIR}" -B "${BUILD_DIR}" -DCONFIG=sdl-hello
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
Usage: tools/lvgl-sdl-hello.sh <command>

Commands:
  configure      Configure the hello-world SDL demo
  build          Build the hello-world SDL demo
  rebuild        Clean and rebuild the hello-world SDL demo
  backend-info   Print default and supported backends
  run            Run the hello-world SDL demo with a real window
  run-headless   Run the hello-world SDL demo in headless mode
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
