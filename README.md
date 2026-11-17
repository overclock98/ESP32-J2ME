# Diamond Rush on ESP32 & ESP32-S3 (J2ME → Flint JVM)

Run the classic **Gameloft *Diamond Rush*** (a J2ME / MIDP-2.0 MIDlet) on bare
**ESP32-S3** and **ESP32** by running the actual Java bytecode on the [Flint JVM](https://github.com/FlintVN/FlintESPJVM),
with a from-scratch **MIDP/CLDC compatibility layer**, native **PNG decoding**, and an
**on-screen touch keypad** — no recompilation of the game, no emulator on a PC.

> The game's `.jar` is **not** included (it's Gameloft's intellectual property — see
> [`game/README.md`](game/README.md)). Everything needed to *build the runtime* is here;
> supply your own legally-obtained copy of the jar to build a flashable image.

---

## Supported Hardware

### 1. Guition JC3248W535EN
- **MCU:** ESP32-S3-WROOM-1 **N16R8** (16 MB flash / 8 MB PSRAM)
- **Display:** AXS15231B, **QSPI**, 320×480, RGB565
- **Touch:** I²C capacitive (same AXS15231B controller)
- **Audio / SD:** Present on board (audio WIP)
- **Build target:** `boards/ESP32-S3N16RX/` (via `./build.sh`)

### 2. Wireless-Tag WT32-SC01 (V3.4)
- **MCU:** ESP32-WROVER-B (Xtensa dual-core LX6, 16 MB flash DIO / 8 MB PSRAM)
- **Display:** ST7796, **4-line HSPI**, 320×480, RGB565
- **Touch:** FocalTech FT6336U I²C capacitive touch (`SDA=18`, `SCL=19`, `INT=39`)
- **Keypad:** Composited 320×160 native on-screen virtual keypad (numpad + softkeys)
- **Build target:** `boards/WT32-SC01/` (via `./build_wt32sc01.sh`)
- See [`README.WT32-SC01.md`](README.WT32-SC01.md) for full pinout, flash offsets, and hardware notes.

## What this is

J2ME games are Java bytecode against the `javax.microedition.*` (MIDP/CLDC) API. The Flint
JVM can run Java bytecode on the ESP32, but ships no MIDP. This project adds everything in
between so the **unmodified** game jar runs on the chip:

```
   Diamond Rush .jar (unmodified bytecode)
            │
   ┌────────▼─────────────────────────────────────────────┐
   │ MIDP / CLDC shim  (midp/, pure Java)                  │
   │  lcdui {Canvas,Graphics,Image,Font,Display} +         │
   │  lcdui.game {GameCanvas,Sprite,TiledLayer,Layer,…} +  │
   │  midlet / media / rms / io                            │
   └────────┬───────────────────────────────┬─────────────┘
            │ flint.drawing (RGB565 raster)  │ board.* natives
   ┌────────▼──────────┐            ┌────────▼─────────────────────────┐
   │ Flint JVM (espjvm)│            │ board.Lcd  (QSPI panel present)   │
   │  + java.base      │            │ board.Touch(I²C + on-screen keypad)│
   │  (FlintJDK)       │            │ board.Png  (native PNG decode)    │
   └───────────────────┘            └───────────────────────────────────┘
                         ESP-IDF v5.5.x · ESP32-S3
```

Key pieces built for this port:

| Area | Where |
|------|-------|
| MIDP / CLDC + Game API shim | [`midp/src/`](midp/src) → `files/lib/midp.jar` |
| `java.base` additions (Random, Date/Calendar, IO, …) | [`FlintJDK/src/java.base/`](FlintJDK/src/java.base) → `files/lib/java.base.jar` |
| Display driver + present | [`boards/ESP32-S3N16RX/main/lcd.c`](boards/ESP32-S3N16RX/main/lcd.c), `flint_native_lcd.cpp` |
| On-screen touch keypad | `boards/ESP32-S3N16RX/main/touch.c`, `flint_native_touch.cpp`, [`midp/src/board/Touch.java`](midp/src/board/Touch.java) |
| Native PNG decoder (ROM miniz) | `boards/ESP32-S3N16RX/main/flint_native_png.cpp`, [`midp/src/board/Png.java`](midp/src/board/Png.java) |
| Launcher (instantiates the MIDlet) | [`apps/GameRun/src/Main.java`](apps/GameRun/src/Main.java) → `files/FlintApp.jar` |
| RGB565/ARGB565 blits, VM fixes | `espjvm/flintjvm/native/draw/...`, `espjvm/flintjvm/vm/...` |
| Board config (PSRAM-first malloc, etc.) | `boards/ESP32-S3N16RX/sdkconfig.defaults*` |

The game and its assets live on a FAT **`storage`** flash partition (built from
[`files/`](files)); the firmware mounts it and runs `FlintApp.jar`, which loads the jar's
classes and `getResourceAsStream`s the PNG/MIDI assets from `/res`.

---

## Prerequisites

- **ESP-IDF v5.5.x** installed and exported (`. $IDF/export.sh`). Provides `idf.py`, `esptool`, `python3`.
- **JDK 17+** on `PATH` (tested with 21) — `javac` / `jar`.
- `make`, `unzip`, `bash`.
- Your own copy of **`DiamondRush_s40v5.jar`** placed in [`game/`](game/README.md).

## Build

### For Guition JC3248W535EN (ESP32-S3):
```bash
# 1) put your jar in place
cp /path/to/DiamondRush_s40v5.jar game/DiamondRush_s40v5.jar

# 2) point at your ESP-IDF (or export it yourself first), then build
ESP_IDF=$HOME/esp/v5.5.2/esp-idf ./build.sh
```
Output image: `boards/build/ESP32-S3N16RX_FlintJVM_Release.bin`

### For Wireless-Tag WT32-SC01 (ESP32):
```bash
./build_wt32sc01.sh
```
Output image: `boards/build/WT32-SC01_FlintJVM_Release.bin`

## Flash

Close any serial monitor first (only one program may own the port).

### ESP32-S3 (Guition):
```bash
python3 -m esptool --chip esp32s3 -p /dev/ttyACM0 write_flash -z 0x0 \
  boards/build/ESP32-S3N16RX_FlintJVM_Release.bin
```
Console monitor (USB-Serial-JTAG):
```bash
idf.py -C boards/ESP32-S3N16RX -p /dev/ttyACM0 monitor
```

### ESP32 (WT32-SC01):
```bash
python -m esptool --chip esp32 -p COM10 -b 921600 write_flash -z 0x0 \
  boards/build/WT32-SC01_FlintJVM_Release.bin
```
Console monitor (UART0 @ 460800 baud):
```bash
python -m serial.tools.miniterm COM10 460800
```

You should see `Diamond Rush: boot` → `creating MIDlet GloftDIRU` → asset loads → the game
rendering, with the touch keypad in the bottom strip.

---

## Status

- ✅ Boots the genuine Gameloft `GloftDIRU` MIDlet; loads all assets.
- ✅ MIDP + Game API render path on the QSPI panel; **native PNG decode** (real art).
- ✅ On-screen **touch keypad** (7-8-9 / 4-5-6 / 1-2-3 / \*-0-#) drives input on the game thread.
- ✅ Memory fix: all allocations routed to PSRAM (was exhausting the ~512 KB internal heap).
- 🚧 Audio (I²S MIDI), persistent RMS save, perf tuning, keypad layout polish.

See [`CHANGELOG.md`](CHANGELOG.md) (upstream Flint) and commit history for details.

## Repository layout

```
espjvm/        Flint JVM core (vm, native, system_api, impl)   [from FlintESPJVM]
FlintJDK/      java.base sources (compiled to java.base.jar)   [from FlintJDK]
midp/          MIDP/CLDC + Game API shim + board.{Lcd,Touch,Png} Java
boards/        ESP-IDF project for the JC3248W535EN board
apps/GameRun/  launcher MIDlet host (Main.java)
files/         staged into the FAT "storage" partition (lib jars, app, assets)
game/          <-- put DiamondRush_s40v5.jar here (not shipped)
build.sh       one-shot build
```

## Credits & licensing

- **Flint JVM** — [FlintVN/FlintESPJVM](https://github.com/FlintVN/FlintESPJVM) and
  **FlintJDK** — [FlintVN/FlintJDK](https://github.com/FlintVN/FlintJDK). See
  [`LICENSE.txt`](LICENSE.txt) and [`README.FlintESPJVM.md`](README.FlintESPJVM.md). This repo
  contains modified copies of those projects' sources.
- **Diamond Rush** is © **Gameloft**. The game jar/assets are **not** distributed here; you must
  supply your own. This project is a non-commercial compatibility/education effort.
- The MIDP shim, board drivers, PNG decoder, touch keypad and integration glue in this repo are
  the project's own work; reuse under the same terms as the upstream Flint license unless noted.
