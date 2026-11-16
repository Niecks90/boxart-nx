# boxart-nx v1.0.1 — Horizon OS Carousel & Polish Update

This update completely overhauls the frontend user experience with an authentic Nintendo Switch Home Menu (`qlaunch`) carousel interface, instant startup performance, and crucial bug fixes.

---

### 🌟 What's New & Fixed in v1.0.1

- **✨ Horizon OS (Home Menu) Carousel Interface:**
  - Redesigned from the ground up: replaced the two-pane child window layout with a genuine **horizontal game card carousel**.
  - Smooth 60 FPS D-pad and analog stick navigation (Left/Right to browse, L/R bumpers to jump 5 games).
  - Selected game card is prominently centered with a vibrant Horizon Cyan glowing highlight and custom status badges.
  - Dedicated controller action bar at the bottom with standard Switch button prompts:
    - `(A) Manage Cover`
    - `(X) SteamGridDB Search`
    - `(Y) Restore Official Icon`
    - `(-) Settings / API Key`
    - `(+) Exit`
  - Full touchscreen support: tap any card to select or open the management menu.

- **⚡ Instant Startup & Lazy Texture Loading (Fixes 30s black screen):**
  - Resolved the ~30-second black screen on launch caused by upfront synchronous CPU JPEG decoding of every installed title.
  - Replaced with ultra-fast metadata loading (< 0.2 seconds total).
  - Textures are now decoded lazily only when cards become visible in the carousel.
  - Added an immediate splash / loading screen on frame 1 with a real-time progress bar.

- **🎮 Fixed Button Inversion (A & B Swap):**
  - Corrected gamepad mapping in `imgui_impl_sdl2` and event loops for Nintendo Switch:
    - **Physical A** now properly triggers **Select / Confirm / Activate**.
    - **Physical B** now properly triggers **Cancel / Go Back / Close Modal**.

- **🛡️ Fixed Crash on Application Exit:**
  - Fixed a double-shutdown condition in the frontend cleanup routine that caused the application to crash or freeze when exiting with `(+)`.

---

### 🎮 Controls

| Button | Action |
|---|---|
| **D-Pad / Left Stick** | Scroll games carousel / navigate modal menus |
| **L / R (Bumpers)** | Jump ±5 games in the carousel |
| **A** | Manage cover / Confirm / Activate |
| **B** | Back / Close modal dialog |
| **X** | Quick search SteamGridDB for selected game |
| **Y** | Quick restore official NAND icon |
| **- (Minus)** | Open Settings (Configure SteamGridDB API key) |
| **+ (Plus)** | Safely exit application |

---

### 📦 Installation

1. Download **`boxart-nx.zip`** below.
2. Extract the archive directly to the root of your microSD card (`sdmc:/`).
3. Reboot your console into Atmosphère (or reload sysmodules via hbmenu/ovlloader).
4. Launch **`boxart-nx`** from the Homebrew Menu.

---

### 📎 Assets
- **`boxart-nx.zip`**: Full package ready to extract onto your SD card (contains both sysmodule and frontend).
- **`boxart-nx.nro`**: Standalone frontend executable (for quick update under `sdmc:/switch/boxart-nx/`).
