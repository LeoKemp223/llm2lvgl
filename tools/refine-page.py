#!/usr/bin/env python3

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, Tuple

# Allow importing llm_client from the same directory
sys.path.insert(0, str(Path(__file__).resolve().parent))
import llm_client


REPO_ROOT = Path(__file__).resolve().parent.parent
TOOLS_DIR = REPO_ROOT / "tools"
PROMPTS_DIR = TOOLS_DIR / "prompts"


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def better_than(candidate: dict, baseline: dict) -> bool:
    c_pass = 1 if bool(candidate.get("pass")) else 0
    b_pass = 1 if bool(baseline.get("pass")) else 0
    if c_pass != b_pass:
        return c_pass > b_pass
    if float(candidate["diff_ratio"]) < float(baseline["diff_ratio"]) - 1e-6:
        return True
    if abs(float(candidate["diff_ratio"]) - float(baseline["diff_ratio"])) <= 1e-6:
        return float(candidate["mean_abs_diff"]) < float(baseline["mean_abs_diff"]) - 1e-3
    return False


def run_task(task_path: Path, fast: bool = True) -> Tuple[int, str]:
    """Run task and return (exit_code, combined_output). Output is streamed in real-time."""
    cmd = [
        sys.executable, "-u",
        str(TOOLS_DIR / "task-run.py"),
        "--task",
        str(task_path),
    ]
    if fast:
        cmd.extend([
            "--skip-sync",
            "--skip-configure",
            "--skip-full-screenshot",
        ])
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        cwd=str(REPO_ROOT),
    )
    lines: list[str] = []
    for line in proc.stdout:  # type: ignore[union-attr]
        line = line.rstrip("\n")
        lines.append(line)
        print(line, flush=True)
    proc.wait()
    return proc.returncode, "\n".join(lines)


def read_artifacts(task_path: Path) -> Tuple[Path, Path, Path]:
    artifacts_dir = task_path.parent / "artifacts"
    return artifacts_dir / "report.json", artifacts_dir / "diff.png", artifacts_dir / "refine_log.json"


def build_refine_prompt(
    source_code: str,
    report: dict,
    diff_path: Path,
) -> list:
    """Build the messages list for the LLM refinement call (with vision).

    Only sends diff.png which is a three-panel composite (Reference | Current | Heatmap),
    avoiding redundant transmission of separate reference and current screenshots.
    """
    system_template = PROMPTS_DIR.joinpath("refine_page.md").read_text("utf-8")

    user_content: list = []

    # Text: current code + report metrics
    user_content.append({
        "type": "text",
        "text": (
            f"## Validation Report\n"
            f"- diff_ratio: {report['diff_ratio']}\n"
            f"- mean_abs_diff: {report['mean_abs_diff']}\n"
            f"- pass: {report['pass']}\n\n"
            f"## Current C Source\n```c\n{source_code}\n```"
        ),
    })

    # Image: diff.png is a three-panel composite (Reference | Current | Heatmap)
    if diff_path.is_file():
        b64 = llm_client.image_to_base64(diff_path)
        suffix = diff_path.suffix.lstrip(".").lower()
        media_type = f"image/{'jpeg' if suffix in ('jpg', 'jpeg') else 'png'}"
        user_content.append({
            "type": "text",
            "text": "\n### Visual comparison (Reference | Current | Diff heatmap — red = mismatch)",
        })
        user_content.append({
            "type": "image_url",
            "image_url": {"url": f"data:{media_type};base64,{b64}"},
        })

    return [
        {"role": "system", "content": system_template},
        {"role": "user", "content": user_content},
    ]


def _strip_build_noise(build_output: str) -> str:
    """Strip cmake progress lines, make errors, tracebacks — keep only compiler diagnostics."""
    lines = build_output.splitlines()
    kept = []
    in_traceback = False
    for line in lines:
        stripped = line.strip()
        # Skip cmake progress lines like "[ 98%] Building ..."
        if re.match(r'^\[\s*\d+%\]', stripped):
            continue
        # Skip make directory/entering/error lines
        if stripped.startswith(("make[", "make:", "Scanning ")):
            continue
        # Skip Python tracebacks from task-run
        if stripped.startswith("Traceback (most recent"):
            in_traceback = True
            continue
        if in_traceback:
            if stripped.startswith(("File ", "raise ", "subprocess.")) or not stripped or line.startswith(" "):
                continue
            in_traceback = False
        # Skip empty lines
        if not stripped:
            continue
        kept.append(line)
    result = "\n".join(kept)
    # Cap at 2000 chars
    if len(result) > 2000:
        result = result[:2000] + "\n... (truncated)"
    return result


def build_build_error_prompt(source_code: str, build_output: str) -> list:
    """Build a prompt to fix compilation errors."""
    system_template = PROMPTS_DIR.joinpath("refine_page.md").read_text("utf-8")
    errors = _strip_build_noise(build_output)
    user_content = (
        f"## Build Error\n"
        f"The following LVGL C code failed to compile. Fix ALL compilation errors.\n\n"
        f"Use search-replace blocks to express your fix:\n"
        f"```\n<<<SEARCH\noriginal lines\n===\nfixed lines\n>>>\n```\n"
        f"If the fix is very large, you may output the complete file in a ```c fence instead.\n\n"
        f"### Compiler Errors\n```\n{errors}\n```\n\n"
        f"### Current C Source\n```c\n{source_code}\n```"
    )
    return [
        {"role": "system", "content": system_template},
        {"role": "user", "content": user_content},
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description="Run LLM-based LVGL source refinement.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    source_path = resolve(task_path, task["generation"]["output_c"])
    if not source_path.is_file():
        raise SystemExit(f"Generated source file not found: {source_path}")

    if not (os.environ.get("OPENAI_API_KEY") or llm_client.load_settings().get("api_key")):
        raise SystemExit("OPENAI_API_KEY not set. Configure it in Web UI settings or set the environment variable.")

    max_iterations = int(task["failure_policy"]["max_iterations"])
    report_path, diff_path, log_path = read_artifacts(task_path)
    artifacts_dir = task_path.parent / "artifacts"

    if not report_path.is_file() or not diff_path.is_file():
        status, build_output = run_task(task_path, fast=False)
        if status not in (0, 2):
            # Build failed — try to fix compilation errors via LLM
            print(f"初始构建失败 (退出码 {status})，尝试修复构建错误...")
            for fix_attempt in range(1, max_iterations + 1):
                baseline_code = source_path.read_text(encoding="utf-8")
                messages = build_build_error_prompt(baseline_code, build_output)
                print(f"构建修复 第 {fix_attempt} 轮: 调用 LLM...")
                response = llm_client.chat(messages, max_tokens=4096)
                blocks = llm_client.extract_search_replace_blocks(response)
                if blocks:
                    try:
                        new_code = llm_client.apply_search_replace(baseline_code, blocks)
                    except ValueError as e:
                        print(f"构建修复 第 {fix_attempt} 轮: search-replace 失败: {e}")
                        continue
                else:
                    new_code = llm_client.extract_code_block(response, "c")
                source_path.write_text(new_code, encoding="utf-8")
                status, build_output = run_task(task_path, fast=True)
                if status in (0, 2):
                    print(f"构建修复 第 {fix_attempt} 轮: 构建成功")
                    break
                print(f"构建修复 第 {fix_attempt} 轮: 仍然失败")
            if status not in (0, 2):
                print("重试后仍无法修复构建错误")
                return status

    # Guard: if report.json still doesn't exist after build succeeded,
    # validation was skipped (e.g. missing reference image). Nothing to refine.
    if not report_path.is_file():
        print(f"未找到验证报告 {report_path} — "
              "验证可能被跳过（缺少参考图片？）。"
              "没有基准报告无法进行精调。")
        return 0 if diff_path.is_file() else 1

    log: Dict[str, object] = {
        "task": task["task_id"],
        "source": str(source_path),
        "method": "llm_vision",
        "iterations": [],
    }

    consecutive_no_improve = 0
    max_no_improve = 3

    for iteration in range(1, max_iterations + 1):
        report = load_json(report_path)
        if bool(report.get("pass")):
            print(f"精调跳过: 验证已通过 {task_path}")
            log_path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            return 0

        if not diff_path.is_file():
            raise SystemExit(f"Diff preview not found: {diff_path}")

        # O1: Backup artifacts so rollback can restore them without rebuilding
        baseline_artifacts: Dict[str, bytes] = {}
        for artifact_name in ("report.json", "diff.png", "current.png"):
            artifact_file = artifacts_dir / artifact_name
            if artifact_file.is_file():
                baseline_artifacts[artifact_name] = artifact_file.read_bytes()

        baseline_code = source_path.read_text(encoding="utf-8")
        iteration_log: Dict[str, object] = {
            "iteration": iteration,
            "baseline": {
                "diff_ratio": float(report["diff_ratio"]),
                "mean_abs_diff": float(report["mean_abs_diff"]),
                "pass": bool(report["pass"]),
            },
        }

        # Call LLM with vision
        print(f"精调 第 {iteration} 轮: 调用 LLM...")
        messages = build_refine_prompt(
            baseline_code, report, diff_path,
        )
        response = llm_client.chat(messages, max_tokens=4096)
        blocks = llm_client.extract_search_replace_blocks(response)
        if blocks:
            try:
                new_code = llm_client.apply_search_replace(baseline_code, blocks)
            except ValueError as e:
                print(f"精调 第 {iteration} 轮: search-replace 失败: {e}，已回滚")
                iteration_log["result"] = "search_replace_failed"
                log["iterations"].append(iteration_log)
                log_path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
                consecutive_no_improve += 1
                if consecutive_no_improve >= max_no_improve:
                    print(f"精调 第 {iteration} 轮: 连续 {max_no_improve} 次失败，提前停止")
                    break
                continue
        else:
            new_code = llm_client.extract_code_block(response, "c")

        # Write new code and rebuild
        source_path.write_text(new_code, encoding="utf-8")
        status, build_err = run_task(task_path, fast=True)
        if status not in (0, 2):
            # Build failed — try to fix via LLM before rolling back
            print(f"精调 第 {iteration} 轮: 构建失败，尝试修复...")
            for fix_try in range(1, max_iterations + 1):
                fix_msgs = build_build_error_prompt(new_code, build_err)
                fix_resp = llm_client.chat(fix_msgs, max_tokens=4096)
                fix_blocks = llm_client.extract_search_replace_blocks(fix_resp)
                if fix_blocks:
                    try:
                        new_code = llm_client.apply_search_replace(new_code, fix_blocks)
                    except ValueError:
                        new_code = llm_client.extract_code_block(fix_resp, "c")
                else:
                    new_code = llm_client.extract_code_block(fix_resp, "c")
                source_path.write_text(new_code, encoding="utf-8")
                status, build_err = run_task(task_path, fast=True)
                if status in (0, 2):
                    print(f"精调 第 {iteration} 轮: 构建修复成功 (第 {fix_try} 次尝试)")
                    break
            if status not in (0, 2):
                # Still failing — rollback code and artifacts without rebuilding
                source_path.write_text(baseline_code, encoding="utf-8")
                for name, data in baseline_artifacts.items():
                    (artifacts_dir / name).write_bytes(data)
                iteration_log["result"] = "build_failed"
                log["iterations"].append(iteration_log)
                log_path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
                consecutive_no_improve += 1
                if consecutive_no_improve >= max_no_improve:
                    print(f"精调 第 {iteration} 轮: 连续 {max_no_improve} 次失败，提前停止")
                    break
                print(f"精调 第 {iteration} 轮: 无法修复构建，已回滚")
                continue

        new_report = load_json(report_path)
        iteration_log["result"] = {
            "diff_ratio": float(new_report["diff_ratio"]),
            "mean_abs_diff": float(new_report["mean_abs_diff"]),
            "pass": bool(new_report["pass"]),
        }

        if not better_than(new_report, report):
            # Got worse — rollback code and artifacts without rebuilding
            source_path.write_text(baseline_code, encoding="utf-8")
            for name, data in baseline_artifacts.items():
                (artifacts_dir / name).write_bytes(data)
            iteration_log["accepted"] = False
            log["iterations"].append(iteration_log)
            log_path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            consecutive_no_improve += 1
            if consecutive_no_improve >= max_no_improve:
                print(f"精调 第 {iteration} 轮: 连续 {max_no_improve} 次未改善，提前停止")
                break
            print(f"精调 第 {iteration} 轮: 无改善，已回滚")
            continue

        consecutive_no_improve = 0

        iteration_log["accepted"] = True
        log["iterations"].append(iteration_log)
        log_path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

        print(
            f"精调 第 {iteration} 轮: "
            f"diff_ratio={new_report['diff_ratio']:.6f}, "
            f"mean_abs_diff={new_report['mean_abs_diff']:.3f}, "
            f"通过={'是' if new_report['pass'] else '否'}"
        )

        if bool(new_report.get("pass")):
            return 0

    final_report = load_json(report_path)
    print(
        f"精调已达最大迭代次数 ({max_iterations}) 仍未通过: "
        f"diff_ratio={final_report['diff_ratio']:.6f}, "
        f"mean_abs_diff={final_report['mean_abs_diff']:.3f}"
    )
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
