#!/usr/bin/env bash
# Formats C++/ObjC++ files after Claude edits them.
f=$(jq -r '.tool_input.file_path // empty')
case "$f" in *.cpp|*.h|*.mm) clang-format -i "$f" ;; esac
exit 0
