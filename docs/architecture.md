# LVGL Agent Architecture

## Goal

Convert the workspace from a hand-written LVGL page sandbox into a task-driven pipeline for:

1. ingesting user HTML and assets
2. generating LVGL page code (currently rule-based, LLM-driven planned)
3. validating the generated page in the SDL simulator
4. exporting portable `.c/.h` output for embedded projects

## Current State

The repository already has a useful simulator and validation loop:

- `m1_real_project/` builds and runs LVGL pages
- `tools/m1-page-flow.sh` performs build, screenshot, and diff validation
- `tools/m1-page-validate.py` produces `diff.png` and `report.json`

The main gaps are:

- no first-class HTML task input
- no board profile system
- no generated task workspace layout
- no portability checks for exported page code
- page registration and CMake source listing are still manual

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
   portability lint + simulator build
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
        │   └── manifest.json
        ├── artifacts/
        │   ├── current.png
        │   ├── full.png
        │   ├── diff.png
        │   └── report.json
        └── export/
            └── portable_bundle/
```

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

## Pipeline Commands

The production path should converge on these commands:

```bash
tools/m1-pipeline.sh init <task-dir>
tools/m1-pipeline.sh lint <task.json>
tools/m1-pipeline.sh run <task.json>
tools/m1-pipeline.sh export <task.json>
```

Current implementation status:

- `init`: implemented
- `lint`: implemented
- `run`: implemented as a compatibility bridge to the current `m1-page-flow.sh`
- `export`: implemented for generated output bundles

Pending:

- HTML-to-reference renderer
- LLM generation driver
- automatic page registry and CMake integration

## Execution Plan

### Phase 1: Task Foundation

Deliverables:

- `workspace/task.schema.json`
- `profiles/*.json`
- `tools/m1-task-init.py`
- `tools/m1-portability-lint.py`
- `tools/m1-pipeline.sh`
- `tools/m1-export-page.py`

Acceptance:

- a task directory can be created from one command
- generated page files can be linted against portability rules
- an existing legacy page can be validated through the compatibility bridge

### Phase 2: HTML Reference Rendering

Deliverables:

- `tools/m1-render-html-ref.py`
- task support for HTML viewport rendering
- local asset resolution rules

Acceptance:

- a task with `input/index.html` can produce `reference/reference.png`

### Phase 3: Code Generation

Current implementation: `rule_based_html_v1` — a rule-based HTML parser that extracts
block-level tags and maps them to LVGL widgets. LLM-driven generation is planned as Phase 3b.

Deliverables:

- `tools/m1-generate-page.py`
- prompt template and codegen rule pack
- generated output under `workspace/tasks/<task>/generated/`

Acceptance:

- the generator emits compilable `.c/.h`
- the output passes portability lint

### Phase 4: Auto Integration

Deliverables:

- generated page auto-registration
- generated source auto-discovery in CMake
- single-task simulator build path

Acceptance:

- no manual edit to `page_registry.c`
- no manual edit to `CMakeLists.txt`

### Phase 5: Export Hardening

Deliverables:

- portable bundle manifest
- `PORTING.md`
- asset copy and constraint summary

Acceptance:

- exported bundle can be copied into an embedded project with no simulator-only dependencies

## Immediate Risks

- current demo pages still contain absolute font paths
- current flow still assumes hand-maintained page registry
- current task validation is reference-image driven, not HTML driven

These are known and should be treated as migration work rather than blockers for the new task layer.
