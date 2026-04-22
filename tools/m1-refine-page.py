#!/usr/bin/env python3

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

import numpy as np
from PIL import Image


REPO_ROOT = Path(__file__).resolve().parent.parent
TOOLS_DIR = REPO_ROOT / "tools"
EPSILON_DIFF = 1e-6
EPSILON_MAD = 1e-3


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def split_args(raw: str) -> List[str]:
    args: List[str] = []
    current: List[str] = []
    in_string = False
    escape = False
    for ch in raw:
        if in_string:
            current.append(ch)
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
            current.append(ch)
            continue
        if ch == ",":
            args.append("".join(current).strip())
            current = []
            continue
        current.append(ch)
    if current:
        args.append("".join(current).strip())
    return args


def parse_int(value: str) -> Optional[int]:
    value = value.strip()
    if re.fullmatch(r"-?\d+", value):
        return int(value)
    return None


def is_identifier(value: str) -> bool:
    return bool(re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", value.strip()))


def clamp(value: int, low: int, high: int) -> int:
    return max(low, min(high, value))


def objective(report: dict) -> Tuple[int, float, float]:
    return (
        1 if bool(report.get("pass")) else 0,
        -float(report["diff_ratio"]),
        -float(report["mean_abs_diff"]),
    )


def better_than(candidate: dict, baseline: dict) -> bool:
    cand_obj = objective(candidate)
    base_obj = objective(baseline)
    if cand_obj[0] != base_obj[0]:
        return cand_obj[0] > base_obj[0]
    if float(candidate["diff_ratio"]) < float(baseline["diff_ratio"]) - EPSILON_DIFF:
        return True
    if abs(float(candidate["diff_ratio"]) - float(baseline["diff_ratio"])) <= EPSILON_DIFF:
        return float(candidate["mean_abs_diff"]) < float(baseline["mean_abs_diff"]) - EPSILON_MAD
    return False


@dataclass
class Candidate:
    name: str
    kind: str
    parent: Optional[str]
    line_idx: int
    indent: str
    func_name: str
    args: List[str]
    fields: Dict[str, int]
    rel_x: int
    rel_y: int
    width: int
    height: int
    abs_x: int
    abs_y: int
    score: float = 0.0

    def box(self, viewport: Tuple[int, int]) -> Tuple[int, int, int, int]:
        max_w, max_h = viewport
        x0 = clamp(self.abs_x, 0, max_w)
        y0 = clamp(self.abs_y, 0, max_h)
        x1 = clamp(self.abs_x + max(1, self.width), 0, max_w)
        y1 = clamp(self.abs_y + max(1, self.height), 0, max_h)
        return x0, y0, x1, y1

    def rendered_line(self) -> str:
        return f"{self.indent}{self.func_name}({', '.join(self.args)});"


def parse_create_body(lines: Sequence[str], page_id: str) -> Tuple[int, int]:
    start = -1
    end = -1
    target = f"lv_obj_t * {page_id}_page_create(void)"
    for idx, line in enumerate(lines):
        if target in line:
            start = idx
            break
    if start == -1:
        raise SystemExit(f"Could not locate create function for page: {page_id}")
    for idx in range(start, len(lines)):
        if "return screen;" in lines[idx]:
            end = idx
            break
    if end == -1:
        raise SystemExit(f"Could not locate end of create function for page: {page_id}")
    return start, end


def parse_parent_maps(body_lines: Sequence[str], body_start: int) -> Tuple[Dict[str, Optional[str]], Dict[str, int], Dict[str, int], Dict[str, int], Dict[str, int]]:
    parent_map: Dict[str, Optional[str]] = {"screen": None}
    pos_x: Dict[str, int] = {}
    pos_y: Dict[str, int] = {}
    width_map: Dict[str, int] = {}
    height_map: Dict[str, int] = {}

    create_re = re.compile(r"^\s*lv_obj_t \* (\w+) = \w+_create\(([^)]+)\);")
    assign_call_re = re.compile(r"^\s*lv_obj_t \* (\w+) = (\w+)\((.*)\);")
    set_pos_re = re.compile(r"^\s*lv_obj_set_pos\((\w+),\s*(-?\d+),\s*(-?\d+)\);")
    set_size_re = re.compile(r"^\s*lv_obj_set_size\((\w+),\s*(-?\d+),\s*(-?\d+)\);")

    for idx, line in enumerate(body_lines, start=body_start):
        match = create_re.match(line)
        if match:
            var_name = match.group(1)
            raw_parent = match.group(2).strip()
            parent_map[var_name] = raw_parent if is_identifier(raw_parent) else None
            continue

        match = assign_call_re.match(line)
        if match:
            var_name = match.group(1)
            func_name = match.group(2)
            if func_name.endswith("_create"):
                continue
            args = split_args(match.group(3))
            if args and is_identifier(args[0]):
                parent_map[var_name] = args[0]
            continue

        match = set_pos_re.match(line)
        if match:
            pos_x[match.group(1)] = int(match.group(2))
            pos_y[match.group(1)] = int(match.group(3))
            continue

        match = set_size_re.match(line)
        if match:
            width_map[match.group(1)] = int(match.group(2))
            height_map[match.group(1)] = int(match.group(3))

    return parent_map, pos_x, pos_y, width_map, height_map


def build_abs_pos(parent_map: Dict[str, Optional[str]], pos_x: Dict[str, int], pos_y: Dict[str, int]) -> Dict[str, Tuple[int, int]]:
    abs_pos: Dict[str, Tuple[int, int]] = {"screen": (0, 0)}

    def resolve(var_name: str) -> Optional[Tuple[int, int]]:
        if var_name in abs_pos:
            return abs_pos[var_name]
        parent = parent_map.get(var_name)
        if parent is None:
            return None
        parent_abs = resolve(parent)
        if parent_abs is None:
            return None
        x = pos_x.get(var_name)
        y = pos_y.get(var_name)
        if x is None or y is None:
            return None
        abs_pos[var_name] = (parent_abs[0] + x, parent_abs[1] + y)
        return abs_pos[var_name]

    for var_name in list(parent_map):
        resolve(var_name)
    return abs_pos


def helper_candidate(
    line_idx: int,
    line: str,
    abs_pos: Dict[str, Tuple[int, int]],
) -> Optional[Candidate]:
    match = re.match(r"^(\s*)(\w+)\((.*)\);\s*$", line)
    if not match:
        return None

    indent, func_name, raw_args = match.groups()
    args = split_args(raw_args)
    if not args:
        return None

    parent = args[0].strip() if is_identifier(args[0]) else None
    parent_abs = abs_pos.get(parent, (0, 0)) if parent else (0, 0)
    name = f"{func_name}@{line_idx + 1}"
    fields: Dict[str, int] = {}
    rel_x: Optional[int] = None
    rel_y: Optional[int] = None
    width: Optional[int] = None
    height: Optional[int] = None

    if func_name == "make_chip" and len(args) >= 6:
        rel_x = parse_int(args[2])
        rel_y = parse_int(args[3])
        width = parse_int(args[4])
        height = parse_int(args[5])
        fields = {"x": 2, "y": 3, "w": 4, "h": 5}
    elif func_name == "make_icon_badge" and len(args) >= 5:
        rel_x = parse_int(args[2])
        rel_y = parse_int(args[3])
        size = parse_int(args[4])
        width = size
        height = size
        fields = {"x": 2, "y": 3, "size": 4}
    elif func_name == "make_round_button" and len(args) >= 4:
        rel_x = parse_int(args[2])
        rel_y = parse_int(args[3])
        width = 40
        height = 40
        fields = {"x": 2, "y": 3}
    elif func_name == "make_action_button" and len(args) >= 5:
        rel_x = parse_int(args[2])
        rel_y = parse_int(args[3])
        width = parse_int(args[4])
        height = 30
        fields = {"x": 2, "y": 3, "w": 4}
    elif func_name == "make_footer_item" and len(args) >= 4:
        rel_x = parse_int(args[3])
        rel_y = 7
        active = args[4].strip().lower() == "true" if len(args) >= 5 else False
        width = 92 if active else 72
        height = 42
        fields = {"x": 3}
    elif func_name == "make_action_card" and len(args) >= 3:
        rel_x = parse_int(args[1])
        rel_y = parse_int(args[2])
        width = 74
        height = 92
        fields = {"x": 1, "y": 2}
    elif func_name == "make_icon_box" and len(args) >= 5:
        rel_x = parse_int(args[1])
        rel_y = parse_int(args[2])
        width = parse_int(args[3])
        height = parse_int(args[4])
        fields = {"x": 1, "y": 2, "w": 3, "h": 4}
    else:
        return None

    if rel_x is None or rel_y is None or width is None or height is None:
        return None

    return Candidate(
        name=name,
        kind=func_name,
        parent=parent,
        line_idx=line_idx,
        indent=indent,
        func_name=func_name,
        args=args,
        fields=fields,
        rel_x=rel_x,
        rel_y=rel_y,
        width=width,
        height=height,
        abs_x=parent_abs[0] + rel_x,
        abs_y=parent_abs[1] + rel_y,
    )


def direct_candidates(
    body_lines: Sequence[str],
    body_start: int,
    parent_map: Dict[str, Optional[str]],
    pos_x: Dict[str, int],
    pos_y: Dict[str, int],
    width_map: Dict[str, int],
    height_map: Dict[str, int],
    abs_pos: Dict[str, Tuple[int, int]],
) -> List[Candidate]:
    candidates: List[Candidate] = []
    set_pos_re = re.compile(r"^(\s*)lv_obj_set_pos\((\w+),\s*(-?\d+),\s*(-?\d+)\);\s*$")
    set_size_re = re.compile(r"^(\s*)lv_obj_set_size\((\w+),\s*(-?\d+),\s*(-?\d+)\);\s*$")

    pos_line_map: Dict[str, int] = {}
    indent_map: Dict[str, str] = {}
    size_line_map: Dict[str, int] = {}
    for idx, line in enumerate(body_lines, start=body_start):
        pos_match = set_pos_re.match(line)
        if pos_match:
            pos_line_map[pos_match.group(2)] = idx
            indent_map[pos_match.group(2)] = pos_match.group(1)
        size_match = set_size_re.match(line)
        if size_match:
            size_line_map[size_match.group(2)] = idx
            indent_map[size_match.group(2)] = size_match.group(1)

    for var_name, line_idx in pos_line_map.items():
        if var_name == "screen":
            continue
        if var_name not in width_map or var_name not in height_map:
            continue
        if var_name not in abs_pos:
            continue
        parent = parent_map.get(var_name)
        if parent is None:
            continue
        candidates.append(
            Candidate(
                name=var_name,
                kind="lv_obj",
                parent=parent,
                line_idx=line_idx,
                indent=indent_map[var_name],
                func_name="lv_obj_set_pos",
                args=[var_name, str(pos_x[var_name]), str(pos_y[var_name])],
                fields={"x": 1, "y": 2},
                rel_x=pos_x[var_name],
                rel_y=pos_y[var_name],
                width=width_map[var_name],
                height=height_map[var_name],
                abs_x=abs_pos[var_name][0],
                abs_y=abs_pos[var_name][1],
            )
        )
    return candidates


def collect_candidates(source_path: Path, page_id: str, viewport: Tuple[int, int]) -> Tuple[List[str], List[Candidate]]:
    lines = source_path.read_text(encoding="utf-8").splitlines()
    body_start, body_end = parse_create_body(lines, page_id)
    body_lines = lines[body_start:body_end + 1]

    parent_map, pos_x, pos_y, width_map, height_map = parse_parent_maps(body_lines, body_start)
    abs_pos = build_abs_pos(parent_map, pos_x, pos_y)

    candidates = direct_candidates(body_lines, body_start, parent_map, pos_x, pos_y, width_map, height_map, abs_pos)
    for idx in range(body_start, body_end + 1):
        helper = helper_candidate(idx, lines[idx], abs_pos)
        if helper is not None:
            candidates.append(helper)

    filtered: List[Candidate] = []
    seen = set()
    for candidate in candidates:
        box = candidate.box(viewport)
        if box[2] <= box[0] or box[3] <= box[1]:
            continue
        key = (candidate.line_idx, candidate.kind, candidate.name)
        if key in seen:
            continue
        seen.add(key)
        filtered.append(candidate)
    return lines, filtered


def load_heatmap_mask(diff_path: Path, report: dict) -> np.ndarray:
    preview = Image.open(diff_path).convert("RGB")
    ref_w, ref_h = report["reference_size"]
    column_gap = 24
    header_height = 36
    x0 = column_gap * 3 + ref_w * 2
    y0 = column_gap + header_height
    heatmap = preview.crop((x0, y0, x0 + ref_w, y0 + ref_h))
    data = np.asarray(heatmap)
    return (data[:, :, 0] > 200) & (data[:, :, 1] < 120) & (data[:, :, 2] < 120)


def score_candidates(candidates: List[Candidate], heatmap_mask: np.ndarray, viewport: Tuple[int, int]) -> List[Candidate]:
    for candidate in candidates:
        x0, y0, x1, y1 = candidate.box(viewport)
        region = heatmap_mask[y0:y1, x0:x1]
        if region.size == 0:
            candidate.score = 0.0
            continue
        candidate.score = float(region.sum())
    return sorted(candidates, key=lambda item: item.score, reverse=True)


def set_candidate_value(candidate: Candidate, field: str, new_value: int) -> Candidate:
    args = list(candidate.args)
    args[candidate.fields[field]] = str(new_value)
    rel_x = candidate.rel_x
    rel_y = candidate.rel_y
    width = candidate.width
    height = candidate.height
    abs_x = candidate.abs_x
    abs_y = candidate.abs_y

    if field == "x":
        rel_x = new_value
        abs_x = abs_x - candidate.rel_x + new_value
    elif field == "y":
        rel_y = new_value
        abs_y = abs_y - candidate.rel_y + new_value
    elif field == "w":
        width = new_value
    elif field == "h":
        height = new_value
    elif field == "size":
        width = new_value
        height = new_value

    return Candidate(
        name=candidate.name,
        kind=candidate.kind,
        parent=candidate.parent,
        line_idx=candidate.line_idx,
        indent=candidate.indent,
        func_name=candidate.func_name,
        args=args,
        fields=dict(candidate.fields),
        rel_x=rel_x,
        rel_y=rel_y,
        width=width,
        height=height,
        abs_x=abs_x,
        abs_y=abs_y,
        score=candidate.score,
    )


def apply_candidate(lines: List[str], candidate: Candidate) -> List[str]:
    updated = list(lines)
    updated[candidate.line_idx] = candidate.rendered_line()
    return updated


def variant_values(candidate: Candidate, field: str, viewport: Tuple[int, int]) -> List[int]:
    current = {
        "x": candidate.rel_x,
        "y": candidate.rel_y,
        "w": candidate.width,
        "h": candidate.height,
        "size": candidate.width,
    }[field]
    deltas = [-8, -4, 4, 8]
    values: List[int] = []
    for delta in deltas:
        value = current + delta
        if field in {"w", "h", "size"}:
            if value < 8:
                continue
            if field == "w" or field == "size":
                if candidate.abs_x + value > viewport[0]:
                    continue
            if field == "h" or field == "size":
                if candidate.abs_y + value > viewport[1]:
                    continue
        elif field == "x":
            if candidate.abs_x - candidate.rel_x + value < 0:
                continue
            if candidate.abs_x - candidate.rel_x + value + candidate.width > viewport[0]:
                continue
        elif field == "y":
            if candidate.abs_y - candidate.rel_y + value < 0:
                continue
            if candidate.abs_y - candidate.rel_y + value + candidate.height > viewport[1]:
                continue
        values.append(value)
    return values


def run_task(task_path: Path, fast: bool = True) -> int:
    cmd = [
        sys.executable,
        str(TOOLS_DIR / "m1-task-run.py"),
        "--task",
        str(task_path),
    ]
    if fast:
        cmd.extend([
            "--skip-sync",
            "--skip-configure",
            "--skip-full-screenshot",
        ])
    result = subprocess.run(
        cmd,
        check=False,
        cwd=str(REPO_ROOT),
    )
    return result.returncode


def read_artifacts(task_path: Path) -> Tuple[Path, Path, Path]:
    artifacts_dir = task_path.parent / "artifacts"
    return artifacts_dir / "report.json", artifacts_dir / "diff.png", artifacts_dir / "refine_log.json"


def trial_candidates(
    task_path: Path,
    source_path: Path,
    lines: List[str],
    ranked_candidates: Sequence[Candidate],
    baseline_report: dict,
    viewport: Tuple[int, int],
    max_candidates: int,
) -> Tuple[Optional[List[str]], Optional[dict], List[dict]]:
    best_source: Optional[List[str]] = None
    best_report: Optional[dict] = None
    trials: List[dict] = []
    original_source = list(lines)

    for candidate in ranked_candidates[:max_candidates]:
        if candidate.score <= 0:
            continue
        for field in ("x", "y", "w", "h", "size"):
            if field not in candidate.fields:
                continue
            for value in variant_values(candidate, field, viewport):
                updated_candidate = set_candidate_value(candidate, field, value)
                updated_source = apply_candidate(original_source, updated_candidate)
                source_path.write_text("\n".join(updated_source) + "\n", encoding="utf-8")
                status = run_task(task_path, fast=True)
                if status not in (0, 2):
                    source_path.write_text("\n".join(original_source) + "\n", encoding="utf-8")
                    trials.append({
                        "candidate": candidate.name,
                        "field": field,
                        "value": value,
                        "status": status,
                        "accepted": False,
                    })
                    continue

                report_path, _, _ = read_artifacts(task_path)
                report = load_json(report_path)
                accepted = best_report is None and better_than(report, baseline_report)
                if best_report is not None and better_than(report, best_report):
                    accepted = True

                trials.append({
                    "candidate": candidate.name,
                    "field": field,
                    "value": value,
                    "status": status,
                    "diff_ratio": float(report["diff_ratio"]),
                    "mean_abs_diff": float(report["mean_abs_diff"]),
                    "accepted": accepted,
                })

                if accepted:
                    best_report = report
                    best_source = updated_source

                source_path.write_text("\n".join(original_source) + "\n", encoding="utf-8")

    return best_source, best_report, trials


def main() -> int:
    parser = argparse.ArgumentParser(description="Run automatic LVGL source refinement until validation passes or max_iterations is reached.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    parser.add_argument("--max-candidates", type=int, default=2, help="Maximum high-diff candidates to try per iteration")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    source_path = resolve(task_path, task["generation"]["output_c"])
    if not source_path.is_file():
        raise SystemExit(f"Generated source file not found: {source_path}")

    viewport = (
        int(task["target"]["viewport"]["width"]),
        int(task["target"]["viewport"]["height"]),
    )
    max_iterations = int(task["failure_policy"]["max_iterations"])
    report_path, diff_path, log_path = read_artifacts(task_path)

    if not report_path.is_file() or not diff_path.is_file():
        status = run_task(task_path, fast=False)
        if status not in (0, 2):
            return status

    log: Dict[str, object] = {
        "task": task["task_id"],
        "source": str(source_path),
        "iterations": [],
    }

    for iteration in range(1, max_iterations + 1):
        report = load_json(report_path)
        if bool(report.get("pass")):
            print(f"Refine skipped: validation already passes for {task_path}")
            log_path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            return 0

        if not diff_path.is_file():
            raise SystemExit(f"Diff preview not found: {diff_path}")

        lines, candidates = collect_candidates(source_path, task["page_id"], viewport)
        heatmap_mask = load_heatmap_mask(diff_path, report)
        ranked = score_candidates(candidates, heatmap_mask, viewport)

        iteration_log = {
            "iteration": iteration,
            "baseline": {
                "diff_ratio": float(report["diff_ratio"]),
                "mean_abs_diff": float(report["mean_abs_diff"]),
                "pass": bool(report["pass"]),
            },
            "candidates": [
                {
                    "name": candidate.name,
                    "kind": candidate.kind,
                    "score": candidate.score,
                    "box": list(candidate.box(viewport)),
                }
                for candidate in ranked[:args.max_candidates]
            ],
        }

        best_source, best_report, trials = trial_candidates(
            task_path=task_path,
            source_path=source_path,
            lines=lines,
            ranked_candidates=ranked,
            baseline_report=report,
            viewport=viewport,
            max_candidates=args.max_candidates,
        )
        iteration_log["trials"] = trials

        if best_source is None or best_report is None:
            source_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
            iteration_log["accepted"] = None
            log["iterations"].append(iteration_log)
            log_path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            print(f"Refine stopped without improvement at iteration {iteration}")
            return 2

        source_path.write_text("\n".join(best_source) + "\n", encoding="utf-8")
        status = run_task(task_path, fast=True)
        if status not in (0, 2):
            return status

        refreshed_report = load_json(report_path)
        iteration_log["accepted"] = {
            "diff_ratio": float(refreshed_report["diff_ratio"]),
            "mean_abs_diff": float(refreshed_report["mean_abs_diff"]),
            "pass": bool(refreshed_report["pass"]),
        }
        log["iterations"].append(iteration_log)
        log_path.write_text(json.dumps(log, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

        print(
            "Refine iteration "
            f"{iteration}: diff_ratio={refreshed_report['diff_ratio']:.6f}, "
            f"mean_abs_diff={refreshed_report['mean_abs_diff']:.3f}, "
            f"pass={refreshed_report['pass']}"
        )

        if bool(refreshed_report.get("pass")):
            return 0

    final_report = load_json(report_path)
    print(
        "Refine reached max_iterations without passing: "
        f"diff_ratio={final_report['diff_ratio']:.6f}, "
        f"mean_abs_diff={final_report['mean_abs_diff']:.3f}"
    )
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
