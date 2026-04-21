# Board Profiles

Board profiles define the hardware and portability constraints that the generator must obey.

## Why Profiles Exist

The simulator is permissive. Embedded targets are not.

Without a profile, an LLM will tend to generate code that:

- assumes desktop font availability
- relies on filesystem-backed assets
- ignores memory and color-depth limits
- accidentally uses simulator-only code paths

## Required Fields

Each profile should define:

- `screen.width`
- `screen.height`
- `screen.color_depth`
- `screen.dpi`
- `fonts.allow_freetype`
- `fonts.builtin_fonts`
- `assets.allow_filesystem`
- `constraints.allow_sdl_only_api`

## Included Profiles

### `profiles/sim_1280x800.json`

Use this for desktop simulator-first iteration.

Traits:

- large viewport
- permissive asset policy
- still forbids SDL in page code

### `profiles/stm32_800x480.json`

Use this for common STM32 HMI-class targets.

Traits:

- medium display
- RGB565-oriented deployment
- no FreeType
- no simulator-only APIs

### `profiles/esp32_480x320.json`

Use this for smaller ESP32-based panels.

Traits:

- tighter viewport
- no FreeType
- no filesystem by default

## Rule Of Thumb

- simulator profile is for iteration
- MCU profiles are for export

If a task is meant to ship to firmware, it should be linted against the MCU profile before export.
