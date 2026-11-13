# boxart-nx

A complete, non-destructive game icon manager for the Nintendo Switch Home Menu (`qlaunch`).

`boxart-nx` intercepts Horizon OS `ns` system service IPC requests that provide application metadata and icons (`GetApplicationControlData`), seamlessly substituting custom box art on the fly without ever modifying NAND system saves or touching Horizon OS partitions.

---

## Features

- **Non-Destructive & Safe:** Never writes to NAND or official `ns` system save files. If an override icon is deleted or missing, Horizon automatically falls back to official game icons without any risk of system freeze or corruption.
- **Ultra-Lightweight Atmosphere MitM Sysmodule:**
  - Written in C++20 with `libstratosphere`.
  - Static memory heap strictly capped at **128 KB** (well within Atmosphere's $\le 2\text{ MB}$ sysmodule constraint).
  - Zero dynamic heap allocations in critical IPC dispatch loops.
  - Intercepts `ns:am2` and `ns:ro` (commands 0, 5, 6, 18, 19, 20, 21, 22), supporting all Horizon OS firmware versions from FW 1.0.0 through 21.0.0+.
- **Fluid Controller-Driven Frontend (.nro):**
  - Powered by **SDL2 + Dear ImGui** (using native hardware-accelerated `SDL_Renderer2`).
  - Full Joy-Con and Pro Controller gamepad navigation (`ImGuiConfigFlags_NavEnableGamepad`).
  - Lists all installed titles with current icon previews, publisher info, and custom status badges.
- **Strict JPEG Normalization Pipeline:**
  - Decodes PNG, JPEG, BMP (`stb_image`) and WebP (`libwebp`).
  - Center-crops square aspect ratio and resizes to standard **256×256 pixels** (`stb_image_resize2`).
  - Encodes strictly compliant **Baseline JPEG 4:2:0** without unnecessary EXIF/ICC metadata using `libjpeg-turbo`.
  - Dynamic size enforcement loop: compresses starting at quality 90 and steps down until the file strictly fits within $131\,072$ bytes ($0\text{x}20000$ / 128 KB).
- **Direct SteamGridDB Integration:**
  - In-app game search using the SteamGridDB REST API (`libcurl` + `nlohmann/json`).
  - Browse and download official square grid covers ($600\times 600$ and $1024\times 1024$) with image thumbnails.
  - Configurable API key stored in `sdmc:/config/boxart-nx/config.ini` or set directly from the UI.
- **Local Import Folder:**
  - Drop your custom `.png`, `.jpg`, or `.webp` files into `sdmc:/config/boxart-nx/import/`.
  - Automatic title matching by TitleID filename (e.g., `0100000000010000.png`) or visual gallery picker.
- **One-Click Revert:** Instant restoration of official icons with automatic cleanup of override files.

---

## Directory Structure

```
boxart-nx/
├── sysmodule/       # Atmosphere MitM Sysmodule (libstratosphere, C++20)
├── frontend/        # Frontend GUI Application (SDL2, ImGui, libjpeg-turbo, libcurl)
├── out/             # Ready-to-deploy SD card file tree
│   ├── atmosphere/
│   │   └── contents/
│   │       └── 010000000000B0A7/
│   │           ├── exefs.nsp       # Sysmodule executable PFS0
│   │           ├── toolbox.json    # Atmosphere sysmodule descriptor
│   │           └── flags/boot2.flag# Auto-launch on system boot
│   └── switch/
│       └── boxart-nx/
│           └── boxart-nx.nro       # Homebrew frontend application
└── Makefile         # Master root build orchestration
```

---

## Installation

1. Copy the contents of the `out/` folder to the root of your Switch microSD card:

```
sdmc:/
├── atmosphere/
│   └── contents/
│       └── 010000000000B0A7/
│           ├── exefs.nsp
│           ├── toolbox.json
│           └── flags/boot2.flag
├── switch/
│   └── boxart-nx/
│       └── boxart-nx.nro
└── config/
    └── boxart-nx/
        ├── config.ini         # SteamGridDB API key configuration (optional)
        ├── covers/            # Active custom cover icons (<TitleID>.jpg)
        └── import/            # Drop your custom images here (.png, .jpg, .webp)
```

2. Reboot the console (or enable the sysmodule via Atmosphere's sysmodule manager).
3. Launch `boxart-nx` from hbmenu to manage and apply game covers.

---

## SteamGridDB Setup (Optional)

To search and download covers directly from SteamGridDB on your console:

1. Obtain a free API key at [steamgriddb.com/profile/preferences/api](https://www.steamgriddb.com/profile/preferences/api).
2. Either enter it inside the `boxart-nx` frontend application, or create `sdmc:/config/boxart-nx/config.ini`:

```ini
[steamgriddb]
api_key = YOUR_STEAMGRIDDB_API_KEY
```

---

## Controller Controls

| Button | Action |
|---|---|
| **D-Pad / Left Stick** | Navigate game list and menus |
| **A** | Select / Apply cover |
| **B** | Back / Close modal dialog |
| **X** | Search SteamGridDB online for selected game |
| **Y** | Restore original official icon |
| **+ (Plus)** | Exit application |

---

## Building from Source

### Prerequisites
- [devkitPro](https://devkitpro.org/) with `devkitA64` and `libnx`.
- `libstratosphere` (Atmosphere libraries).
- Nintendo Switch portlibs:
  - `switch-sdl2`
  - `switch-libjpeg-turbo`
  - `switch-libwebp`
  - `switch-curl`
  - `switch-zlib`

### Compile
```bash
make
```

To clean and recompile:
```bash
make clean && make
```

Compiled deliverables will be automatically placed in the `out/` directory.

---

## License

This project is licensed under the terms of the GNU General Public License v2 (GPL-2.0).
