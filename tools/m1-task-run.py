#!/usr/bin/env python3

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Optional


REPO_ROOT = Path(__file__).resolve().parent.parent
TOOLS_DIR = REPO_ROOT / "tools"
PROJECT_DIR = REPO_ROOT / "m1_real_project"
BUILD_ROOT = PROJECT_DIR / "build"


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def sanitize_segment(value: str) -> str:
    value = value.strip().lower()
    value = re.sub(r"[^a-z0-9]+", "-", value)
    value = value.strip("-")
    return value or "task"


def task_build_dir(task_path: Path, task: dict) -> Path:
    task_key = task.get("task_id") or task_path.parent.name or task["page_id"]
    return BUILD_ROOT / sanitize_segment(task_key)


def task_env(task_path: Path, task: dict, build_dir: Path) -> dict:
    env = os.environ.copy()
    env["M1_BUILD_DIR"] = str(build_dir)
    env["M1_TASK_JSON"] = str(task_path)
    env["M1_PAGE"] = task["page_id"]
    env["M1_VIEWPORT_WIDTH"] = str(int(task["target"]["viewport"]["width"]))
    env["M1_VIEWPORT_HEIGHT"] = str(int(task["target"]["viewport"]["height"]))
    return env


def run(cmd: list, env: Optional[dict] = None, allowed_exit_codes: Optional[set] = None) -> int:
    result = subprocess.run(cmd, check=False, cwd=str(REPO_ROOT), env=env)
    if allowed_exit_codes is not None and result.returncode in allowed_exit_codes:
        return result.returncode
    result.check_returncode()
    return result.returncode


def make_legacy_compat_task(task_path: Path, task: dict) -> Path:
    compat_task = {
        "page_id": task["page_id"],
        "page_name": task["page_name"],
        "entry_page": task["page_id"],
        "reference_image": "reference/reference.png",
        "artifacts_dir": "artifacts",
        "viewport": task["target"]["viewport"],
        "validation": task["validation"],
        "completion": {
            "require_validation_pass": True
        },
        "failure_policy": {
            "max_iterations": task["failure_policy"]["max_iterations"],
            "stop_on_build_error": task["failure_policy"]["stop_on_build_error"],
            "stop_on_missing_reference": task["failure_policy"]["stop_on_missing_reference"],
            "stop_on_validation_script_error": task["failure_policy"]["stop_on_validation_script_error"]
        }
    }

    compat_path = task_path.parent / "workflow" / "tasks" / "compat_validation_task.json"
    compat_path.parent.mkdir(parents=True, exist_ok=True)
    compat_path.write_text(json.dumps(compat_task, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return compat_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a workspace task against the current M1 simulator bridge.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    parser.add_argument("--rerender-ref", action="store_true", help="Force rerender of the HTML reference before validation")
    parser.add_argument("--skip-sync", action="store_true", help="Skip regenerating workspace page registry files")
    parser.add_argument("--skip-configure", action="store_true", help="Skip CMake configure before build")
    parser.add_argument("--skip-full-screenshot", action="store_true", help="Skip writing the full-page screenshot artifact")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    build_dir = task_build_dir(task_path, task)
    build_dir.mkdir(parents=True, exist_ok=True)
    cmd_env = task_env(task_path, task, build_dir)
    reference_path = resolve(task_path, task["reference"]["image"])

    if task["reference"].get("render_from_html", False) and (args.rerender_ref or not reference_path.exists()):
        run([sys.executable, str(TOOLS_DIR / "m1-render-html-ref.py"), "--task", str(task_path)])

    if not args.skip_sync:
        run([
            sys.executable,
            str(TOOLS_DIR / "m1-sync-generated-pages.py"),
            "--task-json",
            str(task_path),
            "--registry-c",
            str(build_dir / "generated_page_registry.c"),
            "--registry-h",
            str(build_dir / "generated_page_registry.h"),
            "--cmake-out",
            str(build_dir / "generated_page_sources.cmake"),
        ], env=cmd_env)

    if not args.skip_configure:
        run([str(TOOLS_DIR / "lvgl-m1-real.sh"), "configure"], env=cmd_env)
    run([str(TOOLS_DIR / "lvgl-m1-real.sh"), "build"], env=cmd_env)

    artifacts_dir = task_path.parent / "artifacts"
    artifacts_dir.mkdir(parents=True, exist_ok=True)
    current_path = artifacts_dir / "current.png"
    full_path = artifacts_dir / "full.png"
    diff_path = artifacts_dir / "diff.png"
    report_path = artifacts_dir / "report.json"

    run([str(TOOLS_DIR / "lvgl-m1-real.sh"), "screenshot", str(current_path)], env=cmd_env)
    if not args.skip_full_screenshot:
        run([str(TOOLS_DIR / "lvgl-m1-real.sh"), "screenshot-full", str(full_path)], env=cmd_env)

    compat_task_path = make_legacy_compat_task(task_path, task)
    validator_status = run([
        sys.executable,
        str(TOOLS_DIR / "m1-page-validate.py"),
        "--task",
        str(compat_task_path),
        "--current",
        str(current_path),
        "--diff",
        str(diff_path),
        "--report",
        str(report_path),
    ], allowed_exit_codes={0, 2})

    print(f"Task run completed: {task_path} (status={validator_status})")
    return validator_status


if __name__ == "__main__":
    raise SystemExit(main())
