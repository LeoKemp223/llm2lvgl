#!/usr/bin/env python3

import argparse
import json
import shutil
from pathlib import Path


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def main() -> int:
    parser = argparse.ArgumentParser(description="Export generated LVGL page code into a portable bundle.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    profile_path = resolve(task_path, task["target"]["profile"])
    profile = load_json(profile_path)
    bundle_dir = resolve(task_path, task["export"]["bundle_dir"])
    bundle_dir.mkdir(parents=True, exist_ok=True)

    output_c = resolve(task_path, task["generation"]["output_c"])
    output_h = resolve(task_path, task["generation"]["output_h"])
    if not output_c.exists() or not output_h.exists():
        raise SystemExit("Generated .c/.h files do not exist. Export requires generated source files.")

    copy_file(output_c, bundle_dir / output_c.name)
    copy_file(output_h, bundle_dir / output_h.name)

    if task["export"].get("include_assets", False):
        assets_dir = resolve(task_path, task["input"]["assets_dir"])
        if assets_dir.exists():
            target_assets_dir = bundle_dir / "assets"
            for asset in assets_dir.rglob("*"):
                if asset.is_file():
                    copy_file(asset, target_assets_dir / asset.relative_to(assets_dir))

    manifest = {
        "task_id": task["task_id"],
        "page_id": task["page_id"],
        "page_name": task["page_name"],
        "profile": profile["id"],
        "screen": profile["screen"],
        "fonts": profile["fonts"],
        "constraints": profile["constraints"],
        "lv_conf_dependencies": [
            "LV_USE_FLEX",
            "LV_USE_LABEL",
            "LV_USE_OBJ",
            "LV_FONT_MONTSERRAT_14",
            "LV_FONT_MONTSERRAT_24",
        ],
        "lvgl_version": "9.x",
        "exported_files": [
            output_c.name,
            output_h.name,
        ],
    }
    (bundle_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    if task["export"].get("include_porting_md", False):
        page_id = task["page_id"]
        page_name = task["page_name"]
        scr_w = profile["screen"]["width"]
        scr_h = profile["screen"]["height"]
        color_depth = profile["screen"]["color_depth"]
        allow_ft = profile["fonts"]["allow_freetype"]
        allow_fs = profile["assets"]["allow_filesystem"]
        prof_id = profile["id"]

        (bundle_dir / "PORTING.md").write_text(
            f"# {page_name} Porting Notes / 移植说明\n\n"
            "## Overview / 概览\n\n"
            f"| Field / 字段 | Value / 值 |\n"
            f"|---|---|\n"
            f"| Page id / 页面标识 | `{page_id}` |\n"
            f"| Target profile / 目标配置 | `{prof_id}` |\n"
            f"| Screen / 屏幕分辨率 | `{scr_w}x{scr_h}` |\n"
            f"| Color depth / 色深 | `{color_depth}` |\n"
            f"| FreeType | `{allow_ft}` |\n"
            f"| Filesystem assets / 文件系统资源 | `{allow_fs}` |\n\n"
            "## Font Sizes / 字体\n\n"
            "The page uses `ui_font_get(size)` for dynamic font loading. On targets\n"
            "without FreeType, it falls back to `lv_font_montserrat_14` (size <= 18)\n"
            "or `lv_font_montserrat_24` (size > 18).\n\n"
            "页面通过 `ui_font_get(size)` 动态加载字体。在没有 FreeType 的目标板上，\n"
            "自动回退到 `lv_font_montserrat_14`（size <= 18）或 `lv_font_montserrat_24`（size > 18）。\n\n"
            "## API Dependencies / 依赖接口\n\n"
            "- `lv_obj_create`, `lv_label_create`, `lv_button_create`\n"
            "- `lv_dropdown_create`, `lv_checkbox_create`\n"
            "- `lv_obj_set_layout` (LV_LAYOUT_FLEX)\n"
            "- `lv_obj_set_style_*` family\n"
            "- `ui_font_get()` / `ui_font_cleanup()` from `ui_font.h`\n\n"
            "## CMake Integration / CMake 集成\n\n"
            "```cmake\n"
            f"add_library(page_{page_id} STATIC\n"
            f"  {output_c.name}\n"
            "  ui_font.c\n"
            ")\n"
            f"target_include_directories(page_{page_id} PRIVATE ${{LVGL_INCLUDE_DIRS}})\n"
            f"target_link_libraries(page_{page_id} PRIVATE lvgl)\n"
            "```\n\n"
            "## Quick Start / 快速开始\n\n"
            "```c\n"
            f'#include "{page_id}_page.h"\n'
            "\n"
            f"lv_obj_t * scr = {page_id}_page_create();\n"
            "lv_screen_load(scr);\n"
            "```\n\n"
            "## Checklist / 移植检查清单\n\n"
            "Before importing into firmware / 导入固件前请确认：\n\n"
            "- [ ] Copy `ui_font.h` and `ui_font.c` alongside the page source\n"
            "      复制 `ui_font.h` 和 `ui_font.c` 到页面源码同级目录\n"
            "- [ ] Wire the page create function into the target project registry\n"
            "      将页面 create 函数注册到目标工程的页面注册表\n"
            "- [ ] Ensure required fonts exist on the target (or rely on built-in fallback)\n"
            "      确认目标板上有所需字体（或依赖内置 fallback）\n"
            "- [ ] Copy any required assets from the exported `assets/` directory\n"
            "      复制 `assets/` 目录中的资源文件到目标工程\n"
            f"- [ ] Enable `LV_USE_FLEX` in `lv_conf.h`\n"
            "      在 `lv_conf.h` 中启用 `LV_USE_FLEX`\n",
            encoding="utf-8",
        )

    print(f"Exported bundle: {bundle_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
