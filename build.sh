#!/usr/bin/env bash
#
# One-shot build: java.base -> midp.jar -> FlintApp.jar -> ESP-IDF firmware -> merged flash image.
# Run from anywhere; it locates itself. Override paths via env vars (see CONFIG below).
#
#   ./build.sh
#
# Requires: ESP-IDF v5.5.x, a JDK 17+ (javac/jar on PATH), make, unzip, python3.
# Produces: boards/build/ESP32-S3N16RX_FlintJVM_Release.bin   (flash at 0x0)
#
set -e
set -o pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

# ---------------- CONFIG (override via environment) ----------------
: "${ESP_IDF:=$HOME/esp/v5.5.2/esp-idf}"                 # path to ESP-IDF v5.5.x checkout
: "${GAME_JAR:=$ROOT/game/DiamondRush_s40v5.jar}"        # the J2ME game jar (NOT shipped, see game/README.md)
JDK="$ROOT/FlintJDK"
LIB="files/lib"
# -------------------------------------------------------------------

# ESP-IDF environment (skip if already exported)
if [ -z "$IDF_PATH" ]; then
    [ -f "$ESP_IDF/export.sh" ] || { echo "ESP-IDF not found at $ESP_IDF (set ESP_IDF=...)"; exit 1; }
    # shellcheck disable=SC1091
    . "$ESP_IDF/export.sh" >/dev/null 2>&1
fi

command -v javac >/dev/null || { echo "javac not found — install a JDK 17+ and put it on PATH"; exit 1; }
[ -f "$GAME_JAR" ] || { echo "Game jar not found at: $GAME_JAR"; echo "See game/README.md — drop your own copy there (set GAME_JAR=... to point elsewhere)."; exit 1; }

echo "=== java.base ==="
( cd "$JDK" && rm -rf bin && make bin/run/java.base JC=javac >/dev/null && cp bin/run/java.base.jar "$ROOT/$LIB/java.base.jar" ) \
    && echo "  java.base OK" || { echo "  java.base FAILED — aborting (stderr above)"; exit 1; }

echo "=== midp.jar ==="
rm -rf midp/bin && mkdir -p midp/bin
javac -g -XDstringConcat=inline --system=none -encoding UTF-8 --module-path "$LIB/java.base.jar" -cp "$LIB/*" \
    -d midp/bin $(find midp/src -name '*.java')
jar c0f "$LIB/midp.jar" -C midp/bin . && echo "  midp OK"

echo "=== extract game assets -> files/res ==="
rm -rf files/res; mkdir -p files/res
( cd files/res && unzip -oq "$GAME_JAR" -x '*.class' && rm -rf META-INF )
mkdir -p files/res/META-INF && unzip -oq "$GAME_JAR" META-INF/MANIFEST.MF -d files/res/ 2>/dev/null || true
cp files/res/META-INF/MANIFEST.MF files/res/MANIFEST.MF 2>/dev/null || true
echo "  res files: $(find files/res -type f | wc -l)"

echo "=== app FlintApp.jar ==="
rm -rf apps/GameRun/stage apps/GameRun/bin; mkdir -p apps/GameRun/stage apps/GameRun/bin
( cd apps/GameRun/stage && unzip -oq "$GAME_JAR" '*.class' )
javac -g -XDstringConcat=inline --system=none -encoding UTF-8 --module-path "$LIB/java.base.jar" \
    -cp "$LIB/*:apps/GameRun/stage" -d apps/GameRun/bin apps/GameRun/src/*.java
cp apps/GameRun/bin/*.class apps/GameRun/stage/
jar c0fe files/FlintApp.jar Main -C apps/GameRun/stage . && echo "  app OK"

echo "=== firmware (idf.py build) ==="
cd boards/ESP32-S3N16RX
idf.py build 2>&1 | tail -20
echo "  firmware OK"

cd build
echo "=== pack storage partition + merge flash image ==="
python3 "$IDF_PATH/components/fatfs/wl_fatfsgen.py" --output_file storage.bin \
    --partition_size 0xE70000 --sector_size 512 --long_name_support ../../../files >/dev/null 2>&1
python3 -m esptool --chip esp32s3 merge_bin --flash_mode dio --flash_size 16MB --flash_freq 80m \
    -o ../../build/ESP32-S3N16RX_FlintJVM_Release.bin \
    0x0 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin \
    0x10000 ESP32-S3N16RX_FlintJVM.bin 0x190000 storage.bin 2>&1 | grep -iE 'Wrote' | tail -1

echo "=== DONE -> boards/build/ESP32-S3N16RX_FlintJVM_Release.bin ==="
echo "Flash:  python3 -m esptool --chip esp32s3 -p /dev/ttyACM0 write_flash -z 0x0 boards/build/ESP32-S3N16RX_FlintJVM_Release.bin"
