# LLM Codegen Rules

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

Existing example pages in `m1_real_project/src/` predate these rules and may violate them. Treat them as visual examples, not as final codegen references.
