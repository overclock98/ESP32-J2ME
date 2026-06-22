# Diamond Rush → FlintESPJVM Port

Running the **unedited** Gameloft J2ME game *Diamond Rush* (S40 v5, `GloftDIRU`) on an
ESP32‑S3 via the **FlintESPJVM** Java VM. This repo (`FlintJDK`) provides the `java.base`
class library; the MIDP/CLDC runtime, the VM, and the board drivers live in the sibling
`FlintESPJVM` repo.

---

## Core requirements (do not violate)

1. **The game JAR must stay UNEDITED.** `DiamondRush_s40v5.jar` is the source of truth.
   All compatibility work goes into the **runtime** — `java.base` (this repo), the MIDP
   shim, the VM, or native drivers. Never patch/recompile the game classes to make it run.
2. **Future goal: load similar J2ME games from an SD card.** The launcher + classpath
   currently read the app/jars from the on‑flash FAT "storage" partition. The architecture
   should evolve so any MIDP/CLDC JAR can be dropped on an SD card and launched, without
   reflashing firmware. Keep game‑specific assumptions out of the VM/shim where possible.
3. **Do NOT store or rely on `/tmp`.** It can be wiped at any time. Anything under `/tmp`
   (e.g. extracted `.class` files, build logs) is disposable — always regenerate it from
   the JAR or the repos. Keep nothing important there.

---

## Where the code lives

### Java — `java.base` (this repo: `~/Documents/FlintJDK`)
Compiled with `make bin/run/java.base` → `bin/run/java.base.jar`.
```
src/java.base/java/
  lang/{Object,String,Math,System,Thread,Throwable,Class,Random,
        UnsupportedOperationException, ...}.java
  util/{Date,Calendar,Hashtable, ...}.java
  io/{ByteArrayInputStream,DataInputStream,DataOutputStream, ...}.java
```

### Java — MIDP/CLDC shim (`~/Documents/FlintESPJVM/midp/src`)
Compiled → `files/lib/midp.jar`.
```
board/{Lcd,Png}.java                       # native bridges (display, PNG decode, key input)
javax/microedition/lcdui/{Canvas,Graphics,Image,Font,Display,Displayable, ...}.java
javax/microedition/lcdui/game/{GameCanvas,Layer,TiledLayer,Sprite,LayerManager}.java
javax/microedition/{midlet,media,rms,io}/...
flint/drawing/RawImage.java
```

### Java — launcher (`~/Documents/FlintESPJVM/apps/GameRun/src/Main.java`)
Compiled → `files/FlintApp.jar`. Instantiates `GloftDIRU`, calls `startApp0`, and runs the
serial‑keyboard → game‑key input thread.

### Native C/C++ (compiled into firmware, `~/Documents/FlintESPJVM`)
```
espjvm/flintjvm/vm/src/{flint.cpp, flint_execution.cpp, flint_class_loader.cpp}   # VM core
espjvm/flintjvm/native/base/src/{flint_native_object.cpp, flint_native_thread.cpp}
espjvm/flintjvm/native/common/src/flint_native.cpp        # native class registration table
espjvm/system_api/src/esp_system_api.cpp                  # console I/O + key-input ring buffer
espjvm/core/src/{main.cpp, esp_usb_serial_jtag_debugger.cpp, esp_board_esp32_s3.cpp}
boards/ESP32-S3N16RX/main/{lcd.c, flint_native_lcd.cpp, flint_native_png.cpp}
boards/ESP32-S3N16RX/sdkconfig*                           # SPIRAM / console config
```

### Build, assets, and tools
```
~/Documents/flint_work/build.sh        # one-shot build (java.base + midp + app + firmware + image)
~/Documents/flint_work/play.py         # raw serial terminal that FORWARDS keystrokes (to play)
~/Downloads/DiamondRush_s40v5.jar      # the UNEDITED game (source of truth)
~/Documents/FlintESPJVM/files/         # becomes the FAT "storage" partition (jars + extracted res/)
~/Documents/FlintESPJVM/boards/build/ESP32-S3N16RX_FlintJVM_Release.bin   # merged 16MB flash image
```

### No system JDK
There is no `javac`/`javap` on `PATH`. `build.sh` puts the Minecraft runtime JDK on `PATH`
itself. For manual disassembly use the full path:
```
/home/bbnmn4800/.minecraft/runtime/java-runtime-delta/linux/java-runtime-delta/bin/javap
```

---

## Commands

### Build (everything)
```bash
cd ~/Documents/flint_work && ./build.sh
```
- Rebuilds `java.base.jar`, `midp.jar`, `FlintApp.jar`, the firmware, the FAT storage image,
  and the merged flash image. It does **NOT** flash.
- Always confirm the `java.base OK` line — a `javac` error there used to be silently swallowed.
- Output image: `~/Documents/FlintESPJVM/boards/build/ESP32-S3N16RX_FlintJVM_Release.bin`.

### Find the serial port (it changes between reconnects!)
```bash
ls /dev/ttyACM*
```
This firmware does **all** serial I/O — logs **and** key input — over the native
USB‑Serial‑JTAG, which enumerates as **`/dev/ttyACM*`**. A USB‑UART/TTL adapter
(`/dev/ttyUSB*`) is **not** used by the firmware. Use the `ttyACM*` port `ls` shows.

### Flash (close any monitor first — a busy port silently keeps the OLD firmware)
```bash
python3 -m esptool --chip esp32s3 -p /dev/ttyACM0 write_flash -z 0x0 \
  ~/Documents/FlintESPJVM/boards/build/ESP32-S3N16RX_FlintJVM_Release.bin
```
If it can’t connect: hold **BOOT**, tap **RESET**, release **BOOT**, retry.

### Play / monitor with working keyboard input  → use `play.py`
```bash
python3 ~/Documents/flint_work/play.py /dev/ttyACM0     # forwards keystrokes; quit with Ctrl-]
```
**Do not use `idf.py monitor` to play** — its `Ctrl-T` menu eats keystrokes
(`--- Error: Unknown menu character`), so keys never reach the device. `idf.py monitor` is
fine for read‑only log viewing (run it from `~/Documents/FlintESPJVM/boards/ESP32-S3N16RX`
so any backtrace symbols decode correctly).

### Controls (in `play.py`)
| Keys | Action |
|------|--------|
| `w`/`a`/`s`/`d` or `2`/`4`/`8`/`6` | up / left / down / right |
| `5` / `Space` / `e` | fire / select |
| `o` | left soft key (OK / Options) |
| `b` | right soft key (**Back**) |
| `0`–`9`, `*`, `#` | pass through |
| `Ctrl-]` | quit `play.py` |

Play with **sound OFF** for now (audio/MIDI is not implemented; sound‑on stalls the loop).

---

## Gotchas / lessons learned

- **Serial port number drifts** (`ttyACM0` ↔ `ttyACM1`). Always `ls /dev/ttyACM*` before
  flashing/monitoring.
- **Close the monitor before flashing.** A busy port lets the write silently no‑op (you’ll
  keep running the old firmware — identical boot log is the tell).
- **Keep console logging light.** Heavy/concurrent logging (esp. flood loops or
  `snprintf`/`vfprintf` on a Java thread’s PSRAM stack) can crash in `_vfprintf_r`. Use the
  VM’s manual `print(int64_t)` / `consoleWrite`, not `snprintf`, on hot/Java‑thread paths.
- **USB‑Serial‑JTAG RX:** typed bytes are read by the debugger task and forwarded to a key
  ring buffer in `esp_system_api.cpp`; `board.Lcd.readKey()` drains it. Forwarding is
  decoupled from the debugger’s frame state so a stray `0x00` can’t swallow input.
- **Change ONE thing at a time** and re‑test — bundling unrelated changes hid an input
  regression for several builds.

---

## Status (snapshot)

**Working:** boot, MIDlet creation, display, PNG decode for menus, menu navigation, sound
on/off screen, **serial keyboard input**, entering the first level.

**Fixed VM/runtime bugs this round:**
- `getMethodInfo` method index was `uint8_t` → truncated for classes with >256 methods
  (class `i` has 389; `<clinit>` @388 ran `ax` @132). Now `uint16_t`.
- `invokespecial` now NPE‑checks the receiver.
- `String(byte[],int,int,String charset)` added (ISO‑8859‑1 / UTF‑8); fixed Latin‑1
  sign‑extension in `charAt`/`getBytes`.
- `GameCanvas.paint()` concrete no‑op (game renders actively).
- `Thread.setPriority0` no longer throws (was killing the loader thread).
- Removed the `Object.wait()` debug flood and made GC logging crash‑safe.

**Open / next:**
- First level renders only partially — investigate tile/PNG render vs. perf (heavy GC).
- Audio (I2S MIDI) so sound‑on doesn’t stall.
- Persistent RMS (save data).
- **Load arbitrary J2ME games from SD card** (see requirement #2).
- Touch driver / on‑screen D‑pad (deferred).
</content>
