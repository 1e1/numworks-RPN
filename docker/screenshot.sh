#!/usr/bin/env bash
# Render the app in the Epsilon Linux simulator and write a real-font screenshot
# to capture/render.png — only Docker is needed.
#
#   docker/screenshot.sh
#
# Needs an Epsilon fork ROOT next to this repo (../epsilon), i.e. the folder
# containing epsilon/, haussmann/, poincare/, shared/. Override with
#   EPSILON_DIR=/path/to/epsilon docker/screenshot.sh
set -euo pipefail

APP_DIR="$(cd "$(dirname "$0")/.." && pwd)"
EPSILON_DIR="${EPSILON_DIR:-$APP_DIR/../epsilon}"
OUT_DIR="${OUT_DIR:-$APP_DIR/capture}"

if [ ! -f "$EPSILON_DIR/epsilon/Makefile" ]; then
  echo "Epsilon fork root not found at: $EPSILON_DIR" >&2
  echo "Set EPSILON_DIR to the fork root (contains epsilon/, haussmann/, …)." >&2
  exit 1
fi
EPSILON_DIR="$(cd "$EPSILON_DIR" && pwd)"
mkdir -p "$OUT_DIR"

# Build host tools for the native arch (an amd64 tool under Rosetta breaks fonts).
case "$(uname -m)" in
  arm64|aarch64) PLATFORM=linux/arm64 ;;
  *)             PLATFORM=linux/amd64 ;;
esac

docker build --platform "$PLATFORM" -t numworks-rpn-capture \
  -f "$APP_DIR/docker/Dockerfile.capture" "$APP_DIR/docker"
docker run --rm --platform "$PLATFORM" \
  -v "$EPSILON_DIR":/epsilon \
  -v "$APP_DIR":/app \
  -v "$OUT_DIR":/out \
  numworks-rpn-capture

echo "Wrote $OUT_DIR/render.png"
