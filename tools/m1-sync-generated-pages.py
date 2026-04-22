#!/usr/bin/env python3

import argparse
import fcntl
import json
import os
import sys
from pathlib import Path
from typing import Optional


REPO_ROOT = Path(__file__).resolve().parent.parent


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def c_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def write_file(path: Path, content: str) -> bool:
    """Write file with locking. Returns True if content changed."""
    path.parent.mkdir(parents=True, exist_ok=True)
    encoded = content.encode("utf-8")
    if path.exists():
        try:
            if path.read_bytes() == encoded:
                return False
        except OSError:
            pass
    fd = os.open(str(path), os.O_WRONLY | os.O_CREAT | os.O_TRUNC)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX)
        os.write(fd, encoded)
    finally:
        fcntl.flock(fd, fcntl.LOCK_UN)
        os.close(fd)
    return True


def discover_task(task_path: Path) -> Optional[dict]:
    task = load_json(task_path)
    if task.get("compat", {}).get("legacy_page_flow_task") is not None:
        return None

    output_c = (task_path.parent / task["generation"]["output_c"]).resolve()
    output_h = (task_path.parent / task["generation"]["output_h"]).resolve()
    if not output_c.exists() or not output_h.exists():
        return None

    return {
        "task_path": task_path.resolve(),
        "task": task,
        "output_c": output_c,
        "output_h": output_h,
    }


def discover_tasks(tasks_root: Path) -> list:
    tasks = []
    if not tasks_root.exists():
        return tasks

    for task_path in sorted(tasks_root.glob("*/task.json")):
        item = discover_task(task_path.resolve())
        if item is not None:
            tasks.append(item)

    return tasks


def unique_tasks(tasks: list) -> list:
    unique = []
    seen_page_ids = set()
    for item in tasks:
        page_id = item["task"]["page_id"]
        if page_id in seen_page_ids:
            print(f"ERROR: Duplicate generated page id: {page_id}", file=sys.stderr)
            sys.exit(1)
        seen_page_ids.add(page_id)
        unique.append(item)
    return unique


def render_header() -> str:
    return (
        "#ifndef GENERATED_PAGE_REGISTRY_H\n"
        "#define GENERATED_PAGE_REGISTRY_H\n\n"
        "#include <stddef.h>\n\n"
        "#include \"page_registry.h\"\n\n"
        "size_t lvgl_generated_page_count(void);\n"
        "const lvgl_page_descriptor_t * lvgl_generated_page_list(void);\n\n"
        "#endif\n"
    )


def render_registry_source(tasks: list) -> str:
    lines = [
        '#include "generated_page_registry.h"',
        "",
    ]

    for item in tasks:
        lines.append(f'#include "{c_string(str(item["output_h"]))}"')

    lines.extend([
        "",
        "static const lvgl_page_descriptor_t g_generated_pages[] = {",
    ])

    for item in tasks:
        task = item["task"]
        page_id = task["page_id"]
        page_name = task["page_name"]
        symbol_base = f"{page_id}_page"
        lines.extend([
            "    {",
            f'        .id = "{c_string(page_id)}",',
            f'        .name = "{c_string(page_name)}",',
            f"        .create = {symbol_base}_create,",
            f"        .get_content_root = {symbol_base}_get_content_root,",
            "    },",
        ])

    lines.extend([
        "};",
        "",
        "size_t lvgl_generated_page_count(void)",
        "{",
        "    return sizeof(g_generated_pages) / sizeof(g_generated_pages[0]);",
        "}",
        "",
        "const lvgl_page_descriptor_t * lvgl_generated_page_list(void)",
        "{",
        "    return g_generated_pages;",
        "}",
        "",
    ])
    return "\n".join(lines)


def render_cmake(tasks: list) -> str:
    lines = ["set(LVGL_GENERATED_PAGE_SOURCES"]
    for item in tasks:
        lines.append(f'  "{str(item["output_c"])}"')
    lines.append(")")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Discover generated task pages and emit LVGL registry/CMake bridge files.")
    parser.add_argument("--tasks-root", default=str(REPO_ROOT / "workspace" / "tasks"), help="Directory containing task subdirectories")
    parser.add_argument("--task-json", help="Optional single task.json to isolate registry/source generation to one page")
    parser.add_argument("--registry-c", required=True, help="Path to write generated_page_registry.c")
    parser.add_argument("--registry-h", required=True, help="Path to write generated_page_registry.h")
    parser.add_argument("--cmake-out", required=True, help="Path to write generated_page_sources.cmake")
    args = parser.parse_args()

    if args.task_json:
        task_item = discover_task(Path(args.task_json).resolve())
        tasks = unique_tasks([] if task_item is None else [task_item])
    else:
        tasks_root = Path(args.tasks_root).resolve()
        tasks = unique_tasks(discover_tasks(tasks_root))

    write_file(Path(args.registry_h).resolve(), render_header())
    write_file(Path(args.registry_c).resolve(), render_registry_source(tasks))
    write_file(Path(args.cmake_out).resolve(), render_cmake(tasks))

    print(f"Discovered generated pages: {len(tasks)}")
    for item in tasks:
        print(f"- {item['task']['page_id']}: {item['output_c']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
