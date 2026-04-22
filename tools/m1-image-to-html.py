#!/usr/bin/env python3

import argparse
import html
import json
import math
from collections import deque
from pathlib import Path
from typing import List, Sequence, Tuple

from PIL import Image, ImageStat


RGB = Tuple[int, int, int]
Box = Tuple[int, int, int, int]


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def clamp(value: int, low: int, high: int) -> int:
    return max(low, min(high, value))


def avg_rgb(image: Image.Image) -> RGB:
    stat = ImageStat.Stat(image.convert("RGB"))
    return tuple(int(round(channel)) for channel in stat.mean[:3])  # type: ignore[return-value]


def rgb_to_hex(rgb: RGB) -> str:
    return "#{:02x}{:02x}{:02x}".format(*rgb)


def color_distance(a: RGB, b: RGB) -> float:
    return math.sqrt(sum((ax - bx) ** 2 for ax, bx in zip(a, b)))


def luminance(rgb: RGB) -> float:
    return 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]


def guess_background(image: Image.Image) -> RGB:
    width, height = image.size
    box_w = max(8, width // 8)
    box_h = max(8, height // 8)
    corners = [
        (0, 0, box_w, box_h),
        (width - box_w, 0, width, box_h),
        (0, height - box_h, box_w, height),
        (width - box_w, height - box_h, width, height),
    ]
    samples = [avg_rgb(image.crop(box)) for box in corners]
    return tuple(int(round(sum(channel) / len(samples))) for channel in zip(*samples))  # type: ignore[return-value]


def build_foreground_mask(image: Image.Image, background: RGB) -> Tuple[List[List[bool]], int]:
    width, height = image.size
    scale = max(1, math.ceil(max(width, height) / 96))
    coarse = image.resize(
        (max(1, width // scale), max(1, height // scale)),
        Image.Resampling.BOX,
    ).convert("RGB")
    coarse_w, coarse_h = coarse.size

    distances: List[float] = []
    pixels = coarse.load()
    for y in range(coarse_h):
        for x in range(coarse_w):
            distances.append(color_distance(pixels[x, y], background))

    mean_dist = sum(distances) / len(distances) if distances else 0.0
    max_dist = max(distances) if distances else 0.0
    threshold = max(22.0, mean_dist + (max_dist - mean_dist) * 0.22)

    mask: List[List[bool]] = []
    for y in range(coarse_h):
        row: List[bool] = []
        for x in range(coarse_w):
            row.append(color_distance(pixels[x, y], background) >= threshold)
        mask.append(row)

    cleaned = [[False for _ in range(coarse_w)] for _ in range(coarse_h)]
    for y in range(coarse_h):
        for x in range(coarse_w):
            neighbors = 0
            for ny in range(max(0, y - 1), min(coarse_h, y + 2)):
                for nx in range(max(0, x - 1), min(coarse_w, x + 2)):
                    if nx == x and ny == y:
                        continue
                    if mask[ny][nx]:
                        neighbors += 1
            if mask[y][x]:
                cleaned[y][x] = neighbors >= 2
            else:
                cleaned[y][x] = neighbors >= 5

    return cleaned, scale


def find_components(mask: Sequence[Sequence[bool]]) -> List[Box]:
    if not mask or not mask[0]:
        return []

    height = len(mask)
    width = len(mask[0])
    visited = [[False for _ in range(width)] for _ in range(height)]
    min_area = max(6, int(width * height * 0.006))
    boxes: List[Box] = []

    for y in range(height):
        for x in range(width):
            if not mask[y][x] or visited[y][x]:
                continue

            queue = deque([(x, y)])
            visited[y][x] = True
            min_x = max_x = x
            min_y = max_y = y
            area = 0

            while queue:
                cx, cy = queue.popleft()
                area += 1
                min_x = min(min_x, cx)
                max_x = max(max_x, cx)
                min_y = min(min_y, cy)
                max_y = max(max_y, cy)
                for nx, ny in ((cx - 1, cy), (cx + 1, cy), (cx, cy - 1), (cx, cy + 1)):
                    if 0 <= nx < width and 0 <= ny < height and mask[ny][nx] and not visited[ny][nx]:
                        visited[ny][nx] = True
                        queue.append((nx, ny))

            box_w = max_x - min_x + 1
            box_h = max_y - min_y + 1
            if area < min_area:
                continue
            if box_w < 4 and box_h < 4:
                continue
            boxes.append((min_x, min_y, max_x + 1, max_y + 1))

    return boxes


def boxes_close(a: Box, b: Box, gap: int = 3) -> bool:
    ax1, ay1, ax2, ay2 = a
    bx1, by1, bx2, by2 = b
    horizontal_overlap = min(ax2, bx2) - max(ax1, bx1)
    vertical_overlap = min(ay2, by2) - max(ay1, by1)
    return (
        horizontal_overlap >= -gap and vertical_overlap >= -gap
    )


def merge_boxes(boxes: Sequence[Box]) -> List[Box]:
    pending = list(boxes)
    merged: List[Box] = []

    while pending:
        current = pending.pop(0)
        changed = True
        while changed:
            changed = False
            rest: List[Box] = []
            for candidate in pending:
                if boxes_close(current, candidate):
                    current = (
                        min(current[0], candidate[0]),
                        min(current[1], candidate[1]),
                        max(current[2], candidate[2]),
                        max(current[3], candidate[3]),
                    )
                    changed = True
                else:
                    rest.append(candidate)
            pending = rest
        merged.append(current)

    return merged


def scale_box(box: Box, scale: int, image_size: Tuple[int, int]) -> Box:
    width, height = image_size
    return (
        clamp(box[0] * scale, 0, width),
        clamp(box[1] * scale, 0, height),
        clamp(box[2] * scale, 0, width),
        clamp(box[3] * scale, 0, height),
    )


def detect_regions(image: Image.Image) -> Tuple[RGB, List[Box]]:
    background = guess_background(image)
    mask, scale = build_foreground_mask(image, background)
    components = find_components(mask)
    merged = merge_boxes(components)
    scaled = [scale_box(box, scale, image.size) for box in merged]
    filtered = [
        box for box in scaled
        if (box[2] - box[0]) * (box[3] - box[1]) >= max(4000, (image.size[0] * image.size[1]) // 80)
    ]
    filtered.sort(key=lambda box: (box[1], box[0]))
    if not filtered:
        width, height = image.size
        filtered = [(32, 80, max(160, width - 32), max(180, height - 80))]
    return background, filtered[:6]


def region_label(index: int, box: Box, image_size: Tuple[int, int]) -> str:
    width, height = image_size
    x1, y1, x2, y2 = box
    box_w = x2 - x1
    box_h = y2 - y1
    if y1 < height * 0.14 and box_h < height * 0.18:
        return "Top Bar"
    if y2 > height * 0.84 and box_h < height * 0.22:
        return "Bottom Navigation"
    if box_w > width * 0.74 and box_h > height * 0.22:
        return f"Main Panel {index}"
    if box_w > width * 0.54:
        return f"Section {index}"
    return f"Card {index}"


def render_card(label: str, box: Box, fill: RGB, viewport: dict) -> List[str]:
    viewport_width = int(viewport["width"])
    x1, y1, x2, y2 = box
    box_w = x2 - x1
    box_h = y2 - y1
    button_width = clamp(box_w, 180, max(220, viewport_width - 96))
    button_height = clamp(box_h, 52, 220)
    radius = clamp(min(button_height // 4, 28), 12, 28)
    fg = (255, 255, 255) if luminance(fill) < 145 else (17, 24, 39)
    detail = f"Detected region {box_w}x{box_h} at ({x1}, {y1})."
    return [
        f'    <h2 style="font-size: 24px; color: {rgb_to_hex(fg if luminance(fill) < 145 else (31, 41, 55))};">{html.escape(label)}</h2>',
        (
            f'    <button style="background-color: {rgb_to_hex(fill)}; color: {rgb_to_hex(fg)}; '
            f'width: {button_width}px; height: {button_height}px; border-radius: {radius}px; padding: 16px;">'
            f'{html.escape(label)}</button>'
        ),
        f'    <p style="font-size: 14px; color: #64748b;">{html.escape(detail)}</p>',
        "",
    ]


def render_bottom_nav(fill: RGB, viewport: dict) -> List[str]:
    viewport_width = int(viewport["width"])
    button_width = clamp((viewport_width - 132) // 4, 60, 96)
    fg = (255, 255, 255) if luminance(fill) < 145 else (17, 24, 39)
    lines = [
        '    <h2 style="font-size: 24px; color: #1f2937;">Bottom Navigation</h2>',
    ]
    for label in ("Home", "Climate", "Lights", "Security"):
        lines.append(
            f'    <button style="background-color: {rgb_to_hex(fill)}; color: {rgb_to_hex(fg)}; '
            f'width: {button_width}px; height: 52px; border-radius: 18px; padding: 12px;">{label}</button>'
        )
    lines.extend([
        '    <p style="font-size: 14px; color: #64748b;">Detected a wide bottom band and expanded it into draft navigation actions.</p>',
        "",
    ])
    return lines


def build_html(task: dict, image_path: Path, viewport: dict, background: RGB, regions: Sequence[Box]) -> str:
    image = Image.open(image_path).convert("RGB")
    lines = [
        "<!doctype html>",
        f'<html lang="{html.escape(task["target"]["language"])}">',
        "  <head>",
        '    <meta charset="utf-8">',
        '    <meta name="viewport" content="width=device-width, initial-scale=1">',
        f"    <title>{html.escape(task['page_name'])}</title>",
        "  </head>",
        f'  <body style="margin: 0; padding: 32px; background-color: {rgb_to_hex(background)}; font-family: sans-serif;">',
        "    <main>",
        f'    <h1 style="font-size: 32px; color: #0f172a;">{html.escape(task["page_name"])}</h1>',
        (
            '    <p style="font-size: 16px; color: #475569;">'
            f'Auto-drafted from `{html.escape(task["input"]["image_entry"])}`. '
            'Review this HTML and refine the semantics before final export.</p>'
        ),
        "",
    ]

    for index, box in enumerate(regions, start=1):
        crop = image.crop(box)
        fill = avg_rgb(crop)
        label = region_label(index, box, image.size)
        if label == "Bottom Navigation":
            lines.extend(render_bottom_nav(fill, viewport))
        else:
            lines.extend(render_card(label, box, fill, viewport))

    lines.extend([
        "    </main>",
        "  </body>",
        "</html>",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Draft task HTML from a source screenshot.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    source_type = task.get("input", {}).get("source_type", "html")
    if source_type != "image":
        print("Task does not use image input. Nothing to do.")
        return 0

    image_rel = task["input"].get("image_entry")
    if not image_rel:
        raise SystemExit("Image task is missing input.image_entry.")

    image_path = resolve(task_path, image_rel)
    html_path = resolve(task_path, task["input"]["html_entry"])
    if not image_path.is_file():
        raise SystemExit(f"Image input not found: {image_path}")

    image = Image.open(image_path).convert("RGB")
    background, regions = detect_regions(image)
    html_path.parent.mkdir(parents=True, exist_ok=True)
    html_path.write_text(
        build_html(task, image_path, task["target"]["viewport"], background, regions),
        encoding="utf-8",
    )

    print(f"Drafted HTML from image: {html_path}")
    print(f"Detected regions: {len(regions)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
