#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

BUILD=build/release-mac
APP=$BUILD/md.app
DIST=dist

cmake -S . -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build "$BUILD"

macdeployqt "$APP" -always-overwrite

mkdir -p "$DIST"
hdiutil create -volname md -srcfolder "$APP" -ov -format UDZO "$DIST/md-macos-arm64.dmg"
