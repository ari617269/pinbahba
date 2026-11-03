#!/usr/bin/env bash
set -euo pipefail
MODE=${1:-view}; THEME=${2:-light}
DIR=$(mktemp -d)
if compgen -G "tests/fixtures/*.md" > /dev/null; then cp tests/fixtures/*.md "$DIR"/; fi
BIN=build/dev/md.app/Contents/MacOS/md
[ -x "$BIN" ] || BIN=build/dev/md
"$BIN" --data-dir "$DIR" --screenshot /tmp/md-shot.png \
  --mode "$MODE" --theme "$THEME" --size 1100x700
echo "/tmp/md-shot.png"
