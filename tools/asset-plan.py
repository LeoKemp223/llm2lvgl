#!/usr/bin/env python3

import argparse
import json
import re
from pathlib import Path
from typing import List, Tuple

from PIL import Image


Box = Tuple[int, int, int, int]


REGION_RE = re.compile(
    r"<!--\s*(?P<label>[^:]+): source box "
    r"(?P<x1>\d+),(?P<y1>\d+),(?P<x2>\d+),(?P<y2>\d+), avg (?P<avg>#[0-9a-fA-F]{6})\s*-->"
)


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def clamp_box(box: Box, width: int, height: int) -> Box:
    x1, y1, x2, y2 = box
    return (
        max(0, min(width, x1)),
        max(0, min(height, y1)),
        max(0, min(width, x2)),
        max(0, min(height, y2)),
    )


def scale_box(box: Box, from_size: Tuple[int, int], to_size: Tuple[int, int]) -> Box:
    fw, fh = from_size
    tw, th = to_size
    return (
        int(round(box[0] * tw / fw)),
        int(round(box[1] * th / fh)),
        int(round(box[2] * tw / fw)),
        int(round(box[3] * th / fh)),
    )


def pad_box(box: Box, pad: int, width: int, height: int) -> Box:
    x1, y1, x2, y2 = box
    return clamp_box((x1 - pad, y1 - pad, x2 + pad, y2 + pad), width, height)


def parse_regions(html_path: Path) -> List[dict]:
    if not html_path.is_file():
        return []
    content = html_path.read_text(encoding="utf-8", errors="replace")
    regions = []
    for index, match in enumerate(REGION_RE.finditer(content), start=1):
        regions.append({
            "label": match.group("label").strip(),
            "source_box": [
                int(match.group("x1")),
                int(match.group("y1")),
                int(match.group("x2")),
                int(match.group("y2")),
            ],
            "avg_color": match.group("avg").lower(),
            "order": index,
        })
    return regions


def role_for(label: str, order: int, box: Box, image_size: Tuple[int, int]) -> str:
    width, height = image_size
    x1, y1, x2, y2 = box
    bw = x2 - x1
    bh = y2 - y1
    low = label.lower()
    if "main panel" in low or (bw > width * 0.72 and bh > height * 0.58):
        return "device_shell"
    if y1 < height * 0.32 and bw > width * 0.45:
        return "top_display"
    if height * 0.42 <= y1 <= height * 0.68 and bw > width * 0.45:
        return "control_panel"
    if y1 > height * 0.60 and bw > width * 0.18:
        return "bottom_card"
    if bw < width * 0.22 and bh < height * 0.16:
        return "decorative_detail"
    return f"region_{order}"


def make_asset_id(role: str, order: int) -> str:
    clean = re.sub(r"[^a-z0-9_]+", "_", role.lower()).strip("_") or "asset"
    return f"{order:02d}_{clean}"


def build_plan(task_path: Path) -> dict:
    task = load_json(task_path)
    source_path = resolve(task_path, task["input"]["image_entry"])
    html_path = resolve(task_path, task["input"]["html_entry"])
    if not source_path.is_file():
        raise SystemExit(f"Image input not found: {source_path}")

    source = Image.open(source_path).convert("RGB")
    viewport = task["target"]["viewport"]
    viewport_size = (int(viewport["width"]), int(viewport["height"]))
    source_size = source.size

    regions = parse_regions(html_path)
    assets = []
    for region in regions:
        source_box = tuple(region["source_box"])  # type: ignore[arg-type]
        source_box = pad_box(source_box, 4, source_size[0], source_size[1])
        viewport_box = scale_box(source_box, source_size, viewport_size)
        role = role_for(region["label"], int(region["order"]), source_box, source_size)
        asset_id = make_asset_id(role, int(region["order"]))
        assets.append({
            "id": asset_id,
            "role": role,
            "mode": "crop",
            "source": task["input"]["image_entry"],
            "source_box": list(source_box),
            "viewport_box": list(viewport_box),
            "avg_color": region["avg_color"],
            "output": f"input/assets/generated/{asset_id}.png",
        })

    plan = {
        "version": 1,
        "task_id": task["task_id"],
        "source_image": task["input"]["image_entry"],
        "source_size": list(source_size),
        "viewport": viewport,
        "assets": assets,
    }
    return plan


def main() -> int:
    parser = argparse.ArgumentParser(description="Plan crop assets for image-based LVGL tasks.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    parser.add_argument("--out", help="Output asset plan JSON path")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    if task.get("input", {}).get("source_type") != "image":
        print("Task does not use image input. Nothing to do.")
        return 0

    plan = build_plan(task_path)
    out_path = Path(args.out).resolve() if args.out else task_path.parent / "artifacts" / "asset_plan.json"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(plan, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Asset plan written: {out_path}")
    print(f"Planned assets: {len(plan['assets'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
