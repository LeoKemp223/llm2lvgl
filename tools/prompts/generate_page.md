You are an expert LVGL UI developer. Your task is to generate a complete LVGL C source file that recreates a given HTML page as closely as possible.

## Codegen Rules

{codegen_rules}

## Output Contract

You MUST produce a single C source file that:

1. Includes `"<page_id>_page.h"` and `"ui_font.h"`
2. Defines a static `g_content_root` pointer
3. Exports `<page_id>_page_create(void)` — creates and returns the screen
4. Exports `<page_id>_page_get_content_root(void)` — returns the content root

## Font API

Use `ui_font_get(size)` to obtain fonts. Example: `ui_font_get(24)` for 24px text.

## Example

Given page_id `my_dashboard`, here is a valid output:

```c
#include "my_dashboard_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

lv_obj_t * my_dashboard_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_size(screen, 1280, 800);

    lv_obj_t * content = lv_obj_create(screen);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(content, 32, 0);
    lv_obj_set_style_pad_row(content, 14, 0);
    lv_obj_set_style_radius(content, 24, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_size(content, 1184, LV_SIZE_CONTENT);
    lv_obj_center(content);

    lv_obj_t * title = lv_label_create(content);
    lv_label_set_text(title, "Dashboard");
    lv_obj_set_style_text_font(title, ui_font_get(32), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0f172a), 0);

    g_content_root = content;
    return screen;
}

lv_obj_t * my_dashboard_page_get_content_root(void)
{
    return g_content_root;
}
```

## Important Notes

- Use LVGL flex layout (`LV_LAYOUT_FLEX`) for arranging elements when the HTML uses flexbox
- Use `lv_obj_set_pos()` for absolute positioning when needed
- Map HTML colors to `lv_color_hex()` calls
- Map font sizes to `ui_font_get()` calls
- Keep all code in a single file, no external dependencies beyond lvgl.h and ui_font.h
- Output ONLY the C code inside a ```c code fence

## Native Widget Policy (CRITICAL)

ALWAYS use LVGL native widgets for interactive UI elements. NEVER simulate buttons, switches, sliders, etc. with `lv_obj_create()` + manual styling.

Mapping:

| HTML | LVGL |
|---|---|
| `<button>` | `lv_button_create(parent)` — style with `LV_PART_MAIN` |
| checkbox | `lv_checkbox_create(parent)` — use `lv_checkbox_set_text()` |
| toggle switch | `lv_switch_create(parent)` — checked state via `lv_obj_add_state(sw, LV_STATE_CHECKED)` |
| slider / range | `lv_slider_create(parent)` — style knob via `LV_PART_KNOB`, track via `LV_PART_INDICATOR` |
| progress bar | `lv_bar_create(parent)` — set value with `lv_bar_set_value()` |
| `<select>` / dropdown | `lv_dropdown_create(parent)` — set options with `lv_dropdown_set_options()` |
| text input | `lv_textarea_create(parent)` |
| circular progress | `lv_arc_create(parent)` |

Example — a styled button:
```c
lv_obj_t * btn = lv_button_create(parent);
lv_obj_set_size(btn, 120, 48);
lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50), 0);
lv_obj_set_style_radius(btn, 8, 0);
lv_obj_t * label = lv_label_create(btn);
lv_label_set_text(label, "Submit");
lv_obj_center(label);
```

Example — a styled switch:
```c
lv_obj_t * sw = lv_switch_create(parent);
lv_obj_set_size(sw, 50, 26);
lv_obj_add_state(sw, LV_STATE_CHECKED);
lv_obj_set_style_bg_color(sw, lv_color_hex(0x4CAF50), LV_PART_INDICATOR | LV_STATE_CHECKED);
```

Example — a styled slider:
```c
lv_obj_t * slider = lv_slider_create(parent);
lv_obj_set_width(slider, 200);
lv_slider_set_range(slider, 0, 100);
lv_slider_set_value(slider, 70, LV_ANIM_OFF);
lv_obj_set_style_bg_color(slider, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
lv_obj_set_style_bg_color(slider, lv_color_hex(0x2196F3), LV_PART_KNOB);
```

Only use `lv_obj_create()` for non-interactive containers: panels, cards, layout wrappers, dividers.
