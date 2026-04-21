# LVGL Agent Architecture

## Goal

Keep the repository on a task-driven path for:

1. ingesting user HTML and assets
2. generating LVGL page code
3. validating the generated page in the SDL simulator
4. exporting portable `.c/.h` output for embedded projects

## Current State

The repository is no longer just a hand-written page sandbox.

It now has a working task pipeline on top of the original M1 simulator loop:

- `workspace/tasks/<task_id>/task.json` is the first-class task entry
- `tools/m1-task-init.py` scaffolds new tasks
- `tools/m1-generate-page.py` generates LVGL page code from HTML input
- `tools/m1-render-html-ref.py` renders HTML reference screenshots
- `tools/m1-portability-lint.py` enforces portability constraints
- `tools/m1-task-run.py` bridges generated tasks into the current simulator build
- `tools/m1-export-page.py` exports generated output as a portable bundle
- `tools/m1-pipeline.sh` is the unified CLI entrypoint

The legacy M1 layer is still present and still important:

- `m1_real_project/` remains the executable LVGL runtime and screenshot target
- `tools/lvgl-m1-real.sh` still handles configure/build/run/screenshot operations
- `tools/m1-page-validate.py` still produces `diff.png` and `report.json`
- `tools/m1-page-flow.sh` remains the compatibility path for legacy page tasks

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
tools/m1-pipeline.sh doctor
tools/m1-pipeline.sh init <task-dir>
tools/m1-pipeline.sh generate <task.json>
tools/m1-pipeline.sh render-ref <task.json>
tools/m1-pipeline.sh sync
tools/m1-pipeline.sh lint <task.json>
tools/m1-pipeline.sh run <task.json>
tools/m1-pipeline.sh export <task.json>
```

Current implementation status:

- `doctor`: implemented
- `init`: implemented
- `generate`: implemented
- `render-ref`: implemented
- `sync`: implemented
- `lint`: implemented
- `run`: implemented through `m1-task-run.py`, which still reuses the legacy simulator and diff validation path
- `export`: implemented for generated output bundles

Pending:

- LLM-driven generation path beyond the current rule-based generator
- stronger end-to-end iteration loop for fixing generation failures automatically
- hardening around sync/build race conditions and duplicate page handling

## Runtime Integration

Generated tasks do not compile in isolation.

The current runtime path is:

1. generate task-local `.c/.h`
2. sync all generated pages into `m1_real_project/build/generated_page_registry.*`
3. let `m1_real_project/CMakeLists.txt` include the generated source list
4. build `lvgl_m1_demo`
5. run screenshots against the selected `page_id`

This means automatic generated-page registration and CMake source discovery are already present,
but they are implemented as a bridge into the existing M1 runtime rather than as a new standalone runner.

## Implementation Status

### Implemented

- task workspace schema and scaffolding
- board-profile based generation constraints
- HTML reference rendering for task validation
- rule-based HTML-to-LVGL generation
- portability lint before simulator validation
- generated-page auto-sync into the M1 build
- portable bundle export

### In Progress / Remaining Gaps

- the default generator is still rule-based and intentionally limited
- validation is still screenshot-driven, even when the reference comes from HTML
- the runtime still depends on the M1 executable bridge instead of a dedicated task-native runner
- exported bundles are portable page artifacts, not a full firmware integration package
- multi-task sync robustness still needs hardening

## Immediate Risks

- current demo pages still contain absolute font paths
- some hand-written example pages predate the portability rules
- generated-page sync is a build bridge and can become a coordination hotspot
- current task validation is still reference-image driven, not semantic-layout driven

These are known and should be treated as migration work rather than blockers for the new task layer.
