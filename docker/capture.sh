#!/usr/bin/env bash
# Container entrypoint: build the Epsilon Linux simulator (once), build the RPN
# app as a native shared object with a screenshot entry point that dumps the
# framebuffer, run it headless, and convert the dump to /out/render.png.
# Mounts: /epsilon (fork root), /app (this repo), /out (output dir).
set -euo pipefail

EPSILON=/epsilon/epsilon
APP=/app
OUT=/out
BIN="$EPSILON/output/release/linux/epsilon.bin"

[ -f "$EPSILON/Makefile" ] || { echo "!! mount the Epsilon fork ROOT at /epsilon" >&2; exit 1; }

if [ ! -f "$BIN" ]; then
  echo ">> Building the Epsilon Linux simulator — first run is long (reused after)…"
  ( cd "$EPSILON" && make PLATFORM=linux -j"$(nproc)" epsilon.bin )
fi

CAP="$EPSILON/external_apps/rpn_capture"
rm -rf "$CAP"; mkdir -p "$CAP/src"
cp "$APP"/src/*.cpp "$APP"/src/*.h "$APP"/src/icon.png "$CAP/src/"
rm -f "$CAP/src/main.cpp"
cp "$APP"/experimental/screenshot_main.cpp "$CAP/src/main.cpp"
sed -i 's#\.\./src/##' "$CAP/src/main.cpp"
printf 'APP_NAME = rpn_capture\nAPP_ICON = src/icon.png\nOUTPUT_DIR = output\nSOURCES = $(wildcard src/*.cpp)\ninclude ../build/external_app.mak\n' > "$CAP/Makefile"

echo ">> Building the app as a native shared object…"
( cd "$CAP" && make PLATFORM=linux SIMULATOR="$BIN" build )
NWB="$CAP/output/linux/rpn_capture.nwb"

echo ">> Rendering headless and capturing the framebuffer…"
( cd "$EPSILON" && timeout 40 "$BIN" --headless --take-screenshot /tmp/ignore.png --nwb "$NWB" || true )
python3 -c "from PIL import Image; Image.open('$OUT/shot.ppm').save('$OUT/render.png')"
rm -rf "$CAP" "$OUT/shot.ppm"
echo ">> Wrote /out/render.png"
