#!/usr/bin/env bash
set -euo pipefail

ROOT="${CATHOOK_ROOT:-/opt/cathook}"
LIB="${LIB:-$ROOT/bin/libcathooktextmode.so}"
if [ ! -f "$LIB" ]; then
    LIB="$ROOT/bin/libcathook-textmode.so"
fi
if [ ! -f "$LIB" ]; then
    LIB="$ROOT/bin/libcathook.so"
fi
if [ ! -f "$LIB" ]; then
    echo "Missing cathook library under $ROOT/bin" >&2
    exit 1
fi

PROCID="${1:-}"
if [ -z "$PROCID" ]; then
    echo "usage: attach.sh <pid>" >&2
    exit 1
fi

STAGE_DIR="$(mktemp -d /tmp/cathook-runtime-XXXXXX)"
STAGE_LIB="$STAGE_DIR/libcathook.so"
install -m 0755 "$LIB" "$STAGE_LIB"

gdb -n -q --batch \
    -ex "set pagination off" \
    -ex "set confirm off" \
    -ex "attach $PROCID" \
    -ex "call ((void *(*)(const char *, int)) dlopen)(\"$STAGE_LIB\", 1)" \
    -ex "call ((char *(*)(void)) dlerror)()" \
    -ex "detach" \
    -ex "quit"
