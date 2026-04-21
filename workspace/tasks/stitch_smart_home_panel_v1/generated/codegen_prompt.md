# Handwritten Implementation For Stitch Smart Home Panel

## Task
- task_id: `stitch_smart_home_panel_v1`
- page_id: `stitch_smart_home_panel`
- page_name: `Stitch Smart Home Panel`

## Status
- This task is now treated as a handwritten LVGL page.
- `input/index.html` remains as source reference only.
- `tools/m1-generate-page.py` skips regeneration because `input.source_type = reference_only`.

## Intent
- Preserve the visual structure of the original dashboard:
  - top app bar
  - thermostat hero card
  - lights and security cards
  - window controls row
  - quick scenes pills
  - bottom navigation
