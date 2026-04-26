#!/usr/bin/env python3

import argparse
import json
from pathlib import Path
from typing import Any, Dict

from PIL import Image
from PIL import ImageChops
from PIL import ImageDraw
from PIL import ImageStat


def load_task(task_path: Path) -> Dict[str, Any]:
    with task_path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def relative_to_task(task_path: Path, value: str) -> Path:
    project_dir = task_path.parent.parent.parent
    return (project_dir / value).resolve()


def create_binary_mask(diff_luma: Image.Image, threshold: int) -> Image.Image:
    return diff_luma.point(lambda value: 255 if value > threshold else 0)


def create_diff_preview(reference: Image.Image, current: Image.Image, heatmap: Image.Image) -> Image.Image:
    column_gap = 24
    header_height = 36
    width = reference.width * 3 + column_gap * 4
    height = reference.height + header_height + column_gap * 2

    canvas = Image.new("RGB", (width, height), (255, 255, 255))
    draw = ImageDraw.Draw(canvas)

    x_positions = [
        column_gap,
        column_gap * 2 + reference.width,
        column_gap * 3 + reference.width * 2,
    ]
    y_image = column_gap + header_height

    labels = ["Reference", "Current", "Heatmap"]
    images = [reference.convert("RGB"), current.convert("RGB"), heatmap.convert("RGB")]

    for label, image, x in zip(labels, images, x_positions):
        draw.text((x, column_gap), label, fill=(17, 24, 39))
        canvas.paste(image, (x, y_image))

    return canvas


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate an M1 page screenshot against a reference.")
    parser.add_argument("--task", required=True, help="Path to the task JSON file.")
    parser.add_argument("--current", required=True, help="Path to the current viewport screenshot.")
    parser.add_argument("--diff", required=True, help="Path to write the diff preview PNG.")
    parser.add_argument("--report", required=True, help="Path to write the JSON validation report.")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    current_path = Path(args.current).resolve()
    diff_path = Path(args.diff).resolve()
    report_path = Path(args.report).resolve()

    task = load_task(task_path)
    reference_path = relative_to_task(task_path, task["reference_image"])

    if not reference_path.is_file():
        raise SystemExit(f"Reference image not found: {reference_path}")
    if not current_path.is_file():
        raise SystemExit(f"Current screenshot not found: {current_path}")

    reference = Image.open(reference_path).convert("RGBA")
    current = Image.open(current_path).convert("RGBA")
    original_current_size = current.size

    size_match = reference.size == current.size
    resized_for_comparison = False
    if not size_match:
        current = current.resize(reference.size)
        resized_for_comparison = True

    diff = ImageChops.difference(reference, current)
    diff_rgb = diff.convert("RGB")
    diff_luma = diff_rgb.convert("L")
    mask = create_binary_mask(diff_luma, int(task["validation"]["pixel_diff_threshold"]))

    stat = ImageStat.Stat(diff_rgb)
    mean_abs_diff = sum(stat.mean[:3]) / 3.0
    bbox = mask.getbbox()
    changed_pixels = sum(mask.point(lambda value: 1 if value else 0).getdata())
    total_pixels = reference.width * reference.height
    diff_ratio = float(changed_pixels) / float(total_pixels)

    heatmap = Image.new("RGB", reference.size, (255, 255, 255))
    heatmap_draw = ImageDraw.Draw(heatmap)
    heatmap_draw.bitmap((0, 0), mask, fill=(239, 68, 68))

    preview = create_diff_preview(reference, current, heatmap)
    diff_path.parent.mkdir(parents=True, exist_ok=True)
    preview.save(diff_path)

    report = {
        "task": task["page_id"],
        "page_name": task["page_name"],
        "reference_image": str(reference_path),
        "current_image": str(current_path),
        "diff_image": str(diff_path),
        "size_match": size_match,
        "resized_for_comparison": resized_for_comparison,
        "reference_size": list(reference.size),
        "current_size": list(original_current_size),
        "changed_pixels": changed_pixels,
        "total_pixels": total_pixels,
        "diff_ratio": diff_ratio,
        "mean_abs_diff": mean_abs_diff,
        "diff_bbox": list(bbox) if bbox else None,
        "thresholds": {
            "pixel_diff_threshold": task["validation"]["pixel_diff_threshold"],
            "max_diff_ratio": task["validation"]["max_diff_ratio"],
            "max_mean_abs_diff": task["validation"]["max_mean_abs_diff"],
            "require_size_match": task["validation"]["require_size_match"],
        },
        "completion": task["completion"],
        "failure_policy": task["failure_policy"],
    }

    report["pass"] = (
        (size_match or not bool(task["validation"]["require_size_match"]))
        and diff_ratio <= float(task["validation"]["max_diff_ratio"])
        and mean_abs_diff <= float(task["validation"]["max_mean_abs_diff"])
    )
    report["completion_met"] = (
        report["pass"] if bool(task["completion"]["require_validation_pass"]) else True
    )
    report["failure_reason"] = None if report["completion_met"] else "validation_threshold_not_met"

    report_path.parent.mkdir(parents=True, exist_ok=True)
    with report_path.open("w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2, ensure_ascii=False)
        handle.write("\n")

    print(f"验证报告已写入 {report_path}")
    print(f"结果: {'通过' if report['pass'] else '未通过'}")
    print(f"完成状态: {'已满足' if report['completion_met'] else '未满足'}")
    print(f"diff_ratio: {diff_ratio:.6f}  (阈值: {task['validation']['max_diff_ratio']})")
    print(f"mean_abs_diff: {mean_abs_diff:.3f}  (阈值: {task['validation']['max_mean_abs_diff']})")
    if not report["pass"]:
        reasons = []
        if not (size_match or not bool(task["validation"]["require_size_match"])):
            reasons.append(f"尺寸不匹配 (参考: {reference.size}, 当前: {original_current_size})")
        if diff_ratio > float(task["validation"]["max_diff_ratio"]):
            reasons.append(f"diff_ratio 超标 ({diff_ratio:.6f} > {task['validation']['max_diff_ratio']})")
        if mean_abs_diff > float(task["validation"]["max_mean_abs_diff"]):
            reasons.append(f"mean_abs_diff 超标 ({mean_abs_diff:.3f} > {task['validation']['max_mean_abs_diff']})")
        print(f"未通过原因: {'; '.join(reasons)}")
    return 0 if report["completion_met"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
