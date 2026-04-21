# Design System Documentation: The Ethereal Command

## 1. Overview & Creative North Star

### The Creative North Star: "Architectural Lucidity"
This design system moves away from the cluttered, "gadget-heavy" aesthetic typical of smart home interfaces. Instead, it adopts **Architectural Lucidity**. This North Star prioritizes the feeling of physical space, breathability, and premium precision. We treat the interface not as a software dashboard, but as a digital extension of a well-lit, modern home.

To break the "template" look, we utilize **intentional asymmetry** and **tonal depth**. Rather than placing everything in a rigid, boxed-in grid, we allow elements to breathe with generous white space and use varying typographic scales to create a clear, editorial hierarchy. The interface should feel like a high-end interior design magazine—functional, yet undeniably sophisticated.

---

## 2. Colors

The color palette is anchored by a pristine white base, energized by a "Vibrant Electric Blue" primary accent. The goal is high-contrast professionalism.

### The "No-Line" Rule
**Borders are prohibited for sectioning.** To define boundaries, designers must use background color shifts or subtle tonal transitions. A 1px solid line is a "design failure" in this system. Use the `surface-container` tiers to create separation.
- *Example:* A main control panel (`surface-container-low`) sits directly on the `background` without a stroke.

### Surface Hierarchy & Nesting
Treat the UI as physical layers of fine paper or frosted glass.
- **Surface (Base):** `#f8f9fa` - The floor of the interface.
- **Surface-Container-Lowest:** `#ffffff` - Used for the most "elevated" content like active control cards.
- **Surface-Container-Highest:** `#e1e3e4` - Used for recessed areas, like a navigation rail or a background "tray" for grouped devices.

### The "Glass & Gradient" Rule
For floating controllers (e.g., a climate popover), use **Glassmorphism**. Combine `surface` colors at 70-80% opacity with a `backdrop-blur` of 20px-40px. 

### Signature Textures
To add "soul," use a subtle linear gradient for primary CTAs:
- **Primary Gradient:** `primary` (`#004bca`) to `primary_container` (`#0061ff`) at a 135-degree angle. This prevents the blue from feeling "flat" or "default."

---

## 3. Typography

The typography strategy relies on the interplay between the geometric precision of **Manrope** and the functional clarity of **Inter**.

- **The Power of Display:** Use `display-lg` (Manrope) for ambient information like "Current Home Temp" or "Good Morning." The high-contrast scale between a massive display weight and a small `label-md` creates an "Editorial" feel.
- **The Workhorse:** `body-md` (Inter) is used for all functional descriptions. Its neutral character allows the vibrant blue accents to command attention without competing for visual interest.
- **Authority:** All `headline` and `display` tokens must use Manrope to maintain a "Modern Architectural" signature.

---

## 4. Elevation & Depth

We convey hierarchy through **Tonal Layering** rather than traditional structural shadows.

- **The Layering Principle:** Depth is achieved by stacking. Place a `surface-container-lowest` card on a `surface-container-low` section. The slight shift in hex code creates a soft, natural lift that mimics natural light hitting different planes.
- **Ambient Shadows:** For elements that truly "float" (like a light-dimmer modal), use an ultra-diffused shadow: `0px 24px 48px rgba(25, 28, 29, 0.06)`. Note the use of a low-opacity `on-surface` tint rather than pure black.
- **The "Ghost Border" Fallback:** If accessibility requires a container edge, use a "Ghost Border": `outline-variant` (`#c2c6d9`) at **15% opacity**.
- **Glassmorphism:** Use semi-transparent `surface` tokens to allow background colors to bleed through, ensuring the interface feels integrated into the home’s "environment" rather than pasted on top.

---

## 5. Components

### Buttons
- **Primary:** High-contrast `primary` background with `on_primary` text. Use `rounded-md` (0.75rem) for a modern, approachable feel.
- **Tertiary (Ghost):** No background or border. Use `primary` text weight and `label-md` for navigation actions.

### Cards & Lists (The "No-Divider" Mandate)
- **Cards:** Forbid divider lines. Separate content using `title-md` for headers and `body-sm` for metadata, separated by exactly `1rem` (16px) of vertical white space.
- **Lists:** Use a `surface-container-low` background on hover to indicate interactivity rather than a separator line.

### Input Fields & Controls
- **Smart Sliders:** Use `primary` for the active track and `surface-variant` for the inactive track. No borders. The "thumb" should be a `surface-container-lowest` circle with a subtle `ambient shadow`.
- **Checkboxes/Radios:** Use `primary` for the selected state. The unchecked state should be a subtle "Ghost Border" circle/square to maintain the light, airy aesthetic.

### Specialized Smart Home Components
- **Device Status Chips:** Use `secondary_container` for "On" states and `surface-variant` for "Off" states. Typography should be `label-sm` in all caps to denote a "technical" status.

---

## 6. Do's and Don'ts

### Do
- **Do** embrace white space. If you think there is enough space, add 8px more.
- **Do** use `primary` (`#004bca`) sparingly. It is a "signal" color; if everything is blue, nothing is important.
- **Do** use Manrope for all numbers. It has a modern, high-end tabular feel that suits smart home metrics.

### Don't
- **Don't** use 1px solid borders to separate sections. Use tonal shifts.
- **Don't** use pure black `#000000` for text. Use `on_surface` (`#191c1d`) to maintain the premium, soft-light feel.
- **Don't** use "Standard" drop shadows. If it looks like a default CSS shadow, it’s too heavy. Keep them ambient and barely visible.