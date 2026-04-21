# M0 Sample Page Shortlist

## Status
- Current workspace does not contain a real product UI or LVGL project.
- The pages below are a provisional shortlist for M0 validation.
- They should be replaced by real business pages once the target project is attached.

## Selection Rules
- Cover basic layout, text, button, image, and list/card composition
- Include at least one reusable component
- Avoid highly custom drawing or animation in M0
- Prefer pages that can be validated in both simulator and on device

## Proposed Pages

### 1. Home Dashboard
- Purpose: validate base layout, typography, icon/image usage, and card composition
- Required elements:
  - top title
  - summary cards
  - one primary action button
  - one status area
- Why chosen:
  - representative page shell
  - naturally exercises reusable card and button components

### 2. Device List
- Purpose: validate repeated item layout, scrolling, icon usage, and list state display
- Required elements:
  - header area
  - scrollable list
  - list item with title, subtitle, status badge, optional icon
- Why chosen:
  - common business page
  - useful for testing reusable list item components

### 3. Settings Panel
- Purpose: validate forms, switches, rows, spacing, and simple interaction states
- Required elements:
  - section titles
  - setting rows
  - switch or button controls
  - optional footer action
- Why chosen:
  - covers typical control widgets
  - keeps logic simple enough for M0

## Required Assets For Replacement
- target device resolution
- target font set
- target icon/image assets
- screenshots or design references
- interaction notes for each selected page

## Acceptance Use
- At least two of the three selected pages must pass the Phase 0 Gate once a real project is connected.
