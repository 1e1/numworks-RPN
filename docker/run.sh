#!/usr/bin/env bash
# Build the image and run the web simulator in a browser via Docker.
#
#   docker/run.sh
#
# By default it looks for an Epsilon checkout next to this repo:
#   ../epsilon/epsilon  (the dir that contains Epsilon's Makefile)
# Override with EPSILON_DIR=/path/to/epsilon docker/run.sh
set -euo pipefail

APP_DIR="$(cd "$(dirname "$0")/.." && pwd)"
# Epsilon fork ROOT: the folder containing epsilon/, haussmann/, poincare/, shared/.
EPSILON_DIR="${EPSILON_DIR:-$APP_DIR/../epsilon}"

if [ ! -f "$EPSILON_DIR/epsilon/Makefile" ]; then
  echo "Epsilon fork root not found at: $EPSILON_DIR" >&2
  echo "Set EPSILON_DIR to the fork root (it must contain epsilon/, haussmann/, …)." >&2
  exit 1
fi
EPSILON_DIR="$(cd "$EPSILON_DIR" && pwd)"

echo ">> App:     $APP_DIR"
echo ">> Epsilon: $EPSILON_DIR"

docker build -t numworks-rpn-sim "$APP_DIR/docker"
docker run --rm -it -p 8000:8000 \
  -v "$EPSILON_DIR":/epsilon \
  -v "$APP_DIR":/app \
  -v numworks-rpn-simcache:/serve \
  -v numworks-rpn-emcache:/emsdk/upstream/emscripten/cache \
  numworks-rpn-sim
