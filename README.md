# C4D_PieMenu — Radial Marking Menu & Profile Editor for Cinema 4D

**C4D_PieMenu** is a fast, highly customizable radial marking menu (pie menu) plugin for Maxon Cinema 4D (supporting Cinema 4D 2025 and 2026). It brings rapid gesture-based tool and command execution directly under your mouse cursor (**Press & Flick**), alongside an interactive graphical profile editor with full **Drag & Drop** command assignment.

---

## 🌟 Key Features

### 1. Zero-Latency Viewport HUD & Marking Menu
- **Hardware-Accelerated Overlay**: Uses a dedicated transparent double-buffered Win32 overlay with GDI+ anti-aliasing, ensuring smooth, instantaneous opening with zero viewport stutter.
- **Press & Flick Gesture**: Press and hold your designated menu hotkey, flick the mouse toward the desired slice, and release the key — the command fires instantly without requiring extra clicks.
- **Click Confirmation & Escape**: Supports traditional left-click (LMB) confirmation or right-click (RMB) / `Esc` to dismiss without executing.
- **Smart Input Detection**: Automatically pauses menu activation when typing in text fields, numeric attributes, console inputs, or renaming objects in Cinema 4D to prevent accidental triggers.
- **Center Hub Slot**: The central deadzone doubles as a dedicated hub slot (e.g. for Undo, Reset, or Deselect).
- **Modern Annular Aesthetics**:
  - Smooth annular arc segments with hover popout expansion.
  - Directional laser pointer guideline connecting the hub to the cursor.
  - Prominent glowing accent highlights for the hovered slot.
  - Deadzone crosshair reticle.
  - Optional profile title badge above the menu.
  - **Inactive Slots as Gaps**: Inactive or unassigned slots can appear as clean gaps, keeping your most-used commands clearly separated and easy to hit.

### 2. Unlimited Context-Aware Profiles
- **Create Unlimited Menus**: Add as many custom pie menus as you need using the **`+ New Menu`**, **`Duplicate`**, and **`- Delete`** buttons.
- **Context-Sensitive Triggering**:
  - **Global**: Accessible across all modes and tools.
  - **Polygons Mode**: Active when working in polygon component mode.
  - **Edges Mode**: Active when working in edge component mode.
  - **Points Mode**: Active when working in point component mode.
  - **Objects / Model Mode**: Active in standard object or model mode.
  - **UV Edit Mode**: Active within UV editing workflows.
- **Individual Per-Menu Hotkeys**: Assign any key combination to each menu:
  - Modifiers: `Ctrl`, `Shift`, `Alt`.
  - Keys: Any standard key (e.g., `V`, `Tab`, `C`, `Space`, function keys, numbers, etc.) with interactive key recording via the **`Press Key...`** button.
- **Smart Conflict Resolution**: If multiple menus share the same hotkey, the plugin dynamically activates the menu that best matches your current document editing context, seamlessly falling back to Global if no mode-specific menu matches.

### 3. Visual Configuration Dialog (Extensions -> Pie Menu Configuration)
- **Built-in Command Palette & Search**: Live search through all Cinema 4D built-in tools, commands, and third-party plugins by name or command ID.
- **Drag & Drop Assignment**: Drag commands directly from the command browser onto radial slots or the slot inspector. Double-clicking a command assigns it to the active slot.
- **Interactive Radial Canvas**: Real-time visual canvas displaying 4-way or 8-way slot layouts, icons, labels, accent colors, and gap distribution.
- **Detailed Slot Inspector**:
  - Custom Command ID or name.
  - Custom Label overrides.
  - Custom Accent Color picker per slot.
  - Enable / Disable slot toggle.
  - One-click *Clear Slot*.
- **Display & Sizing Controls**:
  - Adjustable Outer Radius and Deadzone Radius.
  - 4 or 8 radial slots.
  - Toggle *Show Menu Title* badge.
- **Import & Export Settings**:
  - **Export Settings...**: Export either all menu profiles and global radius/deadzone settings or only the active menu profile to readable `.json` files.
  - **Import Settings...**: Seamlessly load configuration from `.json` or binary `.dat` files with interactive prompt to either Replace all menus or Append/Merge new menus into your current configuration.
- **Auto-Save & Defaults**:
  - Automatically saves all profile changes when closing the dialog or modifying settings.
  - Stored in Cinema 4D user preferences (`piemenu_profiles.dat`).
  - *Reset to Defaults* button to restore default presets at any time.

---

## ⌨️ Controls & Gestures

| Action / Gesture | Description |
|---|---|
| **Hold Configured Hotkey** | Instantly opens the radial pie menu under the cursor |
| **Move Mouse** | Highlights the targeted slot with popout and laser pointer |
| **Release Hotkey** | Executes the active slot command immediately (**Press & Flick**) |
| **LMB Click** | Executes the highlighted slot command |
| **RMB Click or `Esc`** | Cancels and closes the menu without executing |

---

## 📦 Installation

1. Download or extract the release archive (`C4D_PieMenu_v<version>.zip`).
2. Copy the `C4D_PieMenu` folder corresponding to your Cinema 4D version (`2025` or `2026`) into your Cinema 4D plugins directory:
   - **User Plugins folder (Recommended)**:
     ```text
     %APPDATA%\Maxon\Maxon Cinema 4D <Version>_<Hash>\plugins\C4D_PieMenu
     ```
   - **Application Plugins folder**:
     ```text
     <Cinema 4D Installation Directory>\plugins\C4D_PieMenu
     ```
3. Launch or restart Cinema 4D.
4. Access the configurator via the main menu: **Extensions** -> **Pie Menu Configuration**.
