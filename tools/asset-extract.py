#!/usr/bin/env python3

import argparse
import json
from pathlib import Path

from PIL import Image


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def extract_assets(task_path: Path, plan_path: Path) -> dict:
    plan = load_json(plan_path)
    manifest_assets = []
    opened: dict[str, Image.Image] = {}

    for item in plan.get("assets", []):
        source_rel = item["source"]
        if source_rel not in opened:
            opened[source_rel] = Image.open(resolve(task_path, source_rel)).convert("RGBA")
        image = opened[source_rel]
        x1, y1, x2, y2 = [int(v) for v in item["source_box"]]
        crop = image.crop((x1, y1, x2, y2))
        output_rel = item["output"]
        output_path = resolve(task_path, output_rel)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        crop.save(output_path)

        manifest_assets.append({
            "id": item["id"],
            "role": item["role"],
            "mode": item["mode"],
            "path": output_rel,
            "source_box": item["source_box"],
            "viewport_box": item["viewport_box"],
            "size": [crop.width, crop.height],
            "avg_color": item.get("avg_color"),
        })

    return {
        "version": 1,
        "task_id": plan["task_id"],
        "source_image": plan["source_image"],
        "source_size": plan["source_size"],
        "viewport": plan["viewport"],
        "assets": manifest_assets,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract crop assets from an asset plan.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    parser.add_argument("--plan", help="Asset plan JSON path")
    parser.add_argument("--manifest", help="Output asset manifest JSON path")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    plan_path = Path(args.plan).resolve() if args.plan else task_path.parent / "artifacts" / "asset_plan.json"
    if not plan_path.is_file():
        raise SystemExit(f"Asset plan not found: {plan_path}")

    manifest = extract_assets(task_path, plan_path)
    manifest_path = Path(args.manifest).resolve() if args.manifest else task_path.parent / "generated" / "asset_manifest.json"
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Asset manifest written: {manifest_path}")
    print(f"Extracted assets: {len(manifest['assets'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
