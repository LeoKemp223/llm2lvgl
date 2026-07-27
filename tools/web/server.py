#!/usr/bin/env python3
"""Flask server for llm2lvgl Web UI."""

import json
import os
import re
import signal
import shutil
import subprocess
import sys
import threading
import time
import uuid
import zipfile
from io import BytesIO
from pathlib import Path

from flask import (
    Flask,
    Response,
    jsonify,
    render_template,
    request,
    send_file,
    send_from_directory,
)

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
TOOLS_DIR = REPO_ROOT / "tools"
PROFILES_DIR = REPO_ROOT / "profiles"
TASKS_DIR = REPO_ROOT / "workspace" / "tasks"
SETTINGS_PATH = REPO_ROOT / "workspace" / ".llm_settings.json"
PIPELINE_SH = TOOLS_DIR / "pipeline.sh"

# In-memory task run state
TASK_RUNS: dict[str, dict] = {}
_lock = threading.Lock()

_SLUG_RE = re.compile(r"[^a-z0-9_-]")

_HTML_RENDERERS = [
    "chromium", "chromium-browser", "google-chrome",
    "google-chrome-stable", "wkhtmltoimage",
]


def _has_html_renderer() -> bool:
    return any(shutil.which(r) for r in _HTML_RENDERERS)


def _slugify(value: str) -> str:
    return _SLUG_RE.sub("", value.lower().replace(" ", "-"))[:64]


def _safe_task_id(task_id: str) -> str | None:
    """Return sanitised task_id or None if it looks like path traversal."""
    slug = _slugify(task_id)
    if not slug or ".." in task_id or "/" in task_id:
        return None
    return slug


def _safe_upload_suffix(filename: str, fallback: str = ".png") -> str:
    suffix = Path(filename or "").suffix.lower()
    return suffix if suffix in {".png", ".jpg", ".jpeg", ".bmp", ".webp"} else fallback


def _task_dir(task_id: str) -> Path:
    return TASKS_DIR / task_id


def _task_json(task_id: str) -> Path:
    return _task_dir(task_id) / "task.json"


def _task_artifacts(task_id: str) -> dict[str, bool]:
    artifacts = {}
    art_dir = _task_dir(task_id) / "artifacts"
    for name in ("current.png", "full.png", "diff.png", "report.json", "run_state.json"):
        artifacts[name] = (art_dir / name).is_file()
    return artifacts


def _task_analysis_path(task_id: str, task: dict | None = None) -> Path:
    if task is None:
        tj = _task_json(task_id)
        task = json.loads(tj.read_text("utf-8")) if tj.is_file() else {}
    rel = task.get("analysis", {}).get("output", "analysis/analysis.json")
    return _task_dir(task_id) / rel


def _load_analysis(task_id: str, task: dict | None = None) -> dict:
    path = _task_analysis_path(task_id, task)
    if not path.is_file():
        return {}
    try:
        return json.loads(path.read_text("utf-8"))
    except (json.JSONDecodeError, OSError):
        return {}


def _profile_summary(task: dict) -> dict:
    target = task.get("target", {})
    profile_ref = target.get("profile", "")
    profile_name = Path(profile_ref).name if profile_ref else ""
    profile_path = (_task_dir(task.get("task_id", "")) / profile_ref).resolve() if profile_ref else None
    label = Path(profile_name).stem if profile_name else ""

    if profile_path and profile_path.is_file():
        try:
            data = json.loads(profile_path.read_text("utf-8"))
            label = data.get("name") or data.get("id") or label
        except (json.JSONDecodeError, OSError):
            pass

    return {
        "file": profile_name,
        "name": label,
        "viewport": target.get("viewport", {}),
        "color_depth": target.get("color_depth"),
        "dpi": target.get("dpi"),
    }


def _run_state_path(task_id: str) -> Path:
    return _task_dir(task_id) / "artifacts" / "run_state.json"


def _public_run_state(run: dict) -> dict:
    return {
        "status": run.get("status", "idle"),
        "step": run.get("step", ""),
        "exit_code": run.get("exit_code"),
        "started": run.get("started"),
        "updated": time.time(),
    }


def _write_run_state(task_id: str, run: dict) -> None:
    path = _run_state_path(task_id)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(_public_run_state(run), indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def _load_run_state(task_id: str) -> dict:
    path = _run_state_path(task_id)
    if not path.is_file():
        return {}
    try:
        return json.loads(path.read_text("utf-8"))
    except (json.JSONDecodeError, OSError):
        return {}


def _report_pass(task_id: str) -> bool | None:
    report_path = _task_dir(task_id) / "artifacts" / "report.json"
    if not report_path.is_file():
        return None
    try:
        report = json.loads(report_path.read_text("utf-8"))
    except (json.JSONDecodeError, OSError):
        return None
    value = report.get("pass", report.get("passed"))
    return bool(value) if value is not None else None


def _reconcile_run_state(task_id: str) -> dict:
    """Normalize stale in-memory state before exposing it to the UI."""
    run = TASK_RUNS.get(task_id, {})
    if not run:
        saved = _load_run_state(task_id)
        if saved:
            return saved
        artifacts = _task_artifacts(task_id)
        if any(artifacts.values()):
            passed = _report_pass(task_id)
            if passed is not None:
                inferred = {
                    "status": "done" if passed else "failed",
                    "log": [],
                    "step": "export" if passed else "refine",
                    "exit_code": 0 if passed else 2,
                }
                _write_run_state(task_id, inferred)
                return inferred
        return run

    if run.get("status") != "running":
        _write_run_state(task_id, run)
        return run

    proc = run.get("proc")
    if proc is not None and proc.poll() is not None:
        rc = proc.returncode
        run["exit_code"] = rc
        run["status"] = "done" if rc == 0 else "failed"
        if not run.get("_reconciled"):
            run["log"].append(f"--- process exited with status {rc} ---")
            run["_reconciled"] = True
        _write_run_state(task_id, run)
        return run

    # If the process object disappeared but artifacts exist, do not leave the
    # dashboard spinning forever. This can happen after worker/thread loss.
    artifacts = _task_artifacts(task_id)
    if proc is None and any(artifacts.values()):
        passed = _report_pass(task_id)
        run["exit_code"] = 0 if passed else 2
        run["status"] = "done" if passed else "failed"
        if not run.get("_reconciled"):
            reason = "validation passed" if passed else "validation threshold not met"
            run.setdefault("log", []).append(f"--- recovered stale run state: {reason} ---")
            run["_reconciled"] = True
        _write_run_state(task_id, run)
        return run

    _write_run_state(task_id, run)
    return run


# ---------------------------------------------------------------------------
# Flask app factory
# ---------------------------------------------------------------------------

def create_app() -> Flask:
    app = Flask(
        __name__,
        template_folder=str(Path(__file__).parent / "templates"),
        static_folder=str(Path(__file__).parent / "static"),
    )
    app.config["MAX_CONTENT_LENGTH"] = 32 * 1024 * 1024  # 32 MB

    # -- Pages ---------------------------------------------------------------

    @app.get("/")
    def index():
        return render_template("index.html")

    # -- Profiles ------------------------------------------------------------

    @app.get("/api/profiles")
    def list_profiles():
        profiles = []
        for p in sorted(PROFILES_DIR.glob("*.json")):
            try:
                data = json.loads(p.read_text("utf-8"))
                profiles.append({
                    "id": data.get("id", p.stem),
                    "name": data.get("name", p.stem),
                    "file": p.name,
                    "screen": data.get("screen", {}),
                })
            except Exception:
                continue
        return jsonify(profiles)

    # -- Settings ------------------------------------------------------------

    @app.get("/api/settings")
    def get_settings():
        settings = {}
        if SETTINGS_PATH.is_file():
            try:
                settings = json.loads(SETTINGS_PATH.read_text("utf-8"))
            except (json.JSONDecodeError, OSError):
                pass
        # Mask api_key for display
        key = settings.get("api_key", "")
        if key and len(key) > 4:
            settings["api_key"] = f"sk-...{key[-4:]}"
        elif key:
            settings["api_key"] = "sk-...****"
        return jsonify(settings)

    @app.put("/api/settings")
    def put_settings():
        data = request.get_json(silent=True)
        if not data or not isinstance(data, dict):
            return jsonify(error="JSON body required"), 400

        # Load existing settings to preserve masked api_key
        existing = {}
        if SETTINGS_PATH.is_file():
            try:
                existing = json.loads(SETTINGS_PATH.read_text("utf-8"))
            except (json.JSONDecodeError, OSError):
                pass

        new_key = (data.get("api_key") or "").strip()
        # If the key looks masked, keep the old one
        if new_key.startswith("sk-...") or not new_key:
            data["api_key"] = existing.get("api_key", "")
        else:
            data["api_key"] = new_key

        data["model"] = (data.get("model") or "").strip()
        data["base_url"] = (data.get("base_url") or "").strip()

        # Only keep known keys
        save = {k: data[k] for k in ("api_key", "model", "base_url") if data.get(k)}

        SETTINGS_PATH.parent.mkdir(parents=True, exist_ok=True)
        SETTINGS_PATH.write_text(json.dumps(save, indent=2, ensure_ascii=False), "utf-8")
        return jsonify(ok=True)

    # -- Tasks CRUD ----------------------------------------------------------

    @app.get("/api/tasks")
    def list_tasks():
        tasks = []
        if not TASKS_DIR.is_dir():
            return jsonify(tasks)
        for d in sorted(TASKS_DIR.iterdir()):
            tj = d / "task.json"
            if not tj.is_file():
                continue
            try:
                data = json.loads(tj.read_text("utf-8"))
            except Exception:
                continue
            run = _reconcile_run_state(d.name)
            created_at = tj.stat().st_mtime
            tasks.append({
                "task_id": d.name,
                "page_name": data.get("page_name", d.name),
                "source_type": data.get("input", {}).get("source_type", ""),
                "status": run.get("status", "idle"),
                "created_at": created_at,
                "target": _profile_summary(data),
                "analysis": {
                    "confirmed": bool(data.get("analysis", {}).get("confirmed", False)),
                    "validation_mode": data.get("validation", {}).get("mode", "pixel"),
                },
            })
        tasks.sort(key=lambda item: item.get("created_at", 0), reverse=True)
        return jsonify(tasks)

    @app.get("/api/tasks/<task_id>")
    def get_task(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        tj = _task_json(tid)
        if not tj.is_file():
            return jsonify(error="not found"), 404
        data = json.loads(tj.read_text("utf-8"))
        run = _reconcile_run_state(tid)
        # Attach artifacts info
        artifacts = _task_artifacts(tid)
        return jsonify(task=data, run_status=run.get("status", "idle"),
                       run_step=run.get("step", ""),
                       exit_code=run.get("exit_code"),
                       artifacts=artifacts,
                       analysis=_load_analysis(tid, data))

    @app.delete("/api/tasks/<task_id>")
    def delete_task(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        td = _task_dir(tid)
        if not td.exists():
            return jsonify(error="not found"), 404

        with _lock:
            run = TASK_RUNS.get(tid)
            if run and run.get("status") == "running":
                proc = run.get("proc")
                if proc and proc.poll() is None:
                    try:
                        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
                    except (ProcessLookupError, OSError):
                        proc.kill()
                run["status"] = "stopped"
                run["exit_code"] = -15
            TASK_RUNS.pop(tid, None)

        shutil.rmtree(td, ignore_errors=True)
        return jsonify(ok=True, task_id=tid)

    @app.post("/api/tasks")
    def create_task():
        source_type = request.form.get("source_type", "html")
        profile = request.form.get("profile", "")
        name = request.form.get("name", "").strip()

        if source_type not in ("html", "image", "url"):
            return jsonify(error="source_type must be html, image, or url"), 400
        if not profile:
            return jsonify(error="profile is required"), 400

        # Handle custom profile with user-specified resolution
        if profile == "__custom__":
            custom_name = request.form.get("custom_name", "").strip()
            custom_width = request.form.get("custom_width", "").strip()
            custom_height = request.form.get("custom_height", "").strip()
            try:
                cw = int(custom_width)
                ch = int(custom_height)
                assert cw >= 100 and ch >= 100
            except (ValueError, AssertionError):
                return jsonify(error="自定义分辨率无效（宽高均不小于 100）"), 400
            slug = _slugify(custom_name or f"{cw}x{ch}")
            profile_fname = f"custom_{slug}.json"
            profile_path = PROFILES_DIR / profile_fname
            if not profile_path.is_file():
                profile_data = {
                    "version": 1,
                    "id": slug,
                    "name": custom_name or f"Custom {cw}x{ch}",
                    "screen": {
                        "width": cw,
                        "height": ch,
                        "color_depth": 32,
                        "dpi": 160,
                    },
                    "fonts": {
                        "allow_freetype": True,
                        "builtin_fonts": ["lv_font_montserrat_14", "lv_font_montserrat_24"],
                        "default_font": "lv_font_montserrat_14",
                    },
                    "assets": {
                        "allow_filesystem": True,
                        "allow_png": True,
                        "allow_jpg": True,
                        "max_asset_kb": 2048,
                    },
                    "constraints": {
                        "allow_sdl_only_api": False,
                        "allow_snapshot_api_in_export": False,
                    },
                }
                PROFILES_DIR.mkdir(parents=True, exist_ok=True)
                profile_path.write_text(
                    json.dumps(profile_data, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )
        else:
            profile_path = PROFILES_DIR / profile
            if not profile_path.is_file():
                return jsonify(error=f"profile not found: {profile}"), 400

        task_id = _slugify(name) if name else f"web_{uuid.uuid4().hex[:8]}"
        # Ensure unique
        while _task_dir(task_id).exists():
            task_id = f"web_{uuid.uuid4().hex[:8]}"

        td = _task_dir(task_id)

        try:
            image_upload_path = None
            if source_type == "image":
                f = request.files.get("file")
                if not f:
                    return jsonify(error="file is required for image source"), 400
                upload_dir = TASKS_DIR / ".uploads"
                upload_dir.mkdir(parents=True, exist_ok=True)
                image_upload_path = upload_dir / f"{task_id}{_safe_upload_suffix(f.filename)}"
                f.save(str(image_upload_path))

            init_args = [
                str(PIPELINE_SH), "init", str(td),
                "--profile", str(profile_path),
                "--source-type", "image" if source_type == "image" else "html",
            ]
            if image_upload_path is not None:
                init_args += ["--image", str(image_upload_path)]
            if name:
                init_args += ["--page-name", name]
            subprocess.run(init_args, check=True, capture_output=True, text=True)

            if image_upload_path is not None:
                image_upload_path.unlink(missing_ok=True)

            if source_type == "html":
                f = request.files.get("file")
                if not f:
                    return jsonify(error="file is required for html source"), 400
                input_dir = td / "input"
                input_dir.mkdir(parents=True, exist_ok=True)
                f.save(str(input_dir / "index.html"))
                # Save additional asset files (CSS, JS, images, fonts)
                for asset in request.files.getlist("assets"):
                    safe_name = Path(asset.filename or "").name
                    if safe_name and ".." not in safe_name:
                        asset.save(str(input_dir / safe_name))
            elif source_type == "image":
                input_dir = td / "input"
                input_dir.mkdir(parents=True, exist_ok=True)
                # Save additional asset images (icons, etc.)
                for asset in request.files.getlist("assets"):
                    safe_name = Path(asset.filename or "").name
                    if safe_name and ".." not in safe_name:
                        asset.save(str(input_dir / safe_name))
            elif source_type == "url":
                url = request.form.get("url", "").strip()
                if not url:
                    return jsonify(error="url is required"), 400
                subprocess.run(
                    [str(PIPELINE_SH), "fetch", url, str(_task_json(task_id))],
                    check=True, capture_output=True, text=True,
                )
        except subprocess.CalledProcessError as exc:
            # Clean up on failure
            if td.exists():
                shutil.rmtree(td, ignore_errors=True)
            upload_dir = TASKS_DIR / ".uploads"
            for leftover in upload_dir.glob(f"{task_id}.*") if upload_dir.is_dir() else []:
                leftover.unlink(missing_ok=True)
            return jsonify(error="task init failed",
                           detail=exc.stderr or exc.stdout), 400

        # Disable render_from_html when no renderer is installed
        if not _has_html_renderer():
            tj_path = _task_json(task_id)
            if tj_path.is_file():
                task_data = json.loads(tj_path.read_text("utf-8"))
                if task_data.get("reference", {}).get("render_from_html"):
                    task_data["reference"]["render_from_html"] = False
                    tj_path.write_text(
                        json.dumps(task_data, indent=2, ensure_ascii=False), "utf-8"
                    )

        return jsonify(task_id=task_id), 201

    @app.post("/api/tasks/<task_id>/analyze")
    def analyze_task(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        tj = _task_json(tid)
        if not tj.is_file():
            return jsonify(error="not found"), 404

        try:
            proc = subprocess.run(
                [str(PIPELINE_SH), "analyze", str(tj)],
                check=True,
                capture_output=True,
                text=True,
                cwd=str(REPO_ROOT),
            )
        except subprocess.CalledProcessError as exc:
            return jsonify(error="analysis failed", detail=exc.stderr or exc.stdout), 400

        task = json.loads(tj.read_text("utf-8"))
        return jsonify(ok=True, log=proc.stdout, analysis=_load_analysis(tid, task), task=task)

    @app.post("/api/tasks/<task_id>/analysis/confirm")
    def confirm_analysis(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        tj = _task_json(tid)
        if not tj.is_file():
            return jsonify(error="not found"), 404
        body = request.get_json(silent=True) or {}
        analysis = body.get("analysis")
        task = json.loads(tj.read_text("utf-8"))
        analysis_path = _task_analysis_path(tid, task)
        if analysis is not None:
            if not isinstance(analysis, dict):
                return jsonify(error="analysis must be an object"), 400
            analysis_path.parent.mkdir(parents=True, exist_ok=True)
            analysis_path.write_text(
                json.dumps(analysis, indent=2, ensure_ascii=False) + "\n",
                encoding="utf-8",
            )
        try:
            subprocess.run(
                [str(PIPELINE_SH), "analyze", str(tj), "--confirm"],
                check=True,
                capture_output=True,
                text=True,
                cwd=str(REPO_ROOT),
            )
        except subprocess.CalledProcessError as exc:
            return jsonify(error="confirm failed", detail=exc.stderr or exc.stdout), 400
        task = json.loads(tj.read_text("utf-8"))
        return jsonify(ok=True, analysis=_load_analysis(tid, task), task=task)

    # -- Run pipeline --------------------------------------------------------

    @app.post("/api/tasks/<task_id>/run")
    def run_task(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        tj = _task_json(tid)
        if not tj.is_file():
            return jsonify(error="not found"), 404

        with _lock:
            run = TASK_RUNS.get(tid, {})
            if run.get("status") == "running":
                return jsonify(error="task already running"), 409
            TASK_RUNS[tid] = {
                "status": "running",
                "log": [],
                "step": "init",
                "exit_code": None,
                "started": time.time(),
            }
            _write_run_state(tid, TASK_RUNS[tid])

        def _run():
            run_state = TASK_RUNS[tid]
            try:
                # run pipeline
                proc = subprocess.Popen(
                    [str(PIPELINE_SH), "run", str(tj)],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    cwd=str(REPO_ROOT),
                    start_new_session=True,
                )
                run_state["proc"] = proc
                for line in proc.stdout:  # type: ignore[union-attr]
                    line = line.rstrip("\n")
                    run_state["log"].append(line)
                    # Detect step changes from log output
                    ll = line.lower()
                    if "refin" in ll or "build-fix" in ll:
                        if run_state.get("step") != "refine":
                            run_state["log"].append("--- refining with LLM; this step can take several minutes ---")
                        run_state["step"] = "refine"
                    elif "analysis" in ll or "analyz" in ll:
                        run_state["step"] = "analyze"
                    elif "generat" in ll:
                        run_state["step"] = "generate"
                    elif "lint" in ll:
                        run_state["step"] = "lint"
                    elif "build" in ll or "cmake" in ll:
                        run_state["step"] = "build"
                    elif "validat" in ll or "screenshot" in ll:
                        run_state["step"] = "validate"
                    _write_run_state(tid, run_state)
                proc.wait()
                run_rc = proc.returncode

                if run_rc == 0:
                    # export
                    run_state["step"] = "export"
                    run_state["log"].append("--- exporting bundle ---")
                    ep = subprocess.run(
                        [str(PIPELINE_SH), "export", str(tj)],
                        capture_output=True, text=True, cwd=str(REPO_ROOT),
                    )
                    if ep.stdout:
                        run_state["log"].extend(ep.stdout.splitlines())
                    if ep.stderr:
                        run_state["log"].extend(ep.stderr.splitlines())
                    run_state["exit_code"] = ep.returncode
                    run_state["status"] = "done" if ep.returncode == 0 else "failed"
                    _write_run_state(tid, run_state)
                else:
                    run_state["exit_code"] = run_rc
                    run_state["status"] = "failed"
                    if run_rc == 2:
                        run_state["log"].append("--- validation threshold not met; artifacts are available for review ---")
                    _write_run_state(tid, run_state)
            except Exception as exc:
                run_state["log"].append(f"ERROR: {exc}")
                run_state["status"] = "failed"
                run_state["exit_code"] = -1
                _write_run_state(tid, run_state)

        threading.Thread(target=_run, daemon=True).start()
        return jsonify(status="started")

    # -- Stop task -----------------------------------------------------------

    @app.post("/api/tasks/<task_id>/stop")
    def stop_task(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        with _lock:
            run = TASK_RUNS.get(tid)
            if not run or run.get("status") != "running":
                return jsonify(error="task not running"), 409
        proc = run.get("proc")
        if proc and proc.poll() is None:
            # Kill the whole process group so child processes also die
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
            except (ProcessLookupError, OSError):
                proc.kill()
        run["log"].append("--- stopped by user ---")
        run["status"] = "stopped"
        run["exit_code"] = -15
        _write_run_state(tid, run)
        return jsonify(status="stopped")

    # -- SSE log stream ------------------------------------------------------

    @app.get("/api/tasks/<task_id>/stream")
    def stream_task(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return Response("invalid task id", status=400)

        def generate():
            cursor = 0
            last_step = ""
            last_heartbeat = 0.0
            while True:
                run = _reconcile_run_state(tid)
                if not run:
                    yield "event: done\ndata: {\"reason\":\"no_run\"}\n\n"
                    return
                log = run.get("log", [])
                while cursor < len(log):
                    yield f"event: log\ndata: {json.dumps(log[cursor])}\n\n"
                    cursor += 1
                step = run.get("step", "")
                if step != last_step:
                    last_step = step
                    yield f"event: step\ndata: {json.dumps(step)}\n\n"
                now = time.time()
                if now - last_heartbeat >= 2:
                    last_heartbeat = now
                    payload = {
                        "status": run.get("status", "idle"),
                        "step": step,
                        "updated": now,
                    }
                    yield f"event: heartbeat\ndata: {json.dumps(payload)}\n\n"
                if run["status"] in ("done", "failed", "stopped"):
                    payload = {"status": run["status"],
                               "exit_code": run.get("exit_code")}
                    yield f"event: done\ndata: {json.dumps(payload)}\n\n"
                    return
                time.sleep(0.3)

        return Response(generate(), mimetype="text/event-stream",
                        headers={"Cache-Control": "no-cache",
                                 "X-Accel-Buffering": "no"})

    # -- Artifacts & export --------------------------------------------------

    @app.get("/api/tasks/<task_id>/artifacts/<name>")
    def get_artifact(task_id: str, name: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        safe_name = Path(name).name  # strip any path components
        art_dir = _task_dir(tid) / "artifacts"
        fpath = art_dir / safe_name
        if not fpath.is_file():
            return jsonify(error="artifact not found"), 404
        return send_from_directory(str(art_dir), safe_name)

    @app.get("/api/tasks/<task_id>/export")
    def export_task(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        bundle_dir = _task_dir(tid) / "export" / "portable_bundle"
        if not bundle_dir.is_dir():
            return jsonify(error="export not available yet"), 404
        buf = BytesIO()
        with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as zf:
            for fp in sorted(bundle_dir.rglob("*")):
                if fp.is_file():
                    zf.write(fp, fp.relative_to(bundle_dir))
        buf.seek(0)
        return send_file(buf, mimetype="application/zip",
                         as_attachment=True,
                         download_name=f"{tid}_bundle.zip")

    # -- Generated code preview ----------------------------------------------

    @app.get("/api/tasks/<task_id>/code")
    def get_code(task_id: str):
        tid = _safe_task_id(task_id)
        if not tid:
            return jsonify(error="invalid task id"), 400
        gen_dir = _task_dir(tid) / "generated"
        files = {}
        if gen_dir.is_dir():
            for f in sorted(gen_dir.iterdir()):
                if f.suffix in (".c", ".h") and f.is_file():
                    files[f.name] = f.read_text("utf-8", errors="replace")
        return jsonify(files=files)

    return app
