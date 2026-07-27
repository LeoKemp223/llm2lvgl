#!/usr/bin/env python3

import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional

# Allow importing llm_client from the same directory
sys.path.insert(0, str(Path(__file__).resolve().parent))
import llm_client


TOOLS_DIR = Path(__file__).resolve().parent
PROMPTS_DIR = TOOLS_DIR / "prompts"
DOCS_DIR = TOOLS_DIR.parent / "docs"


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def inline_assets(html_content: str, input_dir: Path) -> str:
    """Inline external CSS and JS files referenced in the HTML.

    Replaces <link rel="stylesheet" href="style.css"> with <style>...</style>
    and <script src="app.js"></script> with <script>...</script>.
    Only inlines local files that exist in input_dir.
    """
    # Inline CSS: <link rel="stylesheet" href="...">
    def _inline_css(m):
        href = m.group(1)
        if href.startswith(("http://", "https://", "//")):
            return m.group(0)
        css_path = input_dir / href
        if css_path.is_file():
            css = css_path.read_text(encoding="utf-8", errors="replace")
            return f"<style>/* {href} */\n{css}\n</style>"
        return m.group(0)

    html_content = re.sub(
        r'<link\s+[^>]*rel=["\']stylesheet["\'][^>]*href=["\']([^"\']+)["\'][^>]*/?>',
        _inline_css, html_content, flags=re.IGNORECASE,
    )
    # Also match href before rel
    html_content = re.sub(
        r'<link\s+[^>]*href=["\']([^"\']+)["\'][^>]*rel=["\']stylesheet["\'][^>]*/?>',
        _inline_css, html_content, flags=re.IGNORECASE,
    )

    # Inline JS: <script src="..."></script>
    def _inline_js(m):
        src = m.group(1)
        if src.startswith(("http://", "https://", "//")):
            return m.group(0)
        js_path = input_dir / src
        if js_path.is_file():
            js = js_path.read_text(encoding="utf-8", errors="replace")
            return f"<script>/* {src} */\n{js}\n</script>"
        return m.group(0)

    html_content = re.sub(
        r'<script\s+[^>]*src=["\']([^"\']+)["\'][^>]*>\s*</script>',
        _inline_js, html_content, flags=re.IGNORECASE,
    )

    return html_content


def c_escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")


def sanitize_identifier(value: str) -> str:
    value = value.strip().lower()
    value = re.sub(r"[^a-z0-9]+", "_", value)
    value = re.sub(r"_+", "_", value).strip("_")
    return value or "page"


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


def build_generate_prompt(
    page_id: str,
    page_name: str,
    html_content: str,
    profile: dict,
    viewport: dict,
    generation: dict,
    image_path: Optional[Path] = None,
    asset_images: Optional[List[Path]] = None,
    asset_manifest: Optional[dict] = None,
    analysis: Optional[dict] = None,
) -> list:
    """Build the messages list for the LLM code generation call."""
    # Load system prompt template
    system_template = PROMPTS_DIR.joinpath("generate_page.md").read_text("utf-8")

    # Load codegen rules
    rules_path = DOCS_DIR / "llm_codegen_rules.md"
    codegen_rules = rules_path.read_text("utf-8") if rules_path.is_file() else ""

    system_prompt = system_template.replace("{codegen_rules}", codegen_rules)

    # Build user prompt with task context
    user_parts = [
        f"## Task Metadata",
        f"- page_id: `{page_id}`",
        f"- page_name: `{page_name}`",
        f"- viewport: `{viewport['width']}x{viewport['height']}`",
        f"- profile: `{profile.get('id', 'unknown')}`",
        "",
        f"## Profile Constraints",
        f"- allow_freetype: `{generation.get('allow_freetype', False)}`",
        f"- allow_filesystem_assets: `{generation.get('allow_filesystem_assets', False)}`",
        f"- builtin_fonts: `{profile.get('builtin_fonts', [])}`",
        "",
        f"## HTML Source",
        "```html",
        html_content,
        "```",
    ]

    if analysis:
        user_parts.extend([
            "",
            "## Confirmed Page Analysis",
            "The user has reviewed this analysis before generation. Treat interactions and states as implementation requirements, not optional notes.",
            "Use LVGL native widgets and event callbacks for every listed interaction. At minimum, implement visible pressed/checked/selected/value state changes locally even when no backend action exists.",
            "```json",
            json.dumps({
                "page_type": analysis.get("page_type"),
                "contains_images": analysis.get("contains_images"),
                "validation_mode": analysis.get("validation_mode"),
                "elements": analysis.get("elements", []),
                "interactions": analysis.get("interactions", []),
                "states": analysis.get("states", []),
                "layout_notes": analysis.get("layout_notes"),
                "risk_notes": analysis.get("risk_notes", []),
            }, indent=2, ensure_ascii=False),
            "```",
        ])

    if asset_manifest and asset_manifest.get("assets"):
        user_parts.extend([
            "",
            "## Extracted Visual Assets",
            "These crop assets are generated from the original screenshot. Use them as precise visual references for "
            "colors, local layout, decorative details, and target bounding boxes. If filesystem assets are allowed, "
            "you may place them with lv_image_create/lv_image_set_src. If filesystem assets are not allowed, recreate "
            "them with LVGL primitives while matching the viewport_box coordinates.",
            "```json",
            json.dumps(asset_manifest, indent=2, ensure_ascii=False),
            "```",
        ])

    # For image-based tasks, include the original screenshot so the LLM can
    # see the actual design rather than relying solely on the drafted HTML.
    if image_path and image_path.is_file():
        b64 = llm_client.image_to_base64(image_path)
        suffix = image_path.suffix.lower().lstrip(".")
        mime = {"png": "image/png", "jpg": "image/jpeg", "jpeg": "image/jpeg",
                "bmp": "image/bmp", "webp": "image/webp"}.get(suffix, "image/png")
        user_content: list = [
            {"type": "text", "text": "\n".join(user_parts)},
            {"type": "text", "text": "## Original Design Screenshot\nThe image below is the original UI design. "
                                     "Use it as the primary reference for layout, colors, and component placement. "
                                     "The HTML above is only a rough draft."},
            {"type": "image_url", "image_url": {"url": f"data:{mime};base64,{b64}"}},
        ]
        # Append additional asset images (icons, etc.)
        for ap in (asset_images or []):
            if ap.is_file() and ap != image_path:
                ab64 = llm_client.image_to_base64(ap)
                asuffix = ap.suffix.lower().lstrip(".")
                amime = {"png": "image/png", "jpg": "image/jpeg", "jpeg": "image/jpeg",
                         "bmp": "image/bmp", "webp": "image/webp"}.get(asuffix, "image/png")
                user_content.append({"type": "text", "text": f"## Asset: {ap.name}"})
                user_content.append({"type": "image_url", "image_url": {"url": f"data:{amime};base64,{ab64}"}})
        return [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_content},
        ]

    return [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": "\n".join(user_parts)},
    ]


def write_codegen_prompt(task_path: Path, task: dict, profile: dict) -> None:
    prompt_path = task_path.parent / "generated" / "codegen_prompt.md"
    source_type = task.get("input", {}).get("source_type", "html")
    base_driver = "llm_gpt4o"
    driver = f"image_to_html_v1+{base_driver}" if source_type == "image" else base_driver
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
        "## Driver",
        f"- driver: `{driver}`",
    ]
    prompt_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_manifest(task_path: Path, task: dict, profile: dict) -> None:
    source_type = task.get("input", {}).get("source_type", "html")
    base_driver = "llm_gpt4o"
    driver = f"image_to_html_v1+{base_driver}" if source_type == "image" else base_driver
    manifest = {
        "task_id": task["task_id"],
        "page_id": task["page_id"],
        "page_name": task["page_name"],
        "profile": profile["id"],
        "viewport": task["target"]["viewport"],
        "driver": driver,
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
    if source_type == "image" and not html_path.is_file():
        # For image tasks, the LLM uses the original screenshot via vision.
        # Write a minimal placeholder HTML so downstream code has a file to read.
        html_path.parent.mkdir(parents=True, exist_ok=True)
        viewport = task.get("target", {}).get("viewport", {})
        w = viewport.get("width", 480)
        h = viewport.get("height", 320)
        html_path.write_text(
            f'<html><body style="width:{w}px;height:{h}px;margin:0">'
            f"<!-- placeholder for image-based task -->"
            f"</body></html>\n",
            encoding="utf-8",
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

    if not (os.environ.get("OPENAI_API_KEY") or llm_client.load_settings().get("api_key")):
        raise SystemExit("OPENAI_API_KEY not set. Configure it in Web UI settings or set the environment variable.")

    page_id = sanitize_identifier(task["page_id"])
    page_name = task["page_name"]
    viewport = task["target"]["viewport"]
    html_content = html_path.read_text(encoding="utf-8")
    # Inline external CSS/JS assets so the LLM sees the full design
    html_content = inline_assets(html_content, html_path.parent)

    print(f"Generating LVGL code via LLM for: {page_name}")

    # For image-based tasks, pass the original screenshot to the LLM
    image_path = None
    asset_images: List[Path] = []
    if source_type == "image":
        image_rel = task.get("input", {}).get("image_entry")
        if image_rel:
            image_path = resolve(task_path, image_rel)
        # Collect additional asset images from input dir
        input_dir = task_path.parent / "input"
        if input_dir.is_dir():
            img_exts = {".png", ".jpg", ".jpeg", ".bmp", ".webp", ".gif", ".svg"}
            for f in sorted(input_dir.rglob("*")):
                if f.suffix.lower() in img_exts and f.is_file() and f != image_path:
                    asset_images.append(f)
    asset_manifest_path = task_path.parent / "generated" / "asset_manifest.json"
    asset_manifest = load_json(asset_manifest_path) if asset_manifest_path.is_file() else None
    analysis_path = task_path.parent / task.get("analysis", {}).get("output", "analysis/analysis.json")
    analysis = load_json(analysis_path) if analysis_path.is_file() else None

    messages = build_generate_prompt(
        page_id, page_name, html_content, profile, viewport, task["generation"],
        image_path=image_path, asset_images=asset_images, asset_manifest=asset_manifest,
        analysis=analysis,
    )
    response = llm_client.chat(messages)
    c_code = llm_client.extract_code_block(response, "c")

    output_h.parent.mkdir(parents=True, exist_ok=True)
    output_c.parent.mkdir(parents=True, exist_ok=True)
    output_h.write_text(generate_header(page_id), encoding="utf-8")
    output_c.write_text(c_code, encoding="utf-8")

    write_codegen_prompt(task_path, task, profile)
    write_manifest(task_path, task, profile)

    base_driver = "llm_gpt4o"
    driver = f"image_to_html_v1+{base_driver}" if source_type == "image" else base_driver
    print(f"Generated page source: {output_c}")
    print(f"Generated page header: {output_h}")
    print(f"Driver: {driver}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
