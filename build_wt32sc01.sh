#!/usr/bin/env bash
#
# One-shot build for WT32-SC01 (ESP32-WROVER-B, 16MB Flash)
#
set -e
set -o pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

# ---------------- CONFIG ----------------
: "${ESP_IDF:=/opt/esp-idf}"
: "${GAME_JAR:=$ROOT/game/DiamondRush_s40v5.jar}"
JDK="$ROOT/FlintJDK"
LIB="files/lib"
BOARD="WT32-SC01"
# ----------------------------------------

if [ -z "$IDF_PATH" ]; then
    [ -f "$ESP_IDF/export.sh" ] || { echo "ESP-IDF not found at $ESP_IDF"; exit 1; }
    # shellcheck disable=SC1091
    . "$ESP_IDF/export.sh" >/dev/null 2>&1
fi

command -v javac >/dev/null || { echo "javac not found"; exit 1; }
[ -f "$GAME_JAR" ] || { echo "Game jar not found at: $GAME_JAR"; exit 1; }

echo "=== 1. java.base ==="
if [ ! -f "$ROOT/$LIB/java.base.jar" ]; then
    ( cd "$JDK" && rm -rf bin && make bin/run/java.base JC=javac >/dev/null && cp bin/run/java.base.jar "$ROOT/$LIB/java.base.jar" ) \
        && echo "  java.base OK" || { echo "  java.base FAILED"; exit 1; }
else
    echo "  java.base.jar already exists, skipping."
fi

echo "=== 2. midp.jar ==="
rm -rf midp/bin && mkdir -p midp/bin
find midp/src -name '*.java' > /tmp/midp_sources.txt
javac -g -XDstringConcat=inline --system=none -encoding UTF-8 --module-path "$LIB/java.base.jar" -cp "$LIB/*" \
    -d midp/bin @/tmp/midp_sources.txt
jar c0f "$LIB/midp.jar" -C midp/bin . && echo "  midp OK"

echo "=== 3. extract game assets -> files/res ==="
rm -rf files/res; mkdir -p files/res
( cd files/res && unzip -oq "$GAME_JAR" -x '*.class' && rm -rf META-INF )
mkdir -p files/res/META-INF && unzip -oq "$GAME_JAR" META-INF/MANIFEST.MF -d files/res/ 2>/dev/null || true
cp files/res/META-INF/MANIFEST.MF files/res/MANIFEST.MF 2>/dev/null || true
echo "  res files: $(find files/res -type f | wc -l)"

echo "=== 4. app FlintApp.jar ==="
rm -rf apps/GameRun/stage apps/GameRun/bin; mkdir -p apps/GameRun/stage apps/GameRun/bin
( cd apps/GameRun/stage && unzip -oq "$GAME_JAR" '*.class' )
javac -g -XDstringConcat=inline --system=none -encoding UTF-8 --module-path "$LIB/java.base.jar" \
    -cp "$LIB/*:apps/GameRun/stage" -d apps/GameRun/bin apps/GameRun/src/*.java
cp apps/GameRun/bin/*.class apps/GameRun/stage/
jar c0fe files/FlintApp.jar Main -C apps/GameRun/stage . && echo "  app OK"

echo "=== 5. firmware (idf.py build for $BOARD) ==="
cd "boards/$BOARD"
idf.py build

echo "=== 6. pack storage partition + merge flash image ==="
cd build
python3 "$IDF_PATH/components/fatfs/wl_fatfsgen.py" --output_file storage.bin \
    --partition_size 0xE70000 --sector_size 512 --long_name_support ../../../files >/dev/null 2>&1
mkdir -p ../../build
python3 -m esptool --chip esp32 merge_bin --flash_mode dio --flash_size 16MB --flash_freq 80m \
    -o "../../build/${BOARD}_FlintJVM_Release.bin" \
    0x1000 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin \
    0x10000 "${BOARD}_FlintJVM.bin" 0x190000 storage.bin

echo "=== DONE -> boards/build/${BOARD}_FlintJVM_Release.bin ==="
