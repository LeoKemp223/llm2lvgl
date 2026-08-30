# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LLM-driven pipeline that turns HTML pages or screenshots into embedded LVGL 9.x C code, then validates it in a headless SDL simulator by screenshot-diffing against a reference:

```
image/HTML → generate-page.py (LLM) → generated/<page_id>.c/.h
            → sync-generated-pages.py → runtime_project CMake build → lvgl_runtime_demo
            → headless render + screenshot → page-validate.py (pixel diff vs reference.png)
            → report.json + diff.png → refine-page.py (LLM search-replace loop until diff improves / max iterations)
            → export-page.py (portable .c/.h bundle)
```

`tools/pipeline.sh` is the single CLI entrypoint.

## Environment (critical)

- **System `python3` (/usr/bin/python3) lacks `httpx` and modern Pillow.** All Python tooling MUST run through the project venv: `PATH="$(pwd)/.venv/bin:$PATH" make ...` or `.venv/bin/python3 <script>`. `tools/pipeline.sh` internally calls bare `python3`, so it must be invoked as `PATH="$(pwd)/.venv/bin:$PATH" bash tools/pipeline.sh ...`. Install Py deps with `.venv/bin/pip install` (system pip is PEP 668-blocked).
- venv already contains: Pillow≥9.1, httpx, flask, playwright, socksio.
- LLM credentials live in **`workspace/.llm_settings.json`** (gitignored): `{"api_key", "model", "base_url"}`. Optional: `"thinking": "enabled"` to re-enable model reasoning. Env fallbacks: `OPENAI_API_KEY` / `OPENAI_MODEL` / `OPENAI_BASE_URL`.
- LLM config resolution is in `tools/llm_client.py` (`load_settings`, `_get_config`). Two endpoint styles are supported: `/chat/completions` (default) and `/responses` (reasoning/Codex models, chosen by `api_type` or model-name hints like `gpt-5`/`o1`/`codex`).

## Common Commands

```bash
# Environment self-check
PATH="$(pwd)/.venv/bin:$PATH" bash tools/pipeline.sh doctor

# Built-in demo task end-to-end
PATH="$(pwd)/.venv/bin:$PATH" bash tools/pipeline.sh quickstart

# Per-stage pipeline (replace <task.json> with a workspace/tasks/<id>/task.json path)
bash tools/pipeline.sh init   tools/pipeline.sh generate <task.json>   # LLM page code
bash tools/pipeline.sh render-ref <task.json>   # HTML → reference.png
bash tools/pipeline.sh lint <task.json>         # portability lint
bash tools/pipeline.sh run <task.json>          # build + render + validate (+ LLM refine loop)
bash tools/pipeline.sh export <task.json>       # portable bundle → artifacts/export/

# Web UI (browser: drag-drop, one-click pipeline, log streaming)
.venv/bin/python3 tools/webui.py                  # http://127.0.0.1:5000
.venv/bin/python3 tools/webui.py --host 0.0.0.0   # expose to LAN

# Self-test for search-replace utilities
.venv/bin/python3 tools/llm_client.py
```

The `run` gate requires `analysis.confirmed: true` in `task.json` (set by `analyze-page.py --confirm` or manually).

## Architecture

### Task workspace

`workspace/tasks/<task_id>/task.json` is the first-class unit (`workspace/task.schema.json` defines it):
`input/` (index.html, assets, source.png for image tasks), `reference/` (reference.png), `generated/` (page .c/.h, manifest.json, codegen_prompt.md, asset_manifest.json), `artifacts/` (current.png, diff.png, report.json), `export/`.

### Key tools

- **`generate-page.py`** — builds the LLM prompt (`tools/prompts/generate_page.md` + `docs/llm_codegen_rules.md` + task context + base64 screenshot). For image tasks it downscales the screenshot to max 1024px (env `LVGL_VISION_MAX_DIM`) before sending. Extracts the first ` ```c ` block. Retries up to 3× on empty output.
- **`llm_client.py`** — OpenAI-compatible client. Uses **non-streaming-first** for `/chat/completions` (DeepSeek reasoning models hang for minutes when streaming `reasoning_content`; a plain call returns `message.content` immediately). Sends `thinking: {"type":"disabled"}` by default. Falls back to streaming only if non-streaming fails.
- **`refine-page.py`** — iterative visual refinement. Sends current source + validation report + 3-panel diff image to the LLM; the LLM replies with `<<<SEARCH ... === ... >>>` blocks (parsed by `llm_client.extract_search_replace_blocks`); applies them, rebuilds, re-validates, keeps the change only if metrics improve (`tools/prompts/refine_page.md` governs).
- **`task-run.py`** — main run driver: syncs generated pages, configures the runtime build dir, executes the build, produces the screenshot and validation report.
- **`sync-generated-pages.py`** — discovers all `workspace/tasks/*/task.json`, regenerates `runtime_project/build/generated_page_registry.{c,h}`, which the runtime CMake compiles into `lvgl_runtime_demo`.
- **`image-to-html.py` / `asset-plan.py` / `asset-extract.py`** — image tasks: screenshot → coarse HTML draft + asset crops (used as visual references by the LLM).
- **`portability-lint.py`** — enforces `docs/llm_codegen_rules.md`: no SDL/main/setenv/absolute paths, LVGL 9.x-only API, `ui_font_get()` fonts only.

### Runtime

`runtime_project/` is the LVGL 9.6 simulator (submodules `lv_port_linux_test`). Generated pages are compiled in via the registry bridge: `sync-generated-pages.py` writes the registry, `CMakeLists.txt` includes it. `runtime_project/build/<task_id>/` holds per-task build dirs. Pages render headless and get screenshotted by `page-validate.py` (produces `diff_ratio`, `mean_abs_diff`, and a `diff.png` heatmap).

### Board profiles

`profiles/*.json` constrain output per target (sim 1280x800, esp32 480x320, stm32 800x480, plus custom_* from uploaded images): resolution, color depth, DPI, font policy (builtin vs FreeType), filesystem-asset policy, allowed simulator APIs. Profile id is stored in `task.json → target.profile`.

## Rules and constraints that frequently bite

- Generated code targets **LVGL 9.x only**: `lv_button_create`, `lv_image_create`, `lv_obj_remove_flag`, `lv_obj_set_style_pad_row/column`, `lv_display_*`. LVGL 8 names fail the lint.
- `LV_OPA_*` only sells multiples of 10 (plus `LV_OPA_TRANSP`/`LV_OPA_COVER`); `lv_color_hex()` takes `uint32_t`; `lv_color_t` has no `.full` (use `lv_color_eq`).
- The refine/build-fix loop has historically **gutted pages down to blank containers** to satisfy the compiler. `refine_page.md` now warns the LLM to preserve all widgets; check for regression if a "passing but blank" page appears.
- DeepSeek vision models (`deepseek-*-vision-exp`) burn tokens via reasoning when thinking is on — keep it disabled for pipeline calls.
- Image tasks: the generated HTML is a rough draft only; the screenshot is the authoritative reference (also passed to the LLM as a base64 image).