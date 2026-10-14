# C4D_PieMenu - Radial Marking Menu for Cinema 4D

![](attachments/C4D_PieMenu.gif)

**C4D_PieMenu** is a fast, customizable radial marking menu plugin for Maxon Cinema 4D on **Windows** (Cinema 4D 2025 & 2026). It brings rapid gesture-based command execution (**Press & Flick**) directly under your cursor with an interactive **Drag & Drop** configuration editor.

> [!NOTE]
> **Windows Only**: Built exclusively for Windows 10 / 11 64-bit using low-level Win32 input hooks and hardware-accelerated overlay rendering for zero latency. macOS is not supported.

---

## Features

- **⚡ Zero-Latency HUD (Press & Flick)**: Instantaneous transparent overlay. Hold your hotkey, flick towards a slot, and release to execute. Supports click confirmation, center hub commands, and smart typing detection that ignores hotkeys while renaming or editing values.
- **🎯 Context-Aware Profiles**: Automatically activates relevant menus based on the current mode (**Global**, **Polygons**, **Edges**, **Points**, **Objects**, **UV**), with per-menu hotkeys and intelligent conflict resolution.
- **📂 Nested Submenus**: Multi-level menus with zero-delay hover expansion, breadcrumb path display, and fast navigation (`Backspace`, RMB, or Center Hub to go back).
- **⌨️ Modifier Actions & Native Behaviors**: 
  - Native Cinema 4D modifiers: **Alt** (parent under new generator), **Shift** (child deformer or open tool options), **Ctrl** (spawn at world origin).
  - Custom command overrides for `Shift`, `Ctrl`, and `Alt` on any slot, with live HUD badge feedback.
- **💾 Tool Presets & Snapshots**: Capture active tool or generator settings as instant **Snapshots** or apply saved asset presets on execution for main and modifier actions.
- **🎨 Visual Profile Editor (`Extensions -> Pie Menu Configuration`)**:
  - **Drag & Drop**: Drag commands and submenus directly onto radial slots, modifier rows, or swap slots on canvas (`Ctrl+Drag` to duplicate).
  - **Command Library**: Live search through all Cinema 4D commands by name, numeric ID, or keyboard shortcut.
  - **Per-Menu Customization**: Set slot count (2–24), background color (`BG Color`), display style (*Icon & Label*, *Icon Only*, *Label Only*), icon size (12–64px), independent inner/outer radii, opacity, and pin/stay-open behavior.
  - **Backup & Share**: Export and import menus and settings as human-readable `.json` files.

---

## Controls & Gestures

| Action / Gesture | Description |
|---|---|
| **Hold Hotkey + Flick + Release** | Gesture mode: executes the hovered command immediately (**Press & Flick**) |
| **Tap Hotkey / Click Mode** | Opens or pins menu open under cursor for browsing and LMB click selection |
| **Hover Submenu Slot** | Instantly expands submenu; flick through multiple menu levels in one gesture |
| **Center Hub (`Back`) / `Backspace` / RMB** | Returns to the parent menu (or cancels at root level) |
| **`Esc` / RMB Click** | Cancels and dismisses menu without executing |
| **Hold Alt / Shift / Ctrl** | Triggers native parenting/options behaviors or custom modifier actions |

---

## Installation

1. Download the latest release archive (`C4D_PieMenu_v<version>.zip`) from [Releases](https://github.com/SPluzh/C4D_PieMenu/releases).
2. Copy the `C4D_PieMenu` folder for your version (`2025` or `2026`) into your Cinema 4D plugins directory:
   - **User Plugins (Recommended)**:
     ```text
     %APPDATA%\Maxon\Maxon Cinema 4D <Version>_<Hash>\plugins\C4D_PieMenu
     ```
   - **Application Plugins**:
     ```text
     <Cinema 4D Installation Directory>\plugins\C4D_PieMenu
     ```
3. Restart Cinema 4D.
4. Open the editor via **Extensions** -> **Pie Menu Configuration**.
