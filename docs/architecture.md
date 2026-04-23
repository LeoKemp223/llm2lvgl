# LVGL Agent Architecture

## Goal

Keep the repository on a task-driven path for:

1. ingesting user HTML and assets
2. generating LVGL page code
3. validating the generated page in the SDL simulator
4. exporting portable `.c/.h` output for embedded projects

## Current State

The repository is no longer just a hand-written page sandbox.

It now has a working task pipeline on top of the original simulator loop:

- `workspace/tasks/<task_id>/task.json` is the first-class task entry
- `tools/task-init.py` scaffolds new tasks
- `tools/generate-page.py` generates LVGL page code from HTML input
- `tools/render-html-ref.py` renders HTML reference screenshots
- `tools/portability-lint.py` enforces portability constraints
- `tools/task-run.py` bridges generated tasks into the current simulator build
- `tools/export-page.py` exports generated output as a portable bundle
- `tools/pipeline.sh` is the unified CLI entrypoint

The legacy internal layer is still present and still important:

- `runtime_project/` remains the executable LVGL runtime and screenshot target
- `tools/lvgl-runtime.sh` handles configure/build/run/screenshot operations
- `tools/page-validate.py` produces `diff.png` and `report.json`
- `tools/page-flow.sh` remains the compatibility path for legacy page tasks

## Target Flow

```text
HTML + assets + board profile
            |
            v
     workspace/tasks/<task_id>/task.json
            |
            v
      Code generation (rule-based / LLM)
            |
            v
      generated/<page>.c/.h
            |
            v
   portability lint + bridge sync
            |
            v
      simulator build + screenshot
            |
            v
 screenshot + diff + report.json
            |
            v
 export portable bundle for firmware repo
```

## Workspace Layout

```text
workspace/
├── task.schema.json
├── README.md
└── tasks/
    └── <task_id>/
        ├── task.json
        ├── input/
        │   ├── index.html
        │   ├── assets/
        │   └── notes.md
        ├── reference/
        │   └── reference.png
        ├── generated/
        │   ├── <page_id>.c
        │   ├── <page_id>.h
        │   ├── manifest.json
        │   └── codegen_prompt.md
        ├── artifacts/
        │   ├── current.png
        │   ├── full.png
        │   ├── diff.png
        │   └── report.json
        └── export/
            └── portable_bundle/
```

This layout is implemented and used by the current `workspace/tasks/*` examples.

## Board Profiles

Board profiles live in `profiles/` and define the constraints that matter for code generation and export:

- screen width and height
- color depth
- DPI
- font policy
- asset policy
- whether filesystem access is allowed
- whether simulator-only APIs are allowed

This prevents the generator from silently depending on desktop-only features such as FreeType font loading or SDL APIs.

Profiles such as `sim_1280x800.json`, `sim_480x480.json`, `stm32_800x480.json`, and
`esp32_480x320.json` already exist in the repository.

## Pipeline Commands

The main task entrypoint is:

```bash
tools/pipeline.sh doctor
tools/pipeline.sh init <task-dir>
tools/pipeline.sh generate <task.json>
tools/pipeline.sh render-ref <task.json>
tools/pipeline.sh sync
tools/pipeline.sh lint <task.json>
tools/pipeline.sh run <task.json>
tools/pipeline.sh export <task.json>
```

Current implementation status:

- `doctor`: implemented
- `init`: implemented
- `generate`: implemented
- `render-ref`: implemented
- `sync`: implemented
- `lint`: implemented
- `run`: implemented through `tools/task-run.py`, which still reuses the legacy simulator and diff validation path
- `export`: implemented for generated output bundles

Pending:

- hardening around sync/build race conditions and duplicate page handling

## Runtime Integration

Generated tasks do not compile in isolation.

The current runtime path is:

1. generate task-local `.c/.h`
2. sync all generated pages into `runtime_project/build/generated_page_registry.*`
3. let `runtime_project/CMakeLists.txt` include the generated source list
4. build `lvgl_runtime_demo`
5. run screenshots against the selected `page_id`

This means automatic generated-page registration and CMake source discovery are already present,
but they are implemented as a bridge into the existing runtime rather than as a new standalone runner.

## Implementation Status

### Implemented

- task workspace schema and scaffolding
- board-profile based generation constraints
- HTML reference rendering for task validation
- rule-based HTML-to-LVGL generation
- portability lint before simulator validation
- generated-page auto-sync into the runtime build
- portable bundle export
- LLM-driven code generation via OpenAI-compatible API (`tools/generate-page.py`, `tools/llm_client.py`)
- LLM-driven visual refinement with search-replace patching (`tools/refine-page.py`)
- Web UI for browser-based task management (`tools/webui.py`, `tools/web/`)

### In Progress / Remaining Gaps

- validation is still screenshot-driven, even when the reference comes from HTML
- the runtime still depends on the legacy executable bridge instead of a dedicated task-native runner
- exported bundles are portable page artifacts, not a full firmware integration package
- multi-task sync robustness still needs hardening

## Immediate Risks

- current demo pages still contain absolute font paths
- some hand-written example pages predate the portability rules
- generated-page sync is a build bridge and can become a coordination hotspot
- current task validation is still reference-image driven, not semantic-layout driven

These are known and should be treated as migration work rather than blockers for the new task layer.

## LLM Refine Loop

After initial code generation and validation, `tools/refine-page.py` runs an iterative
visual refinement loop:

1. Read the current validation report (`diff_ratio`, `mean_abs_diff`) and the three-panel diff image
2. Send the current C source + diff image to the LLM with a vision prompt
3. The LLM responds with **search-replace blocks** (not the full file), reducing output tokens by 80-95%
4. Apply the search-replace patches to the source, rebuild, re-validate
5. Accept the change only if metrics improve; otherwise rollback
6. Repeat until validation passes or `max_iterations` is reached

Search-replace format:

```
<<<SEARCH
original lines (exact match)
===
replacement lines
>>>
```

Fallback: if the LLM outputs a full ```c code fence instead of search-replace blocks,
the system falls back to full-file replacement mode for compatibility.

Build errors during refinement are also handled via the same search-replace mechanism:
the compiler output is sent to the LLM, which returns targeted fixes.

## Web UI

`tools/webui.py` launches a Flask-based browser interface (`tools/web/`) that wraps the
CLI pipeline with drag-and-drop upload, real-time SSE log streaming, step progress
indicators, a stop button for cancelling running tasks, and result visualization.

The Web UI stores LLM settings (API key, model, base URL) in `workspace/.llm_settings.json`,
which is also read by `tools/llm_client.py` for CLI usage.
