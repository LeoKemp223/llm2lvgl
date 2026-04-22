# Tools

## Preferred Public Entry Points

- Pipeline entrypoint: [`/home/leo/work/open-git/llm2lvgl/tools/pipeline.sh`](/home/leo/work/open-git/llm2lvgl/tools/pipeline.sh)
- Runtime helper: [`/home/leo/work/open-git/llm2lvgl/tools/lvgl-runtime.sh`](/home/leo/work/open-git/llm2lvgl/tools/lvgl-runtime.sh)
- Page workflow helper: [`/home/leo/work/open-git/llm2lvgl/tools/page-flow.sh`](/home/leo/work/open-git/llm2lvgl/tools/page-flow.sh)
- Page validator: [`/home/leo/work/open-git/llm2lvgl/tools/page-validate.py`](/home/leo/work/open-git/llm2lvgl/tools/page-validate.py)
- Environment doctor: [`/home/leo/work/open-git/llm2lvgl/tools/doctor.py`](/home/leo/work/open-git/llm2lvgl/tools/doctor.py)
- Task workspace bootstrapper: [`/home/leo/work/open-git/llm2lvgl/tools/task-init.py`](/home/leo/work/open-git/llm2lvgl/tools/task-init.py)
- Image-to-HTML draft generator: [`/home/leo/work/open-git/llm2lvgl/tools/image-to-html.py`](/home/leo/work/open-git/llm2lvgl/tools/image-to-html.py)
- HTML-to-LVGL generator: [`/home/leo/work/open-git/llm2lvgl/tools/generate-page.py`](/home/leo/work/open-git/llm2lvgl/tools/generate-page.py)
- HTML reference renderer: [`/home/leo/work/open-git/llm2lvgl/tools/render-html-ref.py`](/home/leo/work/open-git/llm2lvgl/tools/render-html-ref.py)
- Generated-page bridge emitter: [`/home/leo/work/open-git/llm2lvgl/tools/sync-generated-pages.py`](/home/leo/work/open-git/llm2lvgl/tools/sync-generated-pages.py)
- Portability lint: [`/home/leo/work/open-git/llm2lvgl/tools/portability-lint.py`](/home/leo/work/open-git/llm2lvgl/tools/portability-lint.py)
- Workspace task runner: [`/home/leo/work/open-git/llm2lvgl/tools/task-run.py`](/home/leo/work/open-git/llm2lvgl/tools/task-run.py)
- Export helper: [`/home/leo/work/open-git/llm2lvgl/tools/export-page.py`](/home/leo/work/open-git/llm2lvgl/tools/export-page.py)

## Compatibility Notes

- Legacy `m1-*` script names are still present and continue to work.
- `tools/lvgl-runtime.sh` and `tools/doctor.py` automatically wire the repo-local `SDL2_image` fallback when it is present.
- For workspace task pages, `tools/lvgl-runtime.sh` supports `LVGL_PAGE=<page_id>` and auto-infers:
  - matching `workspace/tasks/*/task.json`
  - isolated build dir under `runtime_project/build/<task-id>/`
  - `target.viewport` as `LVGL_VIEWPORT_WIDTH/HEIGHT`
  - missing binary bootstrap via automatic `configure + build`

## Common Commands

- `tools/pipeline.sh doctor`
- `tools/pipeline.sh quickstart`
- `tools/pipeline.sh draft-html <task.json>`
- `tools/lvgl-runtime.sh rebuild`
- `tools/lvgl-runtime.sh run`
- `tools/lvgl-runtime.sh run-headless`
- `tools/lvgl-runtime.sh list-pages`
- `LVGL_PAGE=demo_page tools/lvgl-runtime.sh run`
- `LVGL_PAGE=stitch_smart_home_panel tools/lvgl-runtime.sh screenshot /tmp/stitch.png`
- `tools/page-flow.sh run token`
