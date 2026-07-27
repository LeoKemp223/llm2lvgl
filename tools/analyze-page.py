#!/usr/bin/env python3

import argparse
import json
import os
import re
import sys
from html.parser import HTMLParser
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import llm_client


VALIDATION_MODES = {"pixel", "layout_semantic", "hybrid", "manual_review"}
PAGE_TYPES = {"text_controls", "image_heavy", "mixed", "reference_only"}


class ElementParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.elements: list[dict[str, Any]] = []
        self.interactions: list[dict[str, Any]] = []
        self._stack: list[dict[str, Any]] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attrs_map = {k.lower(): v or "" for k, v in attrs}
        interactive = _interaction_for_tag(tag, attrs_map)
        if tag in {"h1", "h2", "h3", "p", "button", "a", "input", "select", "textarea", "img", "label"}:
            item: dict[str, Any] = {
                "type": tag,
                "role": _role_for_tag(tag),
                "content": "",
                "priority": "high" if tag in {"h1", "button", "input", "select", "textarea"} else "medium",
            }
            if tag == "img":
                item["content"] = attrs_map.get("alt") or attrs_map.get("src") or "image"
                item["src"] = attrs_map.get("src", "")
            if tag == "input":
                item["content"] = attrs_map.get("placeholder") or attrs_map.get("value") or attrs_map.get("type", "input")
            if tag in {"img", "input"}:
                self.elements.append(item)
                if interactive:
                    interactive["label"] = item.get("content") or interactive["label"]
                    self.interactions.append(interactive)
                return
            if interactive:
                item["interaction"] = interactive
            self._stack.append(item)
        elif interactive:
            self._stack.append({
                "type": tag,
                "role": interactive.get("role", "交互控件"),
                "content": interactive.get("label", ""),
                "priority": "high",
                "interaction": interactive,
            })

    def handle_startendtag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        self.handle_starttag(tag, attrs)

    def handle_data(self, data: str) -> None:
        text = " ".join(data.split())
        if text and self._stack:
            self._stack[-1]["content"] = (self._stack[-1].get("content", "") + " " + text).strip()

    def handle_endtag(self, tag: str) -> None:
        if not self._stack:
            return
        item = self._stack[-1]
        if item.get("type") == tag:
            self._stack.pop()
            if item.get("content") or tag in {"img", "input", "select", "textarea"}:
                self.elements.append(item)
                if item.get("interaction"):
                    interaction = dict(item["interaction"])
                    interaction["label"] = item.get("content") or interaction.get("label") or tag
                    self.interactions.append(interaction)


def _role_for_tag(tag: str) -> str:
    return {
        "h1": "title",
        "h2": "section_title",
        "h3": "section_title",
        "p": "body_text",
        "button": "action",
        "a": "link",
        "input": "input",
        "select": "input",
        "textarea": "input",
        "img": "image",
        "label": "label",
    }.get(tag, "content")


def _interaction_for_tag(tag: str, attrs: dict[str, str]) -> dict[str, Any] | None:
    role = attrs.get("role", "").lower()
    input_type = attrs.get("type", "").lower()
    style = attrs.get("style", "").lower()
    class_name = attrs.get("class", "").lower()
    label = attrs.get("aria-label") or attrs.get("title") or attrs.get("placeholder") or attrs.get("value") or ""
    has_click = any(k.startswith("on") for k in attrs) or "cursor:pointer" in style.replace(" ", "")
    looks_menu = any(word in class_name for word in ("tab", "menu", "nav", "dropdown", "select"))
    looks_toggle = any(word in class_name for word in ("switch", "toggle"))
    looks_slider = any(word in class_name for word in ("slider", "range"))

    control_type = None
    lvgl_widget = None
    expected_behavior = ""

    if tag == "button" or role == "button" or has_click:
        control_type = "button"
        lvgl_widget = "lv_button"
        expected_behavior = "点击后触发对应操作或切换状态"
    if tag == "a":
        control_type = "link_button"
        lvgl_widget = "lv_button"
        expected_behavior = "点击后进入目标页面或触发导航"
    if tag == "select" or role in {"combobox", "listbox"} or looks_menu:
        control_type = "dropdown"
        lvgl_widget = "lv_dropdown"
        expected_behavior = "点击后展开选项并更新当前选中项"
    if tag == "textarea" or input_type in {"text", "password", "email", "number", "search"}:
        control_type = "text_input"
        lvgl_widget = "lv_textarea"
        expected_behavior = "可输入或编辑文本"
    if input_type in {"checkbox", "radio"}:
        control_type = "checkbox" if input_type == "checkbox" else "radio"
        lvgl_widget = "lv_checkbox"
        expected_behavior = "点击后切换选中状态"
    if input_type == "range" or role == "slider" or looks_slider:
        control_type = "slider"
        lvgl_widget = "lv_slider"
        expected_behavior = "拖动后改变数值"
    if role == "switch" or looks_toggle:
        control_type = "switch"
        lvgl_widget = "lv_switch"
        expected_behavior = "点击后在开启和关闭之间切换"

    if not control_type:
        return None

    return {
        "type": control_type,
        "label": label,
        "role": "交互控件",
        "initial_state": "默认状态",
        "expected_behavior": expected_behavior,
        "lvgl_widget": lvgl_widget,
        "priority": "high",
    }


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text("utf-8"))


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def inline_assets(html_content: str, input_dir: Path) -> str:
    def _inline_css(match: re.Match[str]) -> str:
        href = match.group(1)
        if href.startswith(("http://", "https://", "//")):
            return match.group(0)
        css_path = input_dir / href
        if css_path.is_file():
            css = css_path.read_text(encoding="utf-8", errors="replace")
            return f"<style>/* {href} */\n{css}\n</style>"
        return match.group(0)

    html_content = re.sub(
        r'<link\s+[^>]*rel=["\']stylesheet["\'][^>]*href=["\']([^"\']+)["\'][^>]*/?>',
        _inline_css,
        html_content,
        flags=re.IGNORECASE,
    )
    html_content = re.sub(
        r'<link\s+[^>]*href=["\']([^"\']+)["\'][^>]*rel=["\']stylesheet["\'][^>]*/?>',
        _inline_css,
        html_content,
        flags=re.IGNORECASE,
    )
    return html_content


def extract_json_object(text: str) -> dict[str, Any]:
    fenced = re.search(r"```json\s*(.*?)```", text, re.DOTALL | re.IGNORECASE)
    if fenced:
        text = fenced.group(1)
    else:
        start = text.find("{")
        end = text.rfind("}")
        if start >= 0 and end > start:
            text = text[start:end + 1]
    return json.loads(text)


def classify(source_type: str, elements: list[dict[str, Any]], asset_count: int) -> tuple[str, str, bool]:
    image_count = sum(1 for item in elements if item.get("type") == "img")
    contains_images = source_type == "image" or image_count > 0 or asset_count > 0
    interactive_count = sum(1 for item in elements if item.get("type") in {"button", "a", "input", "select", "textarea"})
    text_count = sum(1 for item in elements if item.get("type") in {"h1", "h2", "h3", "p", "label", "button", "a"})

    if source_type == "reference_only":
        return "reference_only", "manual_review", contains_images
    if source_type == "image":
        return "image_heavy", "manual_review", True
    if contains_images and text_count + interactive_count > 0:
        return "mixed", "manual_review", True
    if contains_images:
        return "image_heavy", "manual_review", True
    return "text_controls", "pixel", False


def heuristic_analysis(task_path: Path, task: dict[str, Any], html_content: str) -> dict[str, Any]:
    parser = ElementParser()
    parser.feed(html_content)
    input_dir = resolve(task_path, task["input"]["assets_dir"])
    asset_count = len([p for p in input_dir.glob("*") if p.is_file()]) if input_dir.is_dir() else 0
    page_type, validation_mode, contains_images = classify(
        task.get("input", {}).get("source_type", "html"),
        parser.elements,
        asset_count,
    )
    return {
        "schema_version": 1,
        "driver": "heuristic_v1",
        "task_id": task["task_id"],
        "page_id": task["page_id"],
        "page_type": page_type,
        "contains_images": contains_images,
        "validation_mode": validation_mode,
        "elements": parser.elements[:80],
        "interactions": parser.interactions[:60],
        "states": [],
        "layout_notes": "已根据 HTML 标签和上传资源文件完成启发式分析。",
        "risk_notes": [
            "页面包含图片或素材内容，不适合使用严格的整屏像素级对比。"
        ] if validation_mode == "manual_review" else [],
        "confirmed": False,
    }


def llm_analysis(
    task_path: Path,
    task: dict[str, Any],
    html_content: str,
    fallback: dict[str, Any],
) -> dict[str, Any]:
    source_type = task.get("input", {}).get("source_type", "html")
    viewport = task["target"]["viewport"]
    prompt = (
        "Analyze the uploaded UI page for LVGL implementation planning.\n"
        "Return JSON only. Do not include markdown.\n"
        "Required JSON fields: page_type, contains_images, validation_mode, elements, interactions, states, layout_notes, risk_notes.\n"
        "Allowed page_type values: text_controls, image_heavy, mixed, reference_only.\n"
        "Allowed validation_mode values: pixel, layout_semantic, hybrid, manual_review.\n"
        "All human-readable values inside elements.content, elements.role, interactions.label, interactions.role, interactions.initial_state, interactions.expected_behavior, states.name, states.description, layout_notes, and risk_notes must be Simplified Chinese.\n"
        "Keep only enum fields page_type and validation_mode in English because downstream code depends on them.\n"
        "Use pixel only when the page is mainly text, simple controls, vector-like boxes, and no real photos/images.\n"
        "Use manual_review when the source is a screenshot/image or contains meaningful bitmap imagery.\n"
        "For elements, include objects with type, content, role, priority, and optional bbox_guess.\n"
        "For interactions, enumerate EVERY dynamic control, even if it is visually represented by an icon, tab, card, menu, or custom shape. Include type, label, role, initial_state, expected_behavior, lvgl_widget, priority, and optional bbox_guess.\n"
        "Interaction types should include button, icon_button, tab, menu_item, dropdown, switch, slider, checkbox, radio, text_input, gesture_area, carousel, modal_trigger, back_button, next_button when applicable.\n"
        "For states, include UI state groups such as active tab, selected menu item, switch on/off, slider value, expanded/collapsed menu, dialog open/closed, page transition, or disabled/pressed states.\n"
        "Prefer concrete LVGL widgets in lvgl_widget: lv_button, lv_switch, lv_slider, lv_dropdown, lv_checkbox, lv_textarea, lv_arc, lv_roller, lv_bar.\n\n"
        f"Task: {task['task_id']} / {task['page_name']}\n"
        f"Source type: {source_type}\n"
        f"Viewport: {viewport['width']}x{viewport['height']}\n\n"
        "HTML source:\n"
        f"{html_content[:24000]}"
    )
    content: Any = prompt
    image_path = None
    if source_type == "image":
        image_rel = task.get("input", {}).get("image_entry")
        if image_rel:
            image_path = resolve(task_path, image_rel)
    if image_path and image_path.is_file():
        suffix = image_path.suffix.lower().lstrip(".")
        mime = {"png": "image/png", "jpg": "image/jpeg", "jpeg": "image/jpeg", "bmp": "image/bmp", "webp": "image/webp"}.get(suffix, "image/png")
        content = [
            {"type": "text", "text": prompt},
            {"type": "image_url", "image_url": {"url": f"data:{mime};base64,{llm_client.image_to_base64(image_path)}"}},
        ]

    response = llm_client.chat(
        [
            {"role": "system", "content": "You classify UI pages and return strict JSON for an LVGL code-generation pipeline."},
            {"role": "user", "content": content},
        ],
        temperature=0.1,
        max_tokens=4096,
    )
    data = extract_json_object(response)
    data["schema_version"] = 1
    data["driver"] = "llm_v1"
    data["task_id"] = task["task_id"]
    data["page_id"] = task["page_id"]
    data["page_type"] = data.get("page_type") if data.get("page_type") in PAGE_TYPES else fallback["page_type"]
    data["validation_mode"] = data.get("validation_mode") if data.get("validation_mode") in VALIDATION_MODES else fallback["validation_mode"]
    data["contains_images"] = bool(data.get("contains_images", fallback["contains_images"]))
    if not isinstance(data.get("elements"), list):
        data["elements"] = fallback["elements"]
    if not isinstance(data.get("interactions"), list):
        data["interactions"] = fallback.get("interactions", [])
    if not isinstance(data.get("states"), list):
        data["states"] = fallback.get("states", [])
    if not isinstance(data.get("risk_notes"), list):
        data["risk_notes"] = fallback["risk_notes"]
    data.setdefault("layout_notes", fallback["layout_notes"])
    data["confirmed"] = False
    return data


def update_task_with_analysis(task_path: Path, task: dict[str, Any], analysis: dict[str, Any], confirm: bool) -> None:
    task.setdefault("analysis", {})
    task["analysis"].setdefault("enabled", True)
    task["analysis"].setdefault("output", "analysis/analysis.json")
    task["analysis"].setdefault("require_user_confirm", True)
    task["analysis"]["confirmed"] = bool(confirm)
    if confirm:
        task["analysis"]["confirmed_at"] = __import__("datetime").datetime.now().isoformat(timespec="seconds")

    task.setdefault("validation", {})
    task["validation"]["mode"] = analysis.get("validation_mode", "manual_review")
    task_path.write_text(json.dumps(task, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze a task input before LVGL generation.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    parser.add_argument("--confirm", action="store_true", help="Mark the current analysis as user-confirmed and update task.json")
    parser.add_argument("--no-llm", action="store_true", help="Use heuristic analysis even when LLM settings are configured")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    analysis_rel = task.get("analysis", {}).get("output", "analysis/analysis.json")
    analysis_path = resolve(task_path, analysis_rel)

    if args.confirm:
        if not analysis_path.is_file():
            raise SystemExit(f"Analysis file not found: {analysis_path}")
        analysis = load_json(analysis_path)
        analysis["confirmed"] = True
        analysis_path.write_text(json.dumps(analysis, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        update_task_with_analysis(task_path, task, analysis, True)
        print(f"Analysis confirmed: {analysis_path}")
        return 0

    html_path = resolve(task_path, task["input"]["html_entry"])
    html_content = html_path.read_text("utf-8", errors="replace") if html_path.is_file() else ""
    html_content = inline_assets(html_content, html_path.parent if html_path else task_path.parent)
    fallback = heuristic_analysis(task_path, task, html_content)

    analysis = fallback
    if not args.no_llm and (os.environ.get("OPENAI_API_KEY") or llm_client.load_settings().get("api_key")):
        try:
            analysis = llm_analysis(task_path, task, html_content, fallback)
        except Exception as exc:
            fallback["risk_notes"].append(f"LLM analysis failed, used heuristic fallback: {exc}")
            analysis = fallback

    analysis_path.parent.mkdir(parents=True, exist_ok=True)
    analysis_path.write_text(json.dumps(analysis, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    update_task_with_analysis(task_path, task, analysis, False)
    print(f"Analysis written: {analysis_path}")
    print(f"Page type: {analysis['page_type']}")
    print(f"Validation mode: {analysis['validation_mode']}")
    print(f"Contains images: {analysis['contains_images']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
