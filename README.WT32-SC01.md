# Porting ESP32-J2ME to Wireless-Tag WT32-SC01 (V3.4, 16MB)

This port runs unmodified J2ME MIDP 2.0 Java games (such as Gameloft's *Diamond Rush*) on the **Wireless-Tag WT32-SC01** development board using the [Flint JVM](https://github.com/FlintVN/FlintESPJVM).

---

## Hardware Specification (`WT32-SC01` V3.4)

| Subsystem | Hardware Spec | Pin Connections |
| :--- | :--- | :--- |
| **MCU** | ESP32-WROVER-B (Xtensa dual-core LX6 @ 240 MHz) | Internal |
| **Flash** | 16 MB SPI Flash (`DIO` mode, 80 MHz) | Internal |
| **PSRAM** | 8 MB Quad-SPI PSRAM | Internal |
| **USB/UART** | CP2104 USB-UART bridge | UART0 (`TXD0=GPIO1`, `RXD0=GPIO3`), default console 460800 baud |
| **Display** | 3.5" ST7796 320×480 RGB565 (4-line HSPI) | `MOSI=GPIO13`, `SCLK=GPIO14`, `CS=GPIO15`, `DC=GPIO21`, `RST=GPIO22` |
| **Backlight** | Active-HIGH via S8050 NPN transistor | `GPIO23` |
| **Capacitive Touch** | FocalTech FT6336U / FT6236 (I2C address `0x38`) | `SDA=GPIO18`, `SCL=GPIO19`, `INT=GPIO39` (onboard 10 kΩ pull-ups) |

---

## Screen & Keypad Layout

The 320×480 panel is partitioned into two regions:
- **Top 320×320 (`GAME_H`)**: Native J2ME Game Viewport (Diamond Rush rendered 1:1, centered).
- **Bottom 320×160 (`KEYPAD_H`)**: Native on-screen capacitive touch keypad:
  - Phone numpad: `1`–`9`, `*`, `0`, `#`
  - Softkeys: `OK / Select` (Left Softkey) and `BK / Back` (Right Softkey) overlaid on the lower screen boundary.

---

## Prerequisites

- **ESP-IDF v5.4.x / v5.5.x** (`. /opt/esp-idf/export.sh`)
- **JDK 17+** (`javac`, `jar`)
- **Python 3.10+** with `esptool`

---

## Building the Release Binary

Run the one-shot build script:

```bash
cd ESP32-J2ME
./build_wt32sc01.sh
```

The build script compiles:
1. `java.base.jar` (FlintJDK)
2. `midp.jar` (MIDP/CLDC compatibility layer)
3. Unpacks `DiamondRush_s40v5.jar` assets into `files/res/`
4. `FlintApp.jar` (Game launcher)
5. Compiles firmware with `idf.py build`
6. Packages FAT partition (`storage.bin`) and merges into:
   ```
   boards/build/WT32-SC01_FlintJVM_Release.bin
   ```

---

## Flashing & Monitoring

Flash the merged 16MB image starting at offset `0x0`:

```bash
# Windows PowerShell
python -m esptool --chip esp32 -p COM10 -b 921600 write_flash -z 0x0 boards\build\WT32-SC01_FlintJVM_Release.bin

# Linux / macOS
python3 -m esptool --chip esp32 -p /dev/ttyUSB0 -b 921600 write_flash -z 0x0 boards/build/WT32-SC01_FlintJVM_Release.bin
```

To view the live serial console and touch events:

```bash
python -m serial.tools.miniterm COM10 460800
```
