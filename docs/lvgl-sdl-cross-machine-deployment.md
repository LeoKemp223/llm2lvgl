# LVGL SDL Simulator Cross-Machine Deployment

## Scope
- Target: run the official LVGL SDL simulator from [`/home/leo/work/open-git/lvgl_agent/lv_port_linux_test`](/home/leo/work/open-git/lvgl_agent/lv_port_linux_test)
- Preferred entrypoint: [`/home/leo/work/open-git/lvgl_agent/tools/lvgl-sdl-sim.sh`](/home/leo/work/open-git/lvgl_agent/tools/lvgl-sdl-sim.sh)
- Goal: make the simulator reproducible on another machine with the fewest surprises

## What Is Already Fixed In This Repo
- The upstream link problem for non-system SDL library paths is fixed in [`/home/leo/work/open-git/lvgl_agent/lv_port_linux_test/CMakeLists.txt`](/home/leo/work/open-git/lvgl_agent/lv_port_linux_test/CMakeLists.txt#L136)
- The local fallback `SDL2_image.pc` is normalized in [`/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root/usr/lib/x86_64-linux-gnu/pkgconfig/SDL2_image.pc`](/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root/usr/lib/x86_64-linux-gnu/pkgconfig/SDL2_image.pc#L1)
- Common configure/build/run commands are wrapped in [`/home/leo/work/open-git/lvgl_agent/tools/lvgl-sdl-sim.sh`](/home/leo/work/open-git/lvgl_agent/tools/lvgl-sdl-sim.sh#L1)

These fixes are part of the workspace. If another machine uses this same workspace content, the CMake-side fix does not need to be redone.

## Recommended Standard Environment
- OS: Linux desktop, `x86_64`
- Compiler: `gcc` and `g++`
- Build tools: `cmake`, `make`, `pkg-config`
- Runtime: `python3`, `pip`
- SDL packages:
  - `libsdl2-dev`
  - `libsdl2-image-dev`

If the target machine is Linux desktop and has the packages above, use the standard path first. Do not start with the local fallback unless system install is blocked.

## Standard Path
### 1. Check prerequisites
Run:

```bash
which gcc g++ cmake pkg-config python3
pkg-config --modversion sdl2
pkg-config --modversion SDL2_image
```

Expected:
- all tools exist
- `sdl2` returns a version
- `SDL2_image` returns a version

### 2. Build
Run:

```bash
tools/lvgl-sdl-sim.sh rebuild
```

### 3. Verify backend selection
Run:

```bash
tools/lvgl-sdl-sim.sh backend-info
```

Expected:

```text
Default backend: SDL
Supported backends: SDL
```

### 4. Run the simulator
Desktop machine:

```bash
tools/lvgl-sdl-sim.sh run
```

Headless or CI machine:

```bash
tools/lvgl-sdl-sim.sh run-headless
```

## Fallback Path When `libsdl2-image-dev` Cannot Be Installed
Use the repository-local fallback only when system package installation is blocked.

### When This Fallback Is Valid
- Linux
- `x86_64`
- same or similar glibc environment as this machine

### When This Fallback Is Not A Good Choice
- ARM machines
- macOS
- Windows
- Linux distributions with incompatible runtime ABI expectations

In those cases, install native dependencies on the target machine instead of reusing [`/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root`](/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root).

### Fallback Files
- local library root: [`/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root`](/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root)
- local pkg-config file: [`/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root/usr/lib/x86_64-linux-gnu/pkgconfig/SDL2_image.pc`](/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root/usr/lib/x86_64-linux-gnu/pkgconfig/SDL2_image.pc)

The helper script already exports the required `PKG_CONFIG_PATH` and `LD_LIBRARY_PATH`, so the normal commands stay the same:

```bash
tools/lvgl-sdl-sim.sh rebuild
tools/lvgl-sdl-sim.sh run
```

## Desktop Vs Headless
`tools/lvgl-sdl-sim.sh run`
- uses the current desktop display session
- creates a real SDL window
- use this when a human needs to see the UI

`tools/lvgl-sdl-sim.sh run-headless`
- sets `SDL_VIDEODRIVER=dummy`
- forces `SDL_RENDER_DRIVER=software`
- does not create a real window
- use this for smoke tests, CI, remote servers, or “can it boot” checks

## Cross-Machine Risk Matrix
### Low risk
- Linux `x86_64`
- desktop session available
- `libsdl2-dev` and `libsdl2-image-dev` installed from system packages

### Medium risk
- Linux `x86_64`
- no desktop session
- using headless mode only

### High risk
- system package install blocked
- relying on repository-local `SDL2_image` fallback
- different distro family or different architecture

## First-Pass Validation Checklist
On a new machine, run these in order:

```bash
which gcc g++ cmake pkg-config python3
pkg-config --modversion sdl2
pkg-config --modversion SDL2_image
tools/lvgl-sdl-sim.sh rebuild
tools/lvgl-sdl-sim.sh backend-info
timeout 5s tools/lvgl-sdl-sim.sh run-headless
```

If the machine has a GUI session, then also run:

```bash
tools/lvgl-sdl-sim.sh run
```

## Common Failures And Fixes
### `Package SDL2_image was not found`
Cause:
- missing `libsdl2-image-dev`
- or `pkg-config` cannot see the local fallback

Fix:
- preferred: install `libsdl2-image-dev`
- fallback: confirm [`/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root/usr/lib/x86_64-linux-gnu/pkgconfig/SDL2_image.pc`](/home/leo/work/open-git/lvgl_agent/.deps/sdl2-image/root/usr/lib/x86_64-linux-gnu/pkgconfig/SDL2_image.pc) exists and use [`/home/leo/work/open-git/lvgl_agent/tools/lvgl-sdl-sim.sh`](/home/leo/work/open-git/lvgl_agent/tools/lvgl-sdl-sim.sh#L24)

### `ld: cannot find -lSDL2_image`
Cause:
- another workspace is using old upstream `CMakeLists.txt` without the repo fix

Fix:
- ensure the target machine has the modified [`/home/leo/work/open-git/lvgl_agent/lv_port_linux_test/CMakeLists.txt`](/home/leo/work/open-git/lvgl_agent/lv_port_linux_test/CMakeLists.txt#L410)

### `tools/lvgl-sdl-sim.sh run` does not show a window
Cause:
- no usable display session
- remote shell without GUI forwarding
- window exists but is not visible on current session

Fix:
- check `echo "$DISPLAY"`
- use `run-headless` first
- on a desktop session, rerun `tools/lvgl-sdl-sim.sh run`

### configure step tries to install `pcpp`
Cause:
- python package `pcpp` is not preinstalled

Fix:
- install it in advance:

```bash
python3 -m pip install --user pcpp
```

- if the machine has no network, preinstall it before moving offline

## Recommended Team Rule
- Treat system packages as the primary path
- Treat repository-local `SDL2_image` as a temporary compatibility fallback
- Keep using [`/home/leo/work/open-git/lvgl_agent/tools/lvgl-sdl-sim.sh`](/home/leo/work/open-git/lvgl_agent/tools/lvgl-sdl-sim.sh#L1) instead of hand-written environment variables
- Do not overwrite the current SDL link fix in [`/home/leo/work/open-git/lvgl_agent/lv_port_linux_test/CMakeLists.txt`](/home/leo/work/open-git/lvgl_agent/lv_port_linux_test/CMakeLists.txt#L410)

## Minimal Quick Start
For a normal Linux desktop machine:

```bash
tools/lvgl-sdl-sim.sh rebuild
tools/lvgl-sdl-sim.sh backend-info
tools/lvgl-sdl-sim.sh run
```

For CI or a machine without GUI:

```bash
tools/lvgl-sdl-sim.sh rebuild
tools/lvgl-sdl-sim.sh backend-info
timeout 5s tools/lvgl-sdl-sim.sh run-headless
```
