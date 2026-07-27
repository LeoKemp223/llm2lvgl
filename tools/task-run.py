#!/usr/bin/env python3

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional


REPO_ROOT = Path(__file__).resolve().parent.parent
TOOLS_DIR = REPO_ROOT / "tools"
PROJECT_DIR = REPO_ROOT / "runtime_project"
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
    env["LVGL_BUILD_DIR"] = str(build_dir)
    env["LVGL_TASK_JSON"] = str(task_path)
    env["LVGL_PAGE"] = task["page_id"]
    env["LVGL_VIEWPORT_WIDTH"] = str(int(task["target"]["viewport"]["width"]))
    env["LVGL_VIEWPORT_HEIGHT"] = str(int(task["target"]["viewport"]["height"]))
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


def write_manual_review_report(
    task_path: Path,
    task: dict,
    current_path: Path,
    full_path: Path,
    diff_path: Path,
    report_path: Path,
    reference_path: Path,
    validation_mode: str,
) -> None:
    analysis_path = resolve(task_path, task.get("analysis", {}).get("output", "analysis/analysis.json"))
    analysis = {}
    if analysis_path.is_file():
        try:
            analysis = json.loads(analysis_path.read_text("utf-8"))
        except (json.JSONDecodeError, OSError):
            analysis = {}

    report = {
        "task": task["page_id"],
        "page_name": task["page_name"],
        "validation_mode": validation_mode,
        "pass": True,
        "completion_met": True,
        "manual_review_required": True,
        "failure_reason": None,
        "reference_image": str(reference_path) if reference_path.exists() else None,
        "current_image": str(current_path),
        "full_image": str(full_path) if full_path.exists() else None,
        "diff_image": str(diff_path) if diff_path.exists() else None,
        "analysis": {
            "page_type": analysis.get("page_type"),
            "contains_images": analysis.get("contains_images"),
            "validation_mode": analysis.get("validation_mode"),
            "risk_notes": analysis.get("risk_notes", []),
        },
        "review_notes": [
            "Strict whole-screen pixel diff was skipped because this page was classified as image-heavy, mixed, semantic, or manual-review.",
            "Review current.png/full.png visually and use the analysis result to judge image fidelity.",
        ],
        "created_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
    }
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Manual review report written: {report_path}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a workspace task against the current LVGL simulator bridge.")
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
    validation_mode = task.get("validation", {}).get("mode", "pixel")

    # For image-based tasks, use the source image as reference if no reference exists
    source_type = task.get("input", {}).get("source_type", "html")
    if source_type == "image" and not reference_path.exists():
        image_rel = task.get("input", {}).get("image_entry")
        if image_rel:
            src_img = resolve(task_path, image_rel)
            if src_img.is_file():
                reference_path.parent.mkdir(parents=True, exist_ok=True)
                viewport = task.get("target", {}).get("viewport", {})
                vw = viewport.get("width")
                vh = viewport.get("height")
                if vw and vh:
                    from PIL import Image
                    img = Image.open(str(src_img))
                    if img.size != (vw, vh):
                        print(f"将参考图从 {img.size} 缩放到 ({vw}, {vh})")
                        img = img.resize((vw, vh), Image.LANCZOS)
                    img.save(str(reference_path))
                else:
                    shutil.copy2(str(src_img), str(reference_path))
                print(f"参考图已设置: {src_img}")

    if validation_mode == "pixel" and task["reference"].get("render_from_html", False) and (args.rerender_ref or not reference_path.exists()):
        # Check if any HTML renderer is available before attempting render
        _renderers = ["chromium", "chromium-browser", "google-chrome", "google-chrome-stable", "wkhtmltoimage"]
        if any(shutil.which(r) for r in _renderers):
            run([sys.executable, str(TOOLS_DIR / "render-html-ref.py"), "--task", str(task_path)])
        else:
            print("WARNING: No HTML renderer found, skipping reference image rendering.", file=sys.stderr)
            print("  Visual validation will be skipped. Install chromium or wkhtmltoimage for full validation.", file=sys.stderr)

    if not args.skip_sync:
        run([
            sys.executable,
            str(TOOLS_DIR / "sync-generated-pages.py"),
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
        run([str(TOOLS_DIR / "lvgl-runtime.sh"), "configure"], env=cmd_env)
    run([str(TOOLS_DIR / "lvgl-runtime.sh"), "build"], env=cmd_env)

    artifacts_dir = task_path.parent / "artifacts"
    artifacts_dir.mkdir(parents=True, exist_ok=True)
    current_path = artifacts_dir / "current.png"
    full_path = artifacts_dir / "full.png"
    diff_path = artifacts_dir / "diff.png"
    report_path = artifacts_dir / "report.json"

    run([str(TOOLS_DIR / "lvgl-runtime.sh"), "screenshot", str(current_path)], env=cmd_env)
    if not args.skip_full_screenshot:
        run([str(TOOLS_DIR / "lvgl-runtime.sh"), "screenshot-full", str(full_path)], env=cmd_env)

    if validation_mode != "pixel":
        write_manual_review_report(
            task_path,
            task,
            current_path,
            full_path,
            diff_path,
            report_path,
            reference_path,
            validation_mode,
        )
        print(f"Task run completed: {task_path} (status=0, validation_mode={validation_mode})")
        return 0

    if not reference_path.exists():
        print("WARNING: No reference image available, skipping visual validation.", file=sys.stderr)
        print(f"Task run completed: {task_path} (status=0, validation skipped)")
        return 0

    compat_task_path = make_legacy_compat_task(task_path, task)
    validator_status = run([
        sys.executable,
        str(TOOLS_DIR / "page-validate.py"),
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
