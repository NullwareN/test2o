#!/usr/bin/env bash

set -u

helper_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
original_helper="${CAT_STEAMWEBHELPER_ORIGINAL:-$helper_dir/steamwebhelper.original}"

if [ ! -x "$original_helper" ]; then
    for candidate in "$helper_dir"/steamwebhelper.original-*.sh "$helper_dir/steamwebhelper.real"; do
        if [ -x "$candidate" ]; then
            original_helper="$candidate"
            break
        fi
    done
fi

if [ ! -x "$original_helper" ] && [ -x "$helper_dir/steamwebhelper" ]; then
    original_helper="$helper_dir/steamwebhelper"
fi

library_path="${CAT_STEAM_TXTMODE_LIBRARY64:-/opt/cathook/botpanel/cat-steamtxtmode/bin/libx64/libcatsteamtxtmode.so}"
if [ "${CAT_STEAM_TXTMODE-0}" = 1 ] && [ -r "$library_path" ]; then
    export LD_PRELOAD="$library_path"
fi

if [ ! -x "$original_helper" ]; then
    echo "steamwebhelper wrapper: original helper not found in $helper_dir" >&2
    exit 127
fi

exec "$original_helper" "$@"
