#!/usr/bin/env python3

import importlib
import os
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


REPO_ROOT = Path(__file__).resolve().parent.parent
PROJECT_DIR = REPO_ROOT / "runtime_project"
LV_PORT_DIR = REPO_ROOT / "lv_port_linux_test"
LVGL_DIR = LV_PORT_DIR / "lvgl"
DOCTOR_TMP_DIR = REPO_ROOT / ".tmp"
DEMO_TASK = REPO_ROOT / "workspace" / "tasks" / "demo_v1" / "task.json"
DEMO_REFERENCE = REPO_ROOT / "workspace" / "tasks" / "demo_v1" / "reference" / "reference.png"
LOCAL_SDL2_IMAGE_ROOT = REPO_ROOT / ".deps" / "sdl2-image" / "root" / "usr"
LOCAL_SDL2_IMAGE_PKGCONFIG = LOCAL_SDL2_IMAGE_ROOT / "lib" / "x86_64-linux-gnu" / "pkgconfig"
LOCAL_SDL2_IMAGE_PC = LOCAL_SDL2_IMAGE_PKGCONFIG / "SDL2_image.pc"
LOCAL_SDL2_IMAGE_LIBDIR = LOCAL_SDL2_IMAGE_ROOT / "lib" / "x86_64-linux-gnu"


@dataclass
class CheckResult:
    level: str
    title: str
    detail: str = ""


def ok(title: str, detail: str = "") -> CheckResult:
    return CheckResult("OK", title, detail)


def warn(title: str, detail: str = "") -> CheckResult:
    return CheckResult("WARN", title, detail)


def err(title: str, detail: str = "") -> CheckResult:
    return CheckResult("ERR", title, detail)


def has_command(name: str) -> bool:
    return shutil.which(name) is not None


def build_env() -> dict:
    env = dict(os.environ)
    if LOCAL_SDL2_IMAGE_PC.is_file():
        env["PKG_CONFIG_PATH"] = (
            f"{LOCAL_SDL2_IMAGE_PKGCONFIG}{':' + env['PKG_CONFIG_PATH'] if env.get('PKG_CONFIG_PATH') else ''}"
        )
        env["LD_LIBRARY_PATH"] = (
            f"{LOCAL_SDL2_IMAGE_LIBDIR}{':' + env['LD_LIBRARY_PATH'] if env.get('LD_LIBRARY_PATH') else ''}"
        )
    return env


def run_quiet(cmd: List[str], env: Optional[dict] = None) -> subprocess.CompletedProcess:
    return subprocess.run(
        cmd,
        cwd=str(REPO_ROOT),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=env,
    )


def summarize_output(output: str, max_lines: int = 8) -> str:
    lines = [line.rstrip() for line in output.splitlines() if line.strip()]
    if not lines:
        return "Unknown failure."
    if len(lines) <= max_lines:
        return "\n".join(lines)
    return "\n".join(["..."] + lines[-max_lines:])


def check_commands() -> List[CheckResult]:
    results = []
    for command in ("python3", "cmake", "pkg-config"):
        if has_command(command):
            results.append(ok(f"`{command}` is available"))
        else:
            results.append(err(f"`{command}` is missing"))
    return results


def check_python_modules() -> List[CheckResult]:
    results = []
    for module_name, label in (("PIL", "Pillow"),):
        try:
            importlib.import_module(module_name)
            results.append(ok(f"Python module `{label}` is available"))
        except Exception as exc:  # pragma: no cover - import failure detail only
            results.append(err(f"Python module `{label}` is missing", str(exc)))
    return results


def check_pkg_config() -> List[CheckResult]:
    results = []
    if not has_command("pkg-config"):
        return [err("`pkg-config` is missing, cannot verify native dependencies")]

    env = build_env()

    packages = (
        ("sdl2", "SDL2"),
        ("freetype2", "FreeType"),
    )

    for package_name, label in packages:
        completed = run_quiet(["pkg-config", "--exists", package_name], env=env)
        if completed.returncode == 0:
            results.append(ok(f"Native dependency `{label}` is available"))
        else:
            results.append(err(f"Native dependency `{label}` is missing"))

    system_check = run_quiet(["pkg-config", "--exists", "SDL2_image"])
    if system_check.returncode == 0:
        results.append(ok("Native dependency `SDL2_image` is available"))
        return results

    if LOCAL_SDL2_IMAGE_PC.is_file():
        fallback_check = run_quiet(["pkg-config", "--exists", "SDL2_image"], env=env)
        if fallback_check.returncode == 0:
            results.append(ok("Native dependency `SDL2_image` is available", f"Using repo fallback at `{LOCAL_SDL2_IMAGE_PC.relative_to(REPO_ROOT)}`"))
        else:
            results.append(err("Native dependency `SDL2_image` fallback is broken", f"Found `{LOCAL_SDL2_IMAGE_PC.relative_to(REPO_ROOT)}` but pkg-config still cannot resolve it."))
    else:
        results.append(err("Native dependency `SDL2_image` is missing"))

    return results


def check_optional_html_renderer() -> CheckResult:
    for candidate in ("chromium", "chromium-browser", "google-chrome", "google-chrome-stable", "wkhtmltoimage"):
        if has_command(candidate):
            return ok("HTML reference renderer is available", f"Detected `{candidate}`")
    return warn(
        "HTML reference renderer is unavailable",
        "Install chromium/chromium-browser/google-chrome or wkhtmltoimage to use `render-ref` on new tasks.",
    )


def check_repo_files() -> List[CheckResult]:
    results = []
    if DEMO_TASK.is_file():
        results.append(ok("Bundled quickstart task exists", str(DEMO_TASK.relative_to(REPO_ROOT))))
    else:
        results.append(err("Bundled quickstart task is missing", str(DEMO_TASK)))

    if DEMO_REFERENCE.is_file():
        results.append(ok("Bundled quickstart reference image exists", str(DEMO_REFERENCE.relative_to(REPO_ROOT))))
    else:
        results.append(warn("Bundled quickstart reference image is missing", "Quickstart may need HTML rendering tools."))

    missing_runtime_paths = [
        path.relative_to(REPO_ROOT).as_posix()
        for path in (
            PROJECT_DIR / "CMakeLists.txt",
            LV_PORT_DIR / "CMakeLists.txt",
            LVGL_DIR / "CMakeLists.txt",
            LVGL_DIR / "scripts" / "generate_lv_conf.py",
            REPO_ROOT / "tools" / "m1-sync-generated-pages.py",
        )
        if not path.is_file()
    ]
    if missing_runtime_paths:
        results.append(
            err(
                "Runtime source checkout is incomplete",
                "Missing "
                + ", ".join(f"`{path}`" for path in missing_runtime_paths)
                + ". Clone `https://github.com/lvgl/lv_port_linux.git` into `lv_port_linux_test/`, then run "
                + "`git -C lv_port_linux_test submodule update --init --recursive`.",
            )
        )
    else:
        results.append(ok("Runtime source checkout exists", "`lv_port_linux_test/` and nested `lvgl/` are present"))
    return results


def check_cmake_configure(core_ready: bool) -> CheckResult:
    if not core_ready:
        return warn("Skipped CMake configure check", "Fix core dependency errors first.")

    DOCTOR_TMP_DIR.mkdir(parents=True, exist_ok=True)

    try:
        with tempfile.TemporaryDirectory(prefix="doctor-cmake-", dir=str(DOCTOR_TMP_DIR)) as probe_dir:
            completed = run_quiet(["cmake", "-S", str(PROJECT_DIR), "-B", probe_dir], env=build_env())
            if completed.returncode == 0:
                return ok("CMake configure succeeded", "temporary probe build passed")
    except OSError as exc:
        return err("CMake configure failed", str(exc))

    detail = completed.stderr.strip() or completed.stdout.strip() or "Unknown configure failure."
    return err("CMake configure failed", summarize_output(detail))


def print_results(results: List[CheckResult]) -> None:
    for result in results:
        line = f"[{result.level}] {result.title}"
        if result.detail:
            line = f"{line} - {result.detail}"
        print(line)


def main() -> int:
    results: List[CheckResult] = []
    results.extend(check_commands())
    results.extend(check_python_modules())
    results.extend(check_pkg_config())
    results.append(check_optional_html_renderer())
    results.extend(check_repo_files())

    core_ready = all(
        item.level != "ERR"
        for item in results
        if item.title not in {"HTML reference renderer is unavailable", "Python module `Pillow` is missing"}
    )
    results.append(check_cmake_configure(core_ready))

    print_results(results)

    error_count = sum(1 for item in results if item.level == "ERR")
    warn_count = sum(1 for item in results if item.level == "WARN")
    print(f"\nSummary: {error_count} error(s), {warn_count} warning(s)")

    if error_count == 0:
        print("Next step: `tools/pipeline.sh quickstart`")

    return 1 if error_count else 0


if __name__ == "__main__":
    raise SystemExit(main())
