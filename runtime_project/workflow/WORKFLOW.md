# LVGL Page Workflow

This workflow packages page development into a repo-local loop so any model can reuse it.

## Inputs
- Task definitions live in `workflow/tasks/*.json`
- The task schema lives in `workflow/page-task.schema.json`
- Reference screenshots should be stored inside `references/`

## Required Loop
1. Read the task JSON.
2. Implement or adjust the page code under `src/`.
3. Register the page in `src/page_registry.c`.
4. Run `tools/page-flow.sh loop <task-id> <iteration>`.
5. Inspect `artifacts/<page-id>/report.json` and `artifacts/<page-id>/diff.png`.
6. If completion is not met, continue editing and rerun the workflow.
7. Stop when completion is met or the task reaches its configured failure policy.

## Commands
```bash
tools/page-flow.sh build token
tools/page-flow.sh screenshot token
tools/page-flow.sh validate token
tools/page-flow.sh run token
tools/page-flow.sh loop token 1
```

## Task Format
- `page_id`: artifact folder id
- `page_name`: human-readable label
- `entry_page`: page registry id used by `LVGL_PAGE`
- `reference_image`: project-relative path to the visual reference
- `artifacts_dir`: project-relative artifact output folder
- `viewport`: expected simulator size
- `validation`: thresholds for the coarse visual gate
- `completion`: completion criteria for the task
- `failure_policy`: hard stop conditions for automated iteration

## Outputs
Each run writes the following into `artifacts/<page-id>/`:
- `current.png`: viewport screenshot
- `full.png`: full-content screenshot
- `diff.png`: side-by-side visual diff
- `report.json`: structured validation report

## Completion and Failure Policy

- A task is considered completed when `report.json` says `completion_met=true`.
- The default completion gate is `completion.require_validation_pass=true`.
- A task is considered failed for the current automation loop when any configured hard stop is hit.

Current hard-stop sources:
- current iteration exceeds `failure_policy.max_iterations`
- build fails and `failure_policy.stop_on_build_error=true`
- reference image is missing and `failure_policy.stop_on_missing_reference=true`
- validator crashes or exits unexpectedly and `failure_policy.stop_on_validation_script_error=true`

Recommended iteration model:
1. Start with `tools/page-flow.sh loop <task-id> 1`
2. After each code change, increase the iteration number
3. Stop when `completion_met=true`
4. Stop with failure when the iteration count exceeds `max_iterations`

## Design Rules
- Keep tasks and artifacts inside the repo so model switches do not break the flow.
- Use stable page ids through `LVGL_PAGE` or `--page`.
- Prefer project-local references over `/tmp` files.
- Treat the validator as a fail-fast tool. Human review is still needed for final sign-off.
