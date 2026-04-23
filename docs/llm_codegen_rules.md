# LLM Codegen Rules

Target: LVGL 9.x (v9.6). Use ONLY the LVGL 9.x API — do NOT use deprecated LVGL 8.x patterns.

## Key LVGL 9.x API Changes (vs 8.x)

- `lv_obj_clear_flag()` → renamed to `lv_obj_remove_flag()`
- `lv_obj_set_style_pad_gap()` → removed; use `lv_obj_set_style_pad_row()` / `lv_obj_set_style_pad_column()`
- `lv_btn_create()` → renamed to `lv_button_create()`
- `lv_img_create()` → renamed to `lv_image_create()`
- `lv_img_set_src()` → renamed to `lv_image_set_src()`
- `lv_list_add_btn()` → renamed to `lv_list_add_button()`
- `lv_obj_set_style_text_align()` → use `lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0)`
- `lv_disp_*` → renamed to `lv_display_*`
- `lv_indev_*` → renamed to `lv_indev_*` (mostly same, but check signatures)
- `LV_ALIGN_OUT_*` → removed; use `lv_obj_align_to()` with `LV_ALIGN_OUT_*` replaced by manual positioning

These rules define what generated LVGL page code is allowed to do.

## Output Contract

Each task must generate:

- `generated/<page_id>.c`
- `generated/<page_id>.h`

The header must export:

```c
lv_obj_t * <page_id>_page_create(void);
lv_obj_t * <page_id>_page_get_content_root(void);
```

## Allowed Scope

Generated page files should contain:

- LVGL object creation and layout
- local styles needed by that page
- references to task-local assets
- no simulator bootstrap logic

## Forbidden Patterns

Do not generate:

- `lv_sdl_*` API calls
- `SDL_*` API calls
- `main()`
- `setenv`, `getenv`
- absolute filesystem paths such as `/usr/share/...` or `/home/...`
- hardcoded desktop font paths
- direct assumptions about `1280x800` unless the active profile says so

## LVGL API Pitfalls

Common mistakes that cause compilation errors — avoid these:

- `LV_OPA_*` only has multiples of 10: `LV_OPA_0`, `LV_OPA_10`, `LV_OPA_20`, ..., `LV_OPA_100`, plus `LV_OPA_TRANSP`, `LV_OPA_COVER`. Values like `LV_OPA_8`, `LV_OPA_15`, `LV_OPA_25` do NOT exist. Use the nearest valid value or a raw integer (0–255).
- `lv_color_hex()` takes a `uint32_t`, not a string. Correct: `lv_color_hex(0xFF0000)`.
- `lv_color_t` has NO `.full` member in LVGL 9.x. To compare colors, use `lv_color_eq(a, b)`. To check if a color is black, use `lv_color_eq(c, lv_color_black())`. Never write `color.full`.
- `lv_obj_set_style_text_font()` requires a `const lv_font_t *`. Use `ui_font_get(size)` — never reference `lv_font_montserrat_*` directly.
- `LV_SIZE_CONTENT` is valid for width/height but not for position APIs.
- `lv_obj_set_style_bg_opa()` expects `lv_opa_t` (0–255 or `LV_OPA_*` macro).
- `lv_label_set_text()` copies the string — no need to keep the buffer alive, but never pass NULL.

## Widget Rules

Always prefer LVGL native widgets over manually styled `lv_obj_create()`. Native widgets provide correct default behavior, accessibility, and state handling (pressed, focused, disabled).

| HTML Element | LVGL Widget | Create Function |
|---|---|---|
| `<button>` | Button | `lv_button_create(parent)` |
| `<input type="checkbox">` | Checkbox | `lv_checkbox_create(parent)` |
| `<input type="range">`, slider | Slider | `lv_slider_create(parent)` |
| toggle switch | Switch | `lv_switch_create(parent)` |
| `<select>` | Dropdown | `lv_dropdown_create(parent)` |
| progress bar | Bar | `lv_bar_create(parent)` |
| `<textarea>`, `<input type="text">` | Textarea | `lv_textarea_create(parent)` |
| circular gauge / progress | Arc | `lv_arc_create(parent)` |
| scroll picker | Roller | `lv_roller_create(parent)` |

Do NOT simulate these controls with `lv_obj_create()` + manual styling. Use the native widget and customize its appearance via style properties and parts (`LV_PART_MAIN`, `LV_PART_INDICATOR`, `LV_PART_KNOB`, etc.).

Only use `lv_obj_create()` for containers, panels, cards, and layout wrappers that have no interactive behavior.

## Font Rules

- Prefer built-in LVGL fonts from the active board profile
- Only use FreeType when the board profile explicitly allows it
- Never reference host OS font paths in generated page code

## Asset Rules

- Asset paths must be relative to the task or export bundle
- Do not assume host filesystem roots
- If the profile forbids filesystem access, generated code must not use runtime file loading

## Layout Rules

- Derive layout from the active display size when possible
- Avoid fixed coordinates unless visually required
- Prefer reusable helper functions for repeated UI patterns
- Keep page-specific helpers inside the generated page file unless promoted to a shared component library

## Portability Rules

Generated files should be usable in a firmware project after minimal glue integration.

That means:

- no simulator-only includes
- no shell command assumptions
- no Linux-only APIs
- no desktop windowing dependencies

## Validation Rule

Passing the simulator diff gate is necessary but not sufficient.

Generated output must satisfy both:

- visual validation
- portability lint

## Migration Note

Existing example pages in `runtime_project/src/` predate these rules and may violate them. Treat them as visual examples, not as final codegen references.
