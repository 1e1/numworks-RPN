#!/usr/bin/env bash
# Container entrypoint: build (once) the Epsilon web simulator, build the RPN
# .nwb, then serve both. Expects two mounts:
#   /epsilon   an Epsilon fork ROOT (the folder containing epsilon/, haussmann/,
#              poincare/, shared/ …) — the build runs from its epsilon/ subdir.
#   /app       this repository.
# A persistent volume on /serve caches the built simulator between runs.
set -euo pipefail

EPSILON=/epsilon/epsilon
APP=/app
SERVE=/serve
mkdir -p "$SERVE"

if [ ! -f "$EPSILON/Makefile" ]; then
  echo "!! /epsilon/epsilon/Makefile not found." >&2
  echo "   Mount the Epsilon fork ROOT at /epsilon (it must contain epsilon/," >&2
  echo "   haussmann/, poincare/, shared/). See docker/run.sh." >&2
  exit 1
fi

if [ ! -f "$SERVE/epsilon.html" ]; then
  echo ">> Building the Epsilon web simulator — first run is long (cached afterwards)…"
  ( cd "$EPSILON" && make PLATFORM=web WEB_EXTERNAL_APPS=1 -j"$(nproc)" epsilon.html )
  cp "$EPSILON"/output/release/web/epsilon.* "$SERVE"/
else
  echo ">> Reusing the cached Epsilon web simulator (/serve volume)."
fi

echo ">> Building the RPN app for web (.nwb)…"
( cd "$APP" && make PLATFORM=web )
cp "$APP"/output/rpn.nwb "$SERVE"/rpn.nwb

echo
echo "==============================================================="
echo "  Open:  http://localhost:8000/epsilon.html?nwb=/rpn.nwb"
echo "==============================================================="
echo
cd "$SERVE" && exec python3 -m http.server 8000
