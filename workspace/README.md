# Workspace Tasks

This directory hosts task-oriented inputs and outputs for the HTML-to-LVGL pipeline.

Each task lives under `workspace/tasks/<task_id>/` and contains:

- `input/`: user-provided HTML, task notes, and task-local assets
- `reference/`: rendered or supplied visual reference
- `generated/`: LLM-generated LVGL page code
- `artifacts/`: simulator screenshots and validation output
- `export/`: portable delivery bundle

Create a new task with:

```bash
tools/m1-pipeline.sh init workspace/tasks/<task_id>
```
