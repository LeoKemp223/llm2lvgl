#!/usr/bin/env python3

import argparse
import html
import json
import os
import re
from html.parser import HTMLParser
from pathlib import Path
from typing import Dict, List


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def c_escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")


def sanitize_identifier(value: str) -> str:
    value = value.strip().lower()
    value = re.sub(r"[^a-z0-9]+", "_", value)
    value = re.sub(r"_+", "_", value).strip("_")
    return value or "page"


class HTMLContentExtractor(HTMLParser):
    BLOCK_TAGS = {"h1", "h2", "h3", "p", "button", "a", "li"}
    CONTAINER_TAGS = {"div", "section", "article", "main", "header", "footer", "nav"}
    MAX_NESTING = 2

    def __init__(self) -> None:
        super().__init__()
        self.title = ""
        self.items: List[Dict[str, str]] = []
        self._tag_stack: List[str] = []
        self._buffer: List[str] = []
        self._image_count = 0
        self._div_depth = 0

    @staticmethod
    def _parse_inline_style(style_str: str) -> Dict[str, str]:
        props: Dict[str, str] = {}
        for part in style_str.split(";"):
            part = part.strip()
            if ":" in part:
                key, _, val = part.partition(":")
                props[key.strip().lower()] = val.strip()
        return props

    @staticmethod
    def _css_color_to_hex(color: str) -> str:
        color = color.strip().lower()
        if color.startswith("#") and len(color) in (4, 7):
            hex_str = color[1:]
            if len(hex_str) == 3:
                hex_str = "".join(c * 2 for c in hex_str)
            return f"0x{hex_str}"
        if color.startswith("rgb"):
            nums = re.findall(r"\d+", color)
            if len(nums) >= 3:
                return "0x{:02x}{:02x}{:02x}".format(int(nums[0]), int(nums[1]), int(nums[2]))
        return ""

    def handle_starttag(self, tag: str, attrs) -> None:
        attrs_dict = dict(attrs)
        self._tag_stack.append(tag)

        style_props: Dict[str, str] = {}
        if "style" in attrs_dict:
            style_props = self._parse_inline_style(attrs_dict["style"])

        if tag in self.CONTAINER_TAGS and self._div_depth < self.MAX_NESTING:
            self._div_depth += 1
            item: Dict[str, str] = {"type": "container", "text": ""}
            if "background-color" in style_props:
                hex_val = self._css_color_to_hex(style_props["background-color"])
                if hex_val:
                    item["bg_color"] = hex_val
            if "padding" in style_props:
                nums = re.findall(r"\d+", style_props["padding"])
                if nums:
                    item["padding"] = nums[0]
            self.items.append(item)

        if tag == "img":
            src = attrs_dict.get("src", "").strip()
            alt = attrs_dict.get("alt", "").strip() or "Image"
            if src:
                self._image_count += 1
                item_img: Dict[str, str] = {
                    "type": "image",
                    "text": alt,
                    "src": src,
                }
                self.items.append(item_img)

        if tag in self.BLOCK_TAGS and style_props:
            self._current_style_props = style_props
        else:
            self._current_style_props = {}

    def handle_endtag(self, tag: str) -> None:
        if self._tag_stack and self._tag_stack[-1] == tag:
            self._tag_stack.pop()

        if tag in self.CONTAINER_TAGS and self._div_depth > 0:
            self._div_depth -= 1
            self.items.append({"type": "container_end", "text": ""})

        text = html.unescape(" ".join(self._buffer)).strip()
        if text and tag in self.BLOCK_TAGS:
            item: Dict[str, str] = {
                "type": tag,
                "text": re.sub(r"\s+", " ", text),
            }
            style_props = getattr(self, "_current_style_props", {})
            if "color" in style_props:
                hex_val = self._css_color_to_hex(style_props["color"])
                if hex_val:
                    item["color"] = hex_val
            if "background-color" in style_props:
                hex_val = self._css_color_to_hex(style_props["background-color"])
                if hex_val:
                    item["bg_color"] = hex_val
            if "font-size" in style_props:
                nums = re.findall(r"\d+", style_props["font-size"])
                if nums:
                    item["font_size"] = nums[0]
            if "padding" in style_props:
                nums = re.findall(r"\d+", style_props["padding"])
                if nums:
                    item["padding"] = nums[0]
            self.items.append(item)
        if tag == "title" and text:
            self.title = re.sub(r"\s+", " ", text)
        self._buffer = []
        self._current_style_props = {}

    def handle_data(self, data: str) -> None:
        if not self._tag_stack:
            return
        current = self._tag_stack[-1]
        if current == "title" or current in self.BLOCK_TAGS:
            self._buffer.append(data)


def choose_font_size(size: int, profile: dict) -> int:
    """Return the font size to pass to ui_font_get()."""
    return size


def extract_html_content(html_path: Path) -> Dict[str, object]:
    parser = HTMLContentExtractor()
    parser.feed(html_path.read_text(encoding="utf-8"))

    items = parser.items[:24]
    if not items:
        items = [{"type": "p", "text": "No supported HTML content found. Replace with real content."}]

    return {
        "title": parser.title,
        "items": items,
    }


def generate_header(page_id: str) -> str:
    guard = f"{page_id.upper()}_PAGE_H"
    return (
        f"#ifndef {guard}\n"
        f"#define {guard}\n\n"
        '#include "lvgl.h"\n\n'
        f"lv_obj_t * {page_id}_page_create(void);\n"
        f"lv_obj_t * {page_id}_page_get_content_root(void);\n\n"
        "#endif\n"
    )


def render_item_code(index: int, item: Dict[str, str], profile: dict) -> List[str]:
    lines: List[str] = []
    item_type = item["type"]
    text = c_escape(item.get("text", ""))
    color = item.get("color", "")
    bg_color = item.get("bg_color", "")
    font_size_str = item.get("font_size", "")
    padding = item.get("padding", "")

    if item_type == "container":
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_obj_create(content);",
            f"    lv_obj_remove_flag(item_{index}, LV_OBJ_FLAG_SCROLLABLE);",
            f"    lv_obj_set_width(item_{index}, lv_pct(100));",
            f"    lv_obj_set_height(item_{index}, LV_SIZE_CONTENT);",
            f"    lv_obj_set_layout(item_{index}, LV_LAYOUT_FLEX);",
            f"    lv_obj_set_flex_flow(item_{index}, LV_FLEX_FLOW_COLUMN);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, 0);",
        ])
        if bg_color:
            lines.append(f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex({bg_color}), 0);")
            lines.append(f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_COVER, 0);")
        if padding:
            lines.append(f"    lv_obj_set_style_pad_all(item_{index}, {padding}, 0);")
        return lines

    if item_type == "container_end":
        return []

    if item_type == "h1":
        fs = int(font_size_str) if font_size_str else 32
        tc = color if color else "0x111827"
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_label_create(content);",
            f'    lv_label_set_text(item_{index}, "{text}");',
            f"    lv_obj_set_style_text_font(item_{index}, ui_font_get({choose_font_size(fs, profile)}), 0);",
            f"    lv_obj_set_style_text_color(item_{index}, lv_color_hex({tc}), 0);",
            f"    lv_label_set_long_mode(item_{index}, LV_LABEL_LONG_WRAP);",
            f"    lv_obj_set_width(item_{index}, content_width);",
        ])
    elif item_type in ("h2", "h3"):
        fs = int(font_size_str) if font_size_str else 24
        tc = color if color else "0x1f2937"
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_label_create(content);",
            f'    lv_label_set_text(item_{index}, "{text}");',
            f"    lv_obj_set_style_text_font(item_{index}, ui_font_get({choose_font_size(fs, profile)}), 0);",
            f"    lv_obj_set_style_text_color(item_{index}, lv_color_hex({tc}), 0);",
            f"    lv_label_set_long_mode(item_{index}, LV_LABEL_LONG_WRAP);",
            f"    lv_obj_set_width(item_{index}, content_width);",
        ])
    elif item_type == "button":
        fs = int(font_size_str) if font_size_str else 18
        btn_bg = bg_color if bg_color else "0x2563eb"
        btn_tc = color if color else "0xffffff"
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_button_create(content);",
            f"    lv_obj_set_width(item_{index}, LV_SIZE_CONTENT);",
            f"    lv_obj_set_style_pad_hor(item_{index}, 20, 0);",
            f"    lv_obj_set_style_pad_ver(item_{index}, 12, 0);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex({btn_bg}), 0);",
            f"    lv_obj_set_style_radius(item_{index}, 18, 0);",
            f"    lv_obj_t * item_{index}_label = lv_label_create(item_{index});",
            f'    lv_label_set_text(item_{index}_label, "{text}");',
            f"    lv_obj_set_style_text_color(item_{index}_label, lv_color_hex({btn_tc}), 0);",
            f"    lv_obj_set_style_text_font(item_{index}_label, ui_font_get({choose_font_size(fs, profile)}), 0);",
            f"    lv_obj_center(item_{index}_label);",
        ])
    elif item_type == "a":
        fs = int(font_size_str) if font_size_str else 18
        tc = color if color else "0x2563eb"
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_label_create(content);",
            f'    lv_label_set_text(item_{index}, "{text}");',
            f"    lv_obj_set_style_text_font(item_{index}, ui_font_get({choose_font_size(fs, profile)}), 0);",
            f"    lv_obj_set_style_text_color(item_{index}, lv_color_hex({tc}), 0);",
            f"    lv_label_set_long_mode(item_{index}, LV_LABEL_LONG_WRAP);",
            f"    lv_obj_set_width(item_{index}, content_width);",
        ])
    elif item_type == "image":
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_obj_create(content);",
            f"    lv_obj_remove_flag(item_{index}, LV_OBJ_FLAG_SCROLLABLE);",
            f"    lv_obj_set_size(item_{index}, content_width, 120);",
            f"    lv_obj_set_style_radius(item_{index}, 20, 0);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex(0xe5e7eb), 0);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, 0);",
            f"    lv_obj_t * item_{index}_label = lv_label_create(item_{index});",
            f'    lv_label_set_text(item_{index}_label, "{text}");',
            f"    lv_obj_set_style_text_font(item_{index}_label, ui_font_get({choose_font_size(18, profile)}), 0);",
            f"    lv_obj_set_style_text_color(item_{index}_label, lv_color_hex(0x6b7280), 0);",
            f"    lv_obj_center(item_{index}_label);",
        ])
    else:
        fs = int(font_size_str) if font_size_str else 18
        tc = color if color else "0x4b5563"
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_label_create(content);",
            f'    lv_label_set_text(item_{index}, "{text}");',
            f"    lv_obj_set_style_text_font(item_{index}, ui_font_get({choose_font_size(fs, profile)}), 0);",
            f"    lv_obj_set_style_text_color(item_{index}, lv_color_hex({tc}), 0);",
            f"    lv_label_set_long_mode(item_{index}, LV_LABEL_LONG_WRAP);",
            f"    lv_obj_set_width(item_{index}, content_width);",
        ])

    return lines


def generate_source(page_id: str, page_name: str, html_info: Dict[str, object], profile: dict, viewport: dict) -> str:
    title_text = html_info["title"] or page_name
    items: List[Dict[str, str]] = html_info["items"]  # type: ignore[assignment]
    width = int(viewport["width"])
    height = int(viewport["height"])
    content_width = max(240, width - 96)

    lines = [
        f'#include "{page_id}_page.h"',
        "",
        '#include "ui_font.h"',
        "",
        "static lv_obj_t * g_content_root = NULL;",
        "",
        f"lv_obj_t * {page_id}_page_create(void)",
        "{",
        "    lv_obj_t * screen = lv_obj_create(NULL);",
        f"    lv_coord_t content_width = {content_width};",
        "    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);",
        "    lv_obj_set_style_bg_color(screen, lv_color_hex(0xf8fafc), 0);",
        "    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);",
        f"    lv_obj_set_size(screen, {width}, {height});",
        "",
        "    lv_obj_t * content = lv_obj_create(screen);",
        "    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);",
        "    lv_obj_set_layout(content, LV_LAYOUT_FLEX);",
        "    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);",
        "    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);",
        "    lv_obj_set_style_pad_all(content, 32, 0);",
        "    lv_obj_set_style_pad_row(content, 14, 0);",
        "    lv_obj_set_style_radius(content, 24, 0);",
        "    lv_obj_set_style_border_width(content, 0, 0);",
        "    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);",
        "    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);",
        "    lv_obj_set_style_shadow_width(content, 24, 0);",
        "    lv_obj_set_style_shadow_opa(content, 12, 0);",
        "    lv_obj_set_style_shadow_color(content, lv_color_hex(0xcbd5e1), 0);",
        f"    lv_obj_set_size(content, {content_width}, LV_SIZE_CONTENT);",
        "    lv_obj_center(content);",
        "",
        "    lv_obj_t * title = lv_label_create(content);",
        f'    lv_label_set_text(title, "{c_escape(title_text)}");',
        f"    lv_obj_set_style_text_font(title, ui_font_get({choose_font_size(32, profile)}), 0);",
        "    lv_obj_set_style_text_color(title, lv_color_hex(0x0f172a), 0);",
        "    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);",
        f"    lv_obj_set_width(title, {content_width});",
        "",
    ]

    for index, item in enumerate(items):
        lines.extend(render_item_code(index, item, profile))
        lines.append("")

    lines.extend([
        "    g_content_root = content;",
        "    return screen;",
        "}",
        "",
        f"lv_obj_t * {page_id}_page_get_content_root(void)",
        "{",
        "    return g_content_root;",
        "}",
        "",
    ])
    return "\n".join(lines)


def write_codegen_prompt(task_path: Path, task: dict, profile: dict, html_info: Dict[str, object]) -> None:
    prompt_path = task_path.parent / "generated" / "codegen_prompt.md"
    lines = [
        f"# Codegen Prompt For {task['page_name']}",
        "",
        "## Task",
        f"- task_id: `{task['task_id']}`",
        f"- page_id: `{task['page_id']}`",
        f"- page_name: `{task['page_name']}`",
        "",
        "## Target",
        f"- viewport: `{task['target']['viewport']['width']}x{task['target']['viewport']['height']}`",
        f"- profile: `{profile['id']}`",
        f"- color_depth: `{task['target']['color_depth']}`",
        f"- language: `{task['target']['language']}`",
        "",
        "## Generation Constraints",
        f"- allow_freetype: `{task['generation']['allow_freetype']}`",
        f"- allow_filesystem_assets: `{task['generation']['allow_filesystem_assets']}`",
        f"- component_mode: `{task['generation']['component_mode']}`",
        "",
        "## Extracted HTML Content",
        f"- html_title: `{html_info['title']}`",
    ]
    for item in html_info["items"]:  # type: ignore[index]
        lines.append(f"- {item['type']}: `{item['text']}`")
    prompt_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_manifest(task_path: Path, task: dict, profile: dict, html_info: Dict[str, object]) -> None:
    manifest = {
        "task_id": task["task_id"],
        "page_id": task["page_id"],
        "page_name": task["page_name"],
        "profile": profile["id"],
        "viewport": task["target"]["viewport"],
        "driver": "rule_based_html_v1",
        "html_title": html_info["title"],
        "item_count": len(html_info["items"]),
        "output_c": task["generation"]["output_c"],
        "output_h": task["generation"]["output_h"],
    }
    (task_path.parent / "generated" / "manifest.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate an LVGL page from task/profile/HTML inputs.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    profile = load_json(resolve(task_path, task["target"]["profile"]))
    html_path = resolve(task_path, task["input"]["html_entry"])
    output_h = resolve(task_path, task["generation"]["output_h"])
    output_c = resolve(task_path, task["generation"]["output_c"])

    if not html_path.is_file():
        raise SystemExit(f"HTML input not found: {html_path}")

    page_id = sanitize_identifier(task["page_id"])
    page_name = task["page_name"]
    html_info = extract_html_content(html_path)

    output_h.parent.mkdir(parents=True, exist_ok=True)
    output_c.parent.mkdir(parents=True, exist_ok=True)
    output_h.write_text(generate_header(page_id), encoding="utf-8")
    output_c.write_text(
        generate_source(page_id, page_name, html_info, profile, task["target"]["viewport"]),
        encoding="utf-8",
    )

    write_codegen_prompt(task_path, task, profile, html_info)
    write_manifest(task_path, task, profile, html_info)

    print(f"Generated page source: {output_c}")
    print(f"Generated page header: {output_h}")
    print(f"Driver: rule_based_html_v1")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
