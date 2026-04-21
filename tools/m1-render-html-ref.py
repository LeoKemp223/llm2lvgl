#!/usr/bin/env python3

import argparse
import json
import shutil
import subprocess
from pathlib import Path


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve(task_path: Path, value: str) -> Path:
    return (task_path.parent / value).resolve()


def file_url(path: Path) -> str:
    return path.resolve().as_uri()


def chromium_candidates() -> list:
    return [
        "chromium",
        "chromium-browser",
        "google-chrome",
        "google-chrome-stable",
    ]


def detect_browser() -> tuple:
    for candidate in chromium_candidates():
        path = shutil.which(candidate)
        if path:
            return ("chromium", path)

    wkhtmltoimage = shutil.which("wkhtmltoimage")
    if wkhtmltoimage:
        return ("wkhtmltoimage", wkhtmltoimage)

    return ("", "")


def render_with_chromium(binary: str, html_path: Path, output_path: Path, width: int, height: int) -> None:
    cmd = [
        binary,
        "--headless",
        "--disable-gpu",
        "--hide-scrollbars",
        f"--window-size={width},{height}",
        f"--screenshot={output_path}",
        "--allow-file-access-from-files",
        file_url(html_path),
    ]
    subprocess.run(cmd, check=True)


def render_with_wkhtmltoimage(binary: str, html_path: Path, output_path: Path, width: int, height: int) -> None:
    cmd = [
        binary,
        "--enable-local-file-access",
        "--width",
        str(width),
        "--height",
        str(height),
        str(html_path),
        str(output_path),
    ]
    subprocess.run(cmd, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Render a task HTML input to reference/reference.png.")
    parser.add_argument("--task", required=True, help="Path to task.json")
    args = parser.parse_args()

    task_path = Path(args.task).resolve()
    task = load_json(task_path)

    if not task["reference"].get("render_from_html", False):
        print("Task does not request HTML reference rendering. Nothing to do.")
        return 0

    html_path = resolve(task_path, task["input"]["html_entry"])
    output_path = resolve(task_path, task["reference"]["image"])
    width = int(task["target"]["viewport"]["width"])
    height = int(task["target"]["viewport"]["height"])

    if not html_path.is_file():
        raise SystemExit(f"HTML entry not found: {html_path}")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    kind, binary = detect_browser()
    if not binary:
        raise SystemExit(
            "No supported HTML screenshot tool found. Install chromium/chromium-browser/google-chrome or wkhtmltoimage."
        )

    if kind == "chromium":
        render_with_chromium(binary, html_path, output_path, width, height)
    else:
        render_with_wkhtmltoimage(binary, html_path, output_path, width, height)

    print(f"Rendered reference image: {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
