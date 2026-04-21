#!/usr/bin/env python3

import argparse
import json
import os
import re
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_PROFILE = REPO_ROOT / "profiles" / "sim_1280x800.json"


def slugify(value: str) -> str:
    value = value.strip().lower()
    value = re.sub(r"[^a-z0-9]+", "_", value)
    value = re.sub(r"_+", "_", value).strip("_")
    return value or "page"


def titleize(value: str) -> str:
    return value.replace("_", " ").strip().title()


def relative_posix(from_dir: Path, to_path: Path) -> str:
    return Path(os.path.relpath(to_path.resolve(), from_dir.resolve())).as_posix()


def build_task_payload(task_dir: Path, page_id: str, page_name: str, profile_path: Path) -> dict:
    return {
        "task_id": task_dir.name,
        "page_id": page_id,
        "page_name": page_name,
        "input": {
            "source_type": "html",
            "html_entry": "input/index.html",
            "assets_dir": "input/assets",
            "notes_file": "input/notes.md",
        },
        "target": {
            "profile": relative_posix(task_dir, profile_path),
            "viewport": {
                "width": 1280,
                "height": 800,
            },
            "color_depth": 32,
            "dpi": 160,
            "language": "zh-CN",
        },
        "generation": {
            "output_c": f"generated/{page_id}_page.c",
            "output_h": f"generated/{page_id}_page.h",
            "component_mode": "portable",
            "allow_freetype": False,
            "allow_filesystem_assets": False,
        },
        "reference": {
            "image": "reference/reference.png",
            "render_from_html": True,
        },
        "validation": {
            "pixel_diff_threshold": 16,
            "max_diff_ratio": 0.18,
            "max_mean_abs_diff": 22.0,
            "require_size_match": True,
        },
        "export": {
            "bundle_dir": "export/portable_bundle",
            "include_assets": True,
            "include_porting_md": True,
        },
        "failure_policy": {
            "max_iterations": 8,
            "stop_on_build_error": True,
            "stop_on_missing_reference": True,
            "stop_on_validation_script_error": True,
            "stop_on_portability_error": True,
        },
        "compat": {
            "legacy_page_flow_task": None,
        },
        "notes": [
            "Fill input/index.html with the target page HTML.",
            "Replace the simulator profile with the real board profile before export.",
        ],
    }


def ensure_seed_files(task_dir: Path, page_id: str, page_name: str) -> None:
    html_path = task_dir / "input" / "index.html"
    if not html_path.exists():
        html_path.write_text(
            "<!doctype html>\n"
            "<html lang=\"zh-CN\">\n"
            "  <head>\n"
            "    <meta charset=\"utf-8\">\n"
            "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
            f"    <title>{page_name}</title>\n"
            "  </head>\n"
            "  <body>\n"
            f"    <h1>{page_name}</h1>\n"
            "    <p>Replace this placeholder with the real HTML input.</p>\n"
            "  </body>\n"
            "</html>\n",
            encoding="utf-8",
        )

    notes_path = task_dir / "input" / "notes.md"
    if not notes_path.exists():
        notes_path.write_text(
            f"# {page_name}\n\n"
            "- Record layout notes here.\n"
            "- Record board constraints here.\n",
            encoding="utf-8",
        )

    header_path = task_dir / "generated" / f"{page_id}_page.h"
    if not header_path.exists():
        guard = f"{page_id.upper()}_PAGE_H"
        header_path.write_text(
            f"#ifndef {guard}\n"
            f"#define {guard}\n\n"
            '#include "lvgl.h"\n\n'
            f"lv_obj_t * {page_id}_page_create(void);\n"
            f"lv_obj_t * {page_id}_page_get_content_root(void);\n\n"
            "#endif\n",
            encoding="utf-8",
        )

    source_path = task_dir / "generated" / f"{page_id}_page.c"
    if not source_path.exists():
        source_path.write_text(
            f'#include "{page_id}_page.h"\n\n'
            "static lv_obj_t * g_content_root = NULL;\n\n"
            f"lv_obj_t * {page_id}_page_create(void)\n"
            "{\n"
            "    lv_obj_t * screen = lv_obj_create(NULL);\n"
            "    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);\n"
            "    lv_obj_set_style_bg_color(screen, lv_color_hex(0xffffff), 0);\n"
            "    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);\n"
            "\n"
            "    lv_obj_t * title = lv_label_create(screen);\n"
            f'    lv_label_set_text(title, "{page_name}");\n'
            "    lv_obj_set_style_text_color(title, lv_color_hex(0x111827), 0);\n"
            "    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);\n"
            "    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);\n"
            "\n"
            "    lv_obj_t * subtitle = lv_label_create(screen);\n"
            '    lv_label_set_text(subtitle, "Replace this scaffold with generated LVGL code.");\n'
            "    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x6b7280), 0);\n"
            "    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);\n"
            "    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 64);\n"
            "\n"
            "    g_content_root = screen;\n"
            "    return screen;\n"
            "}\n\n"
            f"lv_obj_t * {page_id}_page_get_content_root(void)\n"
            "{\n"
            "    return g_content_root;\n"
            "}\n",
            encoding="utf-8",
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="Initialize a task workspace for the LVGL HTML-to-page pipeline.")
    parser.add_argument("task_dir", help="Task directory to create, for example workspace/tasks/my_page_v1")
    parser.add_argument("--page-id", help="Page id used for generated symbols and filenames")
    parser.add_argument("--page-name", help="Human-readable page name")
    parser.add_argument("--profile", default=str(DEFAULT_PROFILE), help="Board profile JSON path")
    parser.add_argument("--force", action="store_true", help="Overwrite task.json if it already exists")
    args = parser.parse_args()

    task_dir = Path(args.task_dir).resolve()
    page_id = slugify(args.page_id or task_dir.name)
    page_name = args.page_name or titleize(page_id)
    profile_path = Path(args.profile).resolve()

    if not profile_path.is_file():
        raise SystemExit(f"Profile not found: {profile_path}")

    profile_data = json.loads(profile_path.read_text(encoding="utf-8"))
    if "version" not in profile_data:
        raise SystemExit(f"Profile missing 'version' field: {profile_path}")

    task_dir.mkdir(parents=True, exist_ok=True)
    for rel_dir in ("input/assets", "reference", "generated", "artifacts", "export"):
        (task_dir / rel_dir).mkdir(parents=True, exist_ok=True)

    task_path = task_dir / "task.json"
    if task_path.exists() and not args.force:
        raise SystemExit(f"Task already exists: {task_path} (use --force to overwrite)")

    payload = build_task_payload(task_dir, page_id, page_name, profile_path)
    task_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    ensure_seed_files(task_dir, page_id, page_name)

    print(f"Created task: {task_path}")
    print(f"Page id: {page_id}")
    print(f"Profile: {profile_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
