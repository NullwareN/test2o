#!/bin/bash

if [ $EUID != 0 ]; then
	echo "0"
	exit
fi

panel_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$panel_dir" || exit 1

mkdir -p logs
log_path="${CAT_PANEL_LOG:-panel.log}"
if [ -f "$log_path" ]; then
	log_size="$(stat -c%s "$log_path" 2>/dev/null || echo 0)"
	if [ "$log_size" -gt 52428800 ]; then
		mv -f "$log_path" "${log_path}.old" 2>/dev/null || true
	fi
fi
export CAT_STEAM_TXTMODE=0
export CAT_STM_WEBHELPER_TRIM="${CAT_STM_WEBHELPER_TRIM:-1}"
export CAT_STM_WEBHELPER_SINGLE="${CAT_STM_WEBHELPER_SINGLE:-0}"
export CAT_STM_WEBHELPER_NOSANDBOX="${CAT_STM_WEBHELPER_NOSANDBOX:-1}"
export CAT_STEAMWEBHELPER_CLEANUP="${CAT_STEAMWEBHELPER_CLEANUP:-0}"
export CAT_TEXTMODE_GAME="${CAT_TEXTMODE_GAME:-1}"
export CAT_PER_BOT_X_DISPLAY="${CAT_PER_BOT_X_DISPLAY:-1}"
export CAT_CHUNKED_X_DISPLAY="${CAT_CHUNKED_X_DISPLAY:-0}"

node_path="$(command -v node || command -v nodejs || true)"
if [ -z "$node_path" ]; then
	echo "node or nodejs is required to run the web panel." >&2
	exit 1
fi

ipc_setup="${CAT_PANEL_IPC_SETUP:-$panel_dir/scripts/ensure-ipc-server.sh}"
if [ -x "$ipc_setup" ]; then
	CAT_PANEL_DIR="$panel_dir" "$ipc_setup" >>"$log_path" 2>&1 ||
		echo "[ipc] automatic server setup failed; continuing with panel startup" >>"$log_path"
fi

bridge_setup="${CAT_PANEL_BRIDGE_SETUP:-$panel_dir/scripts/ensure-bot-bridge.sh}"
if [ -x "$bridge_setup" ]; then
	"$bridge_setup" >>"$log_path" 2>&1 ||
		echo "[net] bot bridge setup failed; continuing with panel startup" >>"$log_path"
fi

if [ ! -f public/bundle.js ] || [ script.js -nt public/bundle.js ] || [ steam_id.js -nt public/bundle.js ]; then
    "$node_path" node_modules/browserify/bin/cmd.js script.js -o public/bundle.js.tmp || exit 1
    mv -f public/bundle.js.tmp public/bundle.js || exit 1
fi

stopping=0
child_pid=""

stop_panel() {
	stopping=1
	if [ -n "$child_pid" ]; then
		kill "$child_pid" 2>/dev/null
		wait "$child_pid" 2>/dev/null
	fi
	exit 0
}

trap stop_panel INT TERM

while [ "$stopping" -eq 0 ]; do
	printf '[%s] starting web panel\n' "$(date -Is)" >>"$log_path"
	CAT_GDB_CRASH_REPORTS="${CAT_GDB_CRASH_REPORTS:-0}" "$node_path" app.js >>"$log_path" 2>&1 &
	child_pid="$!"
	wait "$child_pid"
	status="$?"
	child_pid=""
	printf '[%s] web panel exited status=%s\n' "$(date -Is)" "$status" >>"$log_path"
	if [ "$stopping" -ne 0 ] || [ "$status" -eq 0 ]; then
		exit "$status"
	fi
	sleep 2
done
