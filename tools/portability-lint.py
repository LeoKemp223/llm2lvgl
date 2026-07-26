#!/usr/bin/env python3

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Dict, List


ABSOLUTE_PATH_PATTERN = re.compile(r'(?:"|<)(/usr/|/home/|/tmp/|/opt/|/var/|/etc/|/root/|/srv/)')
WINDOWS_PATH_PATTERN = re.compile(r"[A-Za-z]:\\\\")
SDL_PATTERN = re.compile(r"\blv_sdl_|\bSDL_")
FREETYPE_PATTERN = re.compile(r"\blv_freetype_")
STDIO_FS_PATTERN = re.compile(r"\b(fopen|open|freopen)\s*\(")
LIBC_ALLOC_PATTERN = re.compile(r"\b(malloc|calloc|realloc|free)\s*\(")
LIBC_PRINTF_PATTERN = re.compile(r"\b(printf|fprintf|sprintf)\s*\(")
LINE_COMMENT_PATTERN = re.compile(r"//.*$", re.MULTILINE)
BLOCK_COMMENT_PATTERN = re.compile(r"/\*.*?\*/", re.DOTALL)
ICON_WITH_BODY_FONT_PATTERN = re.compile(
    r"lv_label_set_text\s*\([^;]*(?:LV_SYMBOL_|\\x[0-9A-Fa-f]{2})[^;]*;(?:(?!lv_label_set_text).)*?"
    r"lv_obj_set_style_text_font\s*\([^;]*ui_font_get\s*\(",
    re.DOTALL,
)
ICON_HELPER_BODY_FONT_PATTERN = re.compile(
    r"(?:static\s+)?(?:lv_obj_t\s*\*\s*)?\w*icon\w*\s*\([^)]*\)\s*\{(?:(?!\n\}).)*ui_font_get\s*\(",
    re.DOTALL | re.IGNORECASE,
)


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve_task_path(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def lint_file(path: Path, profile: dict, task: dict) -> List[Dict[str, object]]:
    findings = []
    try:
        content = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        findings.append({
            "severity": "error",
            "path": str(path),
            "message": "File is not valid UTF-8 text; portability lint expects source text.",
        })
        return findings

    # Strip comments for pattern matching to avoid false positives
    stripped = BLOCK_COMMENT_PATTERN.sub("", content)
    stripped = LINE_COMMENT_PATTERN.sub("", stripped)

    allow_sdl = bool(profile["constraints"].get("allow_sdl_only_api", False))
    allow_freetype = bool(profile["fonts"].get("allow_freetype", False)) and bool(task["generation"].get("allow_freetype", False))
    allow_filesystem = bool(profile["assets"].get("allow_filesystem", False)) and bool(task["generation"].get("allow_filesystem_assets", False))

    checks = [
        ("error", ABSOLUTE_PATH_PATTERN, "Contains absolute host filesystem path."),
        ("error", WINDOWS_PATH_PATTERN, "Contains Windows host filesystem path."),
        ("warning", LIBC_ALLOC_PATTERN, "Uses libc malloc/free; prefer lv_malloc/lv_free for portability."),
        ("warning", LIBC_PRINTF_PATTERN, "Uses libc printf; prefer LV_LOG_* for portability."),
    ]

    if not allow_sdl:
        checks.append(("error", SDL_PATTERN, "Uses simulator-only SDL API."))
    if not allow_freetype:
        checks.append(("error", FREETYPE_PATTERN, "Uses FreeType while the active profile forbids it."))
    if not allow_filesystem:
        checks.append(("error", STDIO_FS_PATTERN, "Uses filesystem access while the active profile forbids it."))

    for severity, pattern, message in checks:
        for match in pattern.finditer(stripped):
            line = stripped.count("\n", 0, match.start()) + 1
            findings.append({
                "severity": severity,
                "path": str(path),
                "line": line,
                "message": message,
            })

    for match in ICON_WITH_BODY_FONT_PATTERN.finditer(stripped):
        line = stripped.count("\n", 0, match.start()) + 1
        findings.append({
            "severity": "warning",
            "path": str(path),
            "line": line,
            "message": "Icon label appears to use ui_font_get(); use ui_icon_font_get() for LV_SYMBOL or Material icon text.",
        })

    for match in ICON_HELPER_BODY_FONT_PATTERN.finditer(stripped):
        line = stripped.count("\n", 0, match.start()) + 1
        findings.append({
            "severity": "warning",
            "path": str(path),
            "line": line,
            "message": "Icon helper appears to use ui_font_get(); icon helpers should use ui_icon_font_get().",
        })

    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description="Run portability lint on generated or mapped LVGL page sources.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)
    profile_path = resolve_task_path(task_path, task["target"]["profile"])
    profile = load_json(profile_path)
    if "version" not in profile:
        print(f"WARNING: Profile missing 'version' field: {profile_path}", file=sys.stderr)

    candidate_files = []
    for key in ("output_c", "output_h"):
        value = task["generation"].get(key)
        if value:
            path = resolve_task_path(task_path, value)
            if path.exists():
                candidate_files.append(path)

    if not candidate_files:
        print("No generated source files found for lint. Nothing to check yet.")
        return 0

    findings = []
    for path in candidate_files:
        findings.extend(lint_file(path, profile, task))

    if not findings:
        print("Portability lint passed.")
        return 0

    error_count = 0
    for finding in findings:
        suffix = f":{finding['line']}" if "line" in finding else ""
        print(f"{finding['severity'].upper()}: {finding['path']}{suffix}: {finding['message']}")
        if finding["severity"] == "error":
            error_count += 1

    print(f"Total findings: {len(findings)}")
    return 1 if error_count else 0


if __name__ == "__main__":
    raise SystemExit(main())
