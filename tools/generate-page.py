#!/usr/bin/env python3

import argparse
import html
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from html.parser import HTMLParser
from pathlib import Path
from typing import Dict, List, Optional


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

    def __init__(self) -> None:
        super().__init__()
        self.title = ""
        self.items: List[Dict[str, object]] = []
        self._node_stack: List[DOMNode] = []
        self._roots: List[DOMNode] = []

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

    @staticmethod
    def _normalize_text(text: str) -> str:
        return re.sub(r"\s+", " ", html.unescape(text)).strip()

    @staticmethod
    def _tailwind_spacing_to_px(raw: str) -> Optional[int]:
        if raw == "px":
            return 1
        if re.fullmatch(r"\d+(\.\d+)?", raw):
            return int(round(float(raw) * 4))
        return None

    @staticmethod
    def _extract_bracket_value(token: str, prefix: str) -> Optional[str]:
        marker = f"{prefix}-["
        if token.startswith(marker) and token.endswith("]"):
            return token[len(marker):-1].strip()
        return None

    @staticmethod
    def _extract_px_value(raw: str) -> Optional[int]:
        match = re.fullmatch(r"(-?\d+(?:\.\d+)?)px", raw)
        if match:
            return int(round(float(match.group(1))))

        match = re.fullmatch(r"(-?\d+(?:\.\d+)?)rem", raw)
        if match:
            return int(round(float(match.group(1)) * 16))
        return None

    @staticmethod
    def _extract_percent_value(raw: str) -> Optional[int]:
        match = re.fullmatch(r"(\d+(?:\.\d+)?)%", raw)
        if match:
            return max(0, min(100, int(round(float(match.group(1))))))
        return None

    @staticmethod
    def _rounded_token_to_px(token: str) -> Optional[int]:
        mapping = {
            "rounded": 4,
            "rounded-sm": 2,
            "rounded-md": 6,
            "rounded-lg": 8,
            "rounded-xl": 12,
            "rounded-2xl": 16,
            "rounded-3xl": 24,
            "rounded-full": 9999,
        }
        return mapping.get(token)

    @staticmethod
    def _named_color_to_hex(raw: str) -> str:
        mapping = {
            "white": "0xffffff",
            "black": "0x000000",
            "transparent": "",
        }
        return mapping.get(raw, "")

    def _class_tokens(self, node: "DOMNode") -> List[str]:
        class_attr = node.attrs.get("class", "")
        return [part.strip() for part in class_attr.split() if part.strip()]

    def _style_props(self, node: "DOMNode") -> Dict[str, str]:
        style_attr = node.attrs.get("style", "")
        return self._parse_inline_style(style_attr) if style_attr else {}

    def _node_text(self, node: "DOMNode", include_icons: bool = False) -> str:
        parts: List[str] = []
        for chunk in node.text_chunks:
            normalized = self._normalize_text(chunk)
            if normalized:
                parts.append(normalized)

        for child in node.children:
            if not include_icons and self._is_material_icon(child):
                continue
            child_text = self._node_text(child, include_icons=include_icons)
            if child_text:
                parts.append(child_text)

        return self._normalize_text(" ".join(parts))

    def _find_first_icon_name(self, node: "DOMNode") -> str:
        if self._is_material_icon(node):
            return (
                node.attrs.get("data-icon", "").strip()
                or self._node_text(node, include_icons=True)
            )

        for child in node.children:
            icon_name = self._find_first_icon_name(child)
            if icon_name:
                return icon_name
        return ""

    def _is_material_icon(self, node: "DOMNode") -> bool:
        if node.attrs.get("data-icon"):
            return True
        return "material-symbols-outlined" in self._class_tokens(node)

    def _node_dimension_px(self, node: "DOMNode", axis: str) -> Optional[int]:
        style_key = "width" if axis == "w" else "height"
        style_props = self._style_props(node)
        if style_key in style_props:
            px_val = self._extract_px_value(style_props[style_key])
            if px_val is not None:
                return px_val

        for token in self._class_tokens(node):
            bracket = self._extract_bracket_value(token, axis)
            if bracket is not None:
                px_val = self._extract_px_value(bracket)
                if px_val is not None:
                    return px_val
                continue

            if token.startswith(f"{axis}-"):
                raw = token[len(axis) + 1:]
                px_val = self._tailwind_spacing_to_px(raw)
                if px_val is not None:
                    return px_val
        return None

    def _node_width_percent(self, node: "DOMNode") -> Optional[int]:
        style_props = self._style_props(node)
        if "width" in style_props:
            percent = self._extract_percent_value(style_props["width"])
            if percent is not None:
                return percent

        for token in self._class_tokens(node):
            bracket = self._extract_bracket_value(token, "w")
            if bracket is not None:
                percent = self._extract_percent_value(bracket)
                if percent is not None:
                    return percent
        return None

    def _is_full_width(self, node: "DOMNode") -> bool:
        return "w-full" in self._class_tokens(node)

    def _node_color(self, node: "DOMNode", kind: str) -> str:
        style_key = {
            "bg": "background-color",
            "text": "color",
            "border": "border-color",
        }[kind]
        style_props = self._style_props(node)
        if style_key in style_props:
            hex_val = self._css_color_to_hex(style_props[style_key])
            if hex_val:
                return hex_val

        prefix = {
            "bg": "bg",
            "text": "text",
            "border": "border",
        }[kind]
        for token in self._class_tokens(node):
            bracket = self._extract_bracket_value(token, prefix)
            if bracket is not None:
                hex_val = self._css_color_to_hex(bracket)
                if hex_val:
                    return hex_val

            if token.startswith(f"{prefix}-"):
                raw = token[len(prefix) + 1:]
                named = self._named_color_to_hex(raw)
                if named:
                    return named
        return ""

    def _node_radius_px(self, node: "DOMNode") -> Optional[int]:
        style_props = self._style_props(node)
        if "border-radius" in style_props:
            px_val = self._extract_px_value(style_props["border-radius"])
            if px_val is not None:
                return px_val

        for token in self._class_tokens(node):
            radius = self._rounded_token_to_px(token)
            if radius is not None:
                return radius
        return None

    def _node_padding_px(self, node: "DOMNode") -> Optional[int]:
        style_props = self._style_props(node)
        if "padding" in style_props:
            px_val = self._extract_px_value(style_props["padding"])
            if px_val is not None:
                return px_val

        for token in self._class_tokens(node):
            for prefix in ("p", "px", "py"):
                if token.startswith(f"{prefix}-"):
                    raw = token[len(prefix) + 1:]
                    px_val = self._tailwind_spacing_to_px(raw)
                    if px_val is not None:
                        return px_val
        return None

    def _node_font_size_px(self, node: "DOMNode") -> Optional[int]:
        style_props = self._style_props(node)
        if "font-size" in style_props:
            px_val = self._extract_px_value(style_props["font-size"])
            if px_val is not None:
                return px_val

        for token in self._class_tokens(node):
            bracket = self._extract_bracket_value(token, "text")
            if bracket is not None:
                px_val = self._extract_px_value(bracket)
                if px_val is not None:
                    return px_val
        return None

    def _node_checked(self, node: "DOMNode") -> bool:
        value = node.attrs.get("checked") or node.attrs.get("aria-checked", "")
        if str(value).lower() in {"true", "checked", "1"}:
            return True

        for token in self._class_tokens(node):
            if token.startswith("bg-") and any(key in token for key in ("primary", "#", "blue", "green")):
                return True

        for child in node.children:
            if any(
                token.startswith(("right-", "translate-x-", "justify-end"))
                for token in self._class_tokens(child)
            ):
                return True
        return False

    def _looks_like_switch_knob(self, node: "DOMNode") -> bool:
        if self._node_text(node):
            return False
        width = self._node_dimension_px(node, "w")
        height = self._node_dimension_px(node, "h")
        radius = self._node_radius_px(node)
        if width is None or height is None:
            return False
        if abs(width - height) > 4:
            return False
        if radius is None or radius < 9999:
            return False
        return True

    def _switch_item(self, node: "DOMNode") -> Optional[Dict[str, object]]:
        if node.tag == "input":
            input_type = node.attrs.get("type", "").strip().lower()
            if input_type not in {"checkbox", "radio"}:
                return None
            if node.attrs.get("role", "").strip().lower() != "switch":
                return None

        role = node.attrs.get("role", "").strip().lower()
        width = self._node_dimension_px(node, "w")
        height = self._node_dimension_px(node, "h")
        radius = self._node_radius_px(node)
        has_knob_child = any(self._looks_like_switch_knob(child) for child in node.children)
        has_no_text = not self._node_text(node)
        looks_like_track = (
            width is not None
            and height is not None
            and width >= int(height * 1.5)
            and height <= 32
            and radius is not None
            and radius >= 9999
        )

        if role != "switch" and not (looks_like_track and has_knob_child and has_no_text):
            return None

        item: Dict[str, object] = {
            "type": "switch",
            "text": "",
            "checked": self._node_checked(node),
            "width_px": width or 40,
            "height_px": height or 24,
        }
        bg_color = self._node_color(node, "bg")
        if bg_color:
            item["bg_color"] = bg_color
        return item

    def _slider_item(self, node: "DOMNode") -> Optional[Dict[str, object]]:
        if node.tag == "input":
            input_type = node.attrs.get("type", "").strip().lower()
            if input_type != "range":
                return None

            item: Dict[str, object] = {
                "type": "slider",
                "text": "",
                "width_px": self._node_dimension_px(node, "w") or 160,
                "height_px": self._node_dimension_px(node, "h") or 6,
                "value": int(node.attrs.get("value", "50") or "50"),
            }
            bg_color = self._node_color(node, "bg")
            if bg_color:
                item["bg_color"] = bg_color
            return item

        width = self._node_dimension_px(node, "w") or (160 if self._is_full_width(node) else None)
        height = self._node_dimension_px(node, "h")
        radius = self._node_radius_px(node)
        if self._node_text(node) or width is None or height is None:
            return None
        if width < 48 or height > 12 or radius is None or radius < 9999:
            return None

        fill_candidates = [child for child in node.children if not self._node_text(child)]
        if not fill_candidates:
            return None

        fill = fill_candidates[0]
        percent = self._node_width_percent(fill)
        if percent is None:
            fill_width = self._node_dimension_px(fill, "w")
            if fill_width is not None and width > 0:
                percent = max(0, min(100, int(round(fill_width * 100 / width))))

        if percent is None:
            return None

        item = {
            "type": "slider",
            "text": "",
            "width_px": width,
            "height_px": height,
            "value": percent,
        }
        fill_color = self._node_color(fill, "bg")
        if fill_color:
            item["bg_color"] = fill_color
        return item

    def _checkbox_item(self, node: "DOMNode") -> Optional[Dict[str, object]]:
        if node.tag != "input":
            return None
        input_type = node.attrs.get("type", "").strip().lower()
        if input_type not in {"checkbox", "radio"}:
            return None

        label = (
            node.attrs.get("aria-label", "").strip()
            or node.attrs.get("name", "").strip()
            or "Option"
        )
        return {
            "type": "checkbox",
            "text": label,
            "checked": self._node_checked(node),
        }

    def _dropdown_item(self, node: "DOMNode") -> Optional[Dict[str, object]]:
        if node.tag != "select":
            return None

        options = []
        for child in node.children:
            if child.tag != "option":
                continue
            option_text = self._node_text(child, include_icons=True)
            if option_text:
                options.append(option_text)

        if not options:
            options = ["Option 1", "Option 2", "Option 3"]

        return {
            "type": "dropdown",
            "text": node.attrs.get("name", "").strip() or options[0],
            "options": options,
            "width_px": self._node_dimension_px(node, "w"),
        }

    def _button_item(self, node: "DOMNode") -> Optional[Dict[str, object]]:
        if node.tag != "button" and node.attrs.get("role", "").strip().lower() != "button":
            return None

        text = self._node_text(node)
        icon_name = self._find_first_icon_name(node)
        if not text and not icon_name:
            return None

        item: Dict[str, object] = {
            "type": "button",
            "text": text or icon_name,
        }
        if icon_name:
            item["icon_name"] = icon_name

        for key, value in (
            ("color", self._node_color(node, "text")),
            ("bg_color", self._node_color(node, "bg")),
            ("border_color", self._node_color(node, "border")),
        ):
            if value:
                item[key] = value

        for key, value in (
            ("font_size", self._node_font_size_px(node)),
            ("padding", self._node_padding_px(node)),
            ("width_px", self._node_dimension_px(node, "w")),
            ("height_px", self._node_dimension_px(node, "h")),
            ("radius_px", self._node_radius_px(node)),
        ):
            if value is not None:
                item[key] = value
        return item

    def _text_item(self, node: "DOMNode") -> Optional[Dict[str, object]]:
        if node.tag not in self.BLOCK_TAGS - {"button"}:
            return None

        text = self._node_text(node)
        if not text:
            return None

        item: Dict[str, object] = {
            "type": node.tag,
            "text": text,
        }
        for key, value in (
            ("color", self._node_color(node, "text")),
            ("bg_color", self._node_color(node, "bg")),
            ("font_size", self._node_font_size_px(node)),
            ("padding", self._node_padding_px(node)),
        ):
            if value:
                item[key] = value
        return item

    def _image_item(self, node: "DOMNode") -> Optional[Dict[str, object]]:
        if node.tag != "img":
            return None
        src = node.attrs.get("src", "").strip()
        if not src:
            return None
        return {
            "type": "image",
            "text": node.attrs.get("alt", "").strip() or "Image",
            "src": src,
        }

    def _semantic_item(self, node: "DOMNode") -> Optional[Dict[str, object]]:
        for detector in (
            self._switch_item,
            self._slider_item,
            self._checkbox_item,
            self._dropdown_item,
            self._button_item,
            self._image_item,
            self._text_item,
        ):
            item = detector(node)
            if item is not None:
                return item
        return None

    def _collect_items(self) -> List[Dict[str, object]]:
        items: List[Dict[str, object]] = []

        def walk(node: "DOMNode") -> None:
            if node.tag in {"script", "style", "meta", "link", "head", "title"}:
                return

            item = self._semantic_item(node)
            if item is not None:
                items.append(item)
                return

            for child in node.children:
                walk(child)

        for node in self._roots:
            walk(node)

        return items

    def handle_starttag(self, tag: str, attrs) -> None:
        attrs_dict = dict(attrs)
        node = DOMNode(tag=tag, attrs=attrs_dict)
        if self._node_stack:
            self._node_stack[-1].children.append(node)
        else:
            self._roots.append(node)
        self._node_stack.append(node)

    def handle_endtag(self, tag: str) -> None:
        if not self._node_stack:
            return

        if self._node_stack[-1].tag == tag:
            node = self._node_stack.pop()
        else:
            match_index = next((i for i in range(len(self._node_stack) - 1, -1, -1) if self._node_stack[i].tag == tag), -1)
            if match_index == -1:
                return
            node = self._node_stack[match_index]
            self._node_stack = self._node_stack[:match_index]

        if tag == "title":
            title_text = self._node_text(node, include_icons=True)
            if title_text:
                self.title = title_text

    def handle_data(self, data: str) -> None:
        if not self._node_stack:
            return
        self._node_stack[-1].text_chunks.append(data)


@dataclass
class DOMNode:
    tag: str
    attrs: Dict[str, str]
    children: List["DOMNode"] = field(default_factory=list)
    text_chunks: List[str] = field(default_factory=list)


def choose_font_size(size: int, profile: dict) -> int:
    """Return the font size to pass to ui_font_get()."""
    return size


def extract_html_content(html_path: Path) -> Dict[str, object]:
    parser = HTMLContentExtractor()
    parser.feed(html_path.read_text(encoding="utf-8"))
    parser.items = parser._collect_items()

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


def render_item_code(index: int, item: Dict[str, object], profile: dict) -> List[str]:
    lines: List[str] = []
    item_type = str(item["type"])
    text = c_escape(str(item.get("text", "")))
    color = str(item.get("color", ""))
    bg_color = str(item.get("bg_color", ""))
    border_color = str(item.get("border_color", ""))
    font_size = int(item["font_size"]) if item.get("font_size") is not None else 0
    padding = int(item["padding"]) if item.get("padding") is not None else 0
    width_px = int(item["width_px"]) if item.get("width_px") is not None else 0
    height_px = int(item["height_px"]) if item.get("height_px") is not None else 0
    radius_px = int(item["radius_px"]) if item.get("radius_px") is not None else 0

    if item_type == "h1":
        fs = font_size or 32
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
        fs = font_size or 24
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
        fs = font_size or 18
        btn_bg = bg_color if bg_color else ("0xffffff" if border_color else "0x2563eb")
        btn_tc = color if color else ("0x1f2937" if border_color or btn_bg == "0xffffff" else "0xffffff")
        btn_radius = radius_px or 18
        pad = padding or 12
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_button_create(content);",
            f"    lv_obj_remove_flag(item_{index}, LV_OBJ_FLAG_SCROLLABLE);",
        ])
        if width_px and height_px:
            lines.append(f"    lv_obj_set_size(item_{index}, {width_px}, {height_px});")
        elif width_px:
            lines.append(f"    lv_obj_set_width(item_{index}, {width_px});")
            lines.append(f"    lv_obj_set_height(item_{index}, LV_SIZE_CONTENT);")
        else:
            lines.append(f"    lv_obj_set_width(item_{index}, LV_SIZE_CONTENT);")
        lines.extend([
            f"    lv_obj_set_style_pad_hor(item_{index}, {max(12, pad)}, 0);",
            f"    lv_obj_set_style_pad_ver(item_{index}, {max(6, pad // 2)}, 0);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex({btn_bg}), 0);",
            f"    lv_obj_set_style_bg_opa(item_{index}, {'LV_OPA_TRANSP' if border_color and not bg_color else 'LV_OPA_COVER'}, 0);",
            f"    lv_obj_set_style_radius(item_{index}, {btn_radius}, 0);",
            f"    lv_obj_set_style_border_width(item_{index}, {1 if border_color else 0}, 0);",
        ])
        if border_color:
            lines.append(f"    lv_obj_set_style_border_color(item_{index}, lv_color_hex({border_color}), 0);")
        lines.extend([
            f"    lv_obj_set_style_shadow_width(item_{index}, 0, 0);",
            f"    lv_obj_t * item_{index}_label = lv_label_create(item_{index});",
            f'    lv_label_set_text(item_{index}_label, "{text}");',
            f"    lv_obj_set_style_text_color(item_{index}_label, lv_color_hex({btn_tc}), 0);",
            f"    lv_obj_set_style_text_font(item_{index}_label, ui_font_get({choose_font_size(fs, profile)}), 0);",
            f"    lv_obj_center(item_{index}_label);",
        ])
    elif item_type == "switch":
        track = bg_color if bg_color else "0x2563eb"
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_switch_create(content);",
            f"    lv_obj_set_size(item_{index}, {width_px or 40}, {height_px or 24});",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex(0xd1d5db), LV_PART_MAIN);",
            f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_COVER, LV_PART_MAIN);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, LV_PART_MAIN);",
            f"    lv_obj_set_style_pad_all(item_{index}, 2, LV_PART_MAIN);",
            f"    lv_obj_set_style_radius(item_{index}, LV_RADIUS_CIRCLE, LV_PART_MAIN);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex({track}), LV_PART_INDICATOR);",
            f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_COVER, LV_PART_INDICATOR);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, LV_PART_INDICATOR);",
            f"    lv_obj_set_style_radius(item_{index}, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex(0xffffff), LV_PART_KNOB);",
            f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_COVER, LV_PART_KNOB);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, LV_PART_KNOB);",
            f"    lv_obj_set_style_shadow_width(item_{index}, 0, LV_PART_KNOB);",
            f"    lv_obj_set_style_radius(item_{index}, LV_RADIUS_CIRCLE, LV_PART_KNOB);",
        ])
        if bool(item.get("checked")):
            lines.append(f"    lv_obj_add_state(item_{index}, LV_STATE_CHECKED);")
    elif item_type == "slider":
        fill = bg_color if bg_color else "0x2563eb"
        value = int(item.get("value", 50))
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_slider_create(content);",
            f"    lv_obj_set_size(item_{index}, {width_px or 160}, {height_px or 6});",
            f"    lv_slider_set_range(item_{index}, 0, 100);",
            f"    lv_slider_set_value(item_{index}, {value}, LV_ANIM_OFF);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex(0xe5e7eb), LV_PART_MAIN);",
            f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_COVER, LV_PART_MAIN);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, LV_PART_MAIN);",
            f"    lv_obj_set_style_pad_all(item_{index}, 0, LV_PART_MAIN);",
            f"    lv_obj_set_style_radius(item_{index}, LV_RADIUS_CIRCLE, LV_PART_MAIN);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex({fill}), LV_PART_INDICATOR);",
            f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_COVER, LV_PART_INDICATOR);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, LV_PART_INDICATOR);",
            f"    lv_obj_set_style_radius(item_{index}, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);",
            f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_TRANSP, LV_PART_KNOB);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, LV_PART_KNOB);",
            f"    lv_obj_set_style_shadow_width(item_{index}, 0, LV_PART_KNOB);",
        ])
    elif item_type == "checkbox":
        tc = color if color else "0x1f2937"
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_checkbox_create(content);",
            f'    lv_checkbox_set_text(item_{index}, "{text}");',
            f"    lv_obj_set_style_text_font(item_{index}, ui_font_get({choose_font_size(font_size or 16, profile)}), 0);",
            f"    lv_obj_set_style_text_color(item_{index}, lv_color_hex({tc}), 0);",
            f"    lv_obj_set_style_border_width(item_{index}, 0, LV_PART_MAIN);",
            f"    lv_obj_set_style_border_width(item_{index}, 1, LV_PART_INDICATOR);",
            f"    lv_obj_set_style_border_color(item_{index}, lv_color_hex(0xcbd5e1), LV_PART_INDICATOR);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex(0x2563eb), LV_PART_INDICATOR | LV_STATE_CHECKED);",
            f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);",
        ])
        if bool(item.get("checked")):
            lines.append(f"    lv_obj_add_state(item_{index}, LV_STATE_CHECKED);")
    elif item_type == "dropdown":
        options = item.get("options", [])
        option_text = "\\n".join(c_escape(str(option)) for option in options) if isinstance(options, list) else "Option 1\\nOption 2"
        lines.extend([
            f"    lv_obj_t * item_{index} = lv_dropdown_create(content);",
            f'    lv_dropdown_set_options(item_{index}, "{option_text}");',
            f"    lv_obj_set_width(item_{index}, {width_px or 180});",
            f"    lv_obj_set_style_radius(item_{index}, {radius_px or 10}, 0);",
            f"    lv_obj_set_style_border_width(item_{index}, 1, 0);",
            f"    lv_obj_set_style_border_color(item_{index}, lv_color_hex(0xcbd5e1), 0);",
            f"    lv_obj_set_style_bg_color(item_{index}, lv_color_hex(0xffffff), 0);",
            f"    lv_obj_set_style_bg_opa(item_{index}, LV_OPA_COVER, 0);",
        ])
    elif item_type == "a":
        fs = font_size or 18
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
        fs = font_size or 18
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
    source_type = task.get("input", {}).get("source_type", "html")
    manifest = {
        "task_id": task["task_id"],
        "page_id": task["page_id"],
        "page_name": task["page_name"],
        "profile": profile["id"],
        "viewport": task["target"]["viewport"],
        "driver": "image_to_html_v1+rule_based_html_v1" if source_type == "image" else "rule_based_html_v1",
        "html_title": html_info["title"],
        "item_count": len(html_info["items"]),
        "output_c": task["generation"]["output_c"],
        "output_h": task["generation"]["output_h"],
    }
    (task_path.parent / "generated" / "manifest.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def ensure_html_input(task_path: Path, task: dict) -> Path:
    html_path = resolve(task_path, task["input"]["html_entry"])
    source_type = task.get("input", {}).get("source_type", "html")
    if source_type == "image":
        subprocess.run(
            [sys.executable, str(Path(__file__).resolve().parent / "m1-image-to-html.py"), "--task", str(task_path)],
            check=True,
        )
    return html_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate an LVGL page from task/profile/HTML inputs.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    profile = load_json(resolve(task_path, task["target"]["profile"]))
    source_type = task.get("input", {}).get("source_type", "html")
    html_path = ensure_html_input(task_path, task)
    output_h = resolve(task_path, task["generation"]["output_h"])
    output_c = resolve(task_path, task["generation"]["output_c"])

    if source_type == "reference_only":
        if output_h.is_file() and output_c.is_file():
            print(f"Skipping generation for handwritten task: {task_path}")
            print(f"Using existing source: {output_c}")
            return 0
        raise SystemExit(f"Handwritten task is missing generated files: {output_c} / {output_h}")

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

    driver = "image_to_html_v1+rule_based_html_v1" if source_type == "image" else "rule_based_html_v1"
    print(f"Generated page source: {output_c}")
    print(f"Generated page header: {output_h}")
    print(f"Driver: {driver}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
