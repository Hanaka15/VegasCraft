#!/usr/bin/env bash
# Steam launch option for Fallout New Vegas (Proton) + xNVSE + VegasCraft:
#   bash /home/hanaka/VegasCraft/tools/proton_nvse_launch.sh %command%
#
# Critical: Minecraft and Fallout must share ONE Proton/wineserver.
# A second `proton run` for Prism makes the next launch hang on `wineserver -w`
# and Fallout never starts.
#
# Flow: unpack Prism → rewrite target to VegasCraft_boot.cmd → one Proton session
# runs Prism (start) then FalloutNV.exe (/wait).
set -euo pipefail

FNV="${VEGASCRAFT_FNV:-$HOME/.local/share/Steam/steamapps/common/Fallout New Vegas enplczru}"
GAME="$FNV/FalloutNV.exe"
BOOT_CMD="$FNV/VegasCraft_boot.cmd"
INNER_CMD="$FNV/VegasCraft_inner.cmd"
REPO_BOOT="${VEGASCRAFT_REPO:-$HOME/VegasCraft}/tools/VegasCraft_boot.cmd"
REPO_INNER="${VEGASCRAFT_REPO:-$HOME/VegasCraft}/tools/VegasCraft_inner.cmd"
STEAM_LOADER="$FNV/nvse_steam_loader.dll"
NVSE_DLL="$FNV/nvse_1_4.dll"
ASI_LOADER="$FNV/dinput8.dll"
ASI_NVSE="$FNV/nvse_steam_loader.asi"
PLUGIN="$FNV/Data/NVSE/Plugins/vegascraft.dll"
PLUGIN_INI="$FNV/Data/NVSE/Plugins/VegasCraft.ini"
BUNDLE="$FNV/Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip"
REPO_ASI="${VEGASCRAFT_REPO:-$HOME/VegasCraft}/tools/proton-nvse/dinput8.dll"
LOG="${XDG_RUNTIME_DIR:-/tmp}/vegascraft-launch.log"
APPID="${SteamAppId:-${SteamGameId:-22490}}"
COMPAT="${STEAM_COMPAT_DATA_PATH:-$HOME/.local/share/Steam/steamapps/compatdata/$APPID}"
PRISM_DIR="$COMPAT/pfx/drive_c/users/steamuser/AppData/Local/VegasCraft"
PRISM_EXE="$PRISM_DIR/Prism/prismlauncher.exe"

log() { printf '%s\n' "$*" | tee -a "$LOG" >&2; }

: >"$LOG"
log "VegasCraft launch $(date -Iseconds)"
log "COMPAT=$COMPAT"

if [[ ! -f "$GAME" ]]; then
	log "ERROR: missing $GAME"
	exec "$@"
fi

# Install boot cmd + ASI NVSE chainload
if [[ -f "$REPO_BOOT" ]]; then
	cp -f "$REPO_BOOT" "$BOOT_CMD"
fi
if [[ -f "$REPO_INNER" ]]; then
	cp -f "$REPO_INNER" "$INNER_CMD"
fi
if [[ ! -f "$ASI_LOADER" && -f "$REPO_ASI" ]]; then
	cp -f "$REPO_ASI" "$ASI_LOADER"
	log "Installed dinput8.dll ASI loader"
fi
if [[ -f "$STEAM_LOADER" && -f "$ASI_LOADER" ]]; then
	if [[ ! -f "$ASI_NVSE" ]] || ! cmp -s "$STEAM_LOADER" "$ASI_NVSE"; then
		cp -f "$STEAM_LOADER" "$ASI_NVSE"
		log "Installed nvse_steam_loader.asi"
	fi
fi

if [[ ! -f "$BOOT_CMD" ]]; then
	log "ERROR: missing $BOOT_CMD"
fi
if [[ ! -f "$STEAM_LOADER" || ! -f "$NVSE_DLL" ]]; then
	log "WARNING: xNVSE incomplete"
fi
if [[ ! -f "$ASI_LOADER" || ! -f "$ASI_NVSE" ]]; then
	log "WARNING: missing ASI NVSE chainload files"
fi
if [[ ! -f "$PLUGIN" ]]; then
	log "WARNING: missing $PLUGIN"
fi
if [[ ! -f "$BUNDLE" ]]; then
	log "WARNING: missing $BUNDLE"
else
	log "Found Minecraft bundle: $BUNDLE"
fi

# Plugin must not CreateProcess a second Prism.
if [[ -f "$PLUGIN_INI" ]]; then
	sed -i 's/^bStartWithHost\s*=\s*1/bStartWithHost = 0/' "$PLUGIN_INI" || true
	log "Set bStartWithHost=0 (Prism started by VegasCraft_boot.cmd)"
fi

# Windowed (bFull Screen=0). iPresentInterval=0 = no vsync lag.
force_display_prefs() {
	local docs="$COMPAT/pfx/drive_c/users/steamuser/Documents/My Games/FalloutNV"
	local f
	for f in "$docs/FalloutPrefs.ini" "$docs/Fallout.ini"; do
		[[ -f "$f" ]] || continue
		python3 - "$f" <<'PY'
import pathlib, sys
p = pathlib.Path(sys.argv[1])
raw = p.read_bytes()
nl = b"\r\n" if b"\r\n" in raw else b"\n"
out = []
for line in raw.decode("utf-8", "replace").splitlines():
    if line.startswith("bFull Screen="):
        out.append("bFull Screen=0")
    elif line.startswith("iPresentInterval="):
        out.append("iPresentInterval=0")
    else:
        out.append(line)
p.write_bytes(nl.join(x.encode() for x in out) + nl)
PY
		log "Windowed prefs: $f"
	done
}
force_display_prefs

# Raise Fallout window; keep Minecraft unmapped. Clear Steam LD_PRELOAD for xdotool.
start_fnv_focus_helper() {
	local helper_log="${XDG_RUNTIME_DIR:-/tmp}/vegascraft-focus.log"
	: >"$helper_log"
	(
		unset LD_PRELOAD
		export LD_PRELOAD=""
		for _ in $(seq 1 90); do
			sleep 1
			if command -v xdotool >/dev/null 2>&1; then
				xdotool search --name 'Minecraft' windowunmap 2>>"$helper_log" || true
				xdotool search --name 'Prism Launcher' windowunmap 2>>"$helper_log" || true
				for wid in $(xdotool search --name '^Fallout: New Vegas$' 2>/dev/null); do
					xdotool windowmap "$wid" windowactivate "$wid" windowraise "$wid" 2>>"$helper_log" || true
				done
			fi
		done
	) &
	FOCUS_PID=$!
	log "FNV focus helper pid=$FOCUS_PID (windowed)"
}
FOCUS_PID=0
start_fnv_focus_helper

export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:+$WINEDLLOVERRIDES;}dinput8.dll=n,b;nvse_steam_loader.dll=n,b;nvse_1_4.dll=n,b"
export STEAM_COMPAT_DATA_PATH="${STEAM_COMPAT_DATA_PATH:-$COMPAT}"
export STEAM_COMPAT_CLIENT_INSTALL_PATH="${STEAM_COMPAT_CLIENT_INSTALL_PATH:-$HOME/.local/share/Steam}"
# Needed so this single session can run 64-bit Prism + 32-bit FNV.
export PROTON_USE_WOW64="${PROTON_USE_WOW64:-1}"

patch_java_security_workaround() {
	local inst="$PRISM_DIR/Prism/instances/VegasCraft"
	local sec_dst="$inst/.minecraft/java.security.proton"
	local cfg repo_sec
	if [[ ! -f "$sec_dst" ]]; then
		repo_sec="${VEGASCRAFT_REPO:-$HOME/VegasCraft}/tools/minecraft-bundle/Prism/instances/VegasCraft/.minecraft/java.security.proton"
		if [[ -f "$repo_sec" ]]; then
			mkdir -p "$(dirname "$sec_dst")"
			cp -f "$repo_sec" "$sec_dst"
		fi
	fi
	cfg="$inst/instance.cfg"
	[[ -f "$cfg" ]] || return 0
	python3 - "$cfg" <<'PY'
import pathlib, re, sys
p = pathlib.Path(sys.argv[1])
t = p.read_text()
need = [
    "-Djava.security.properties=java.security.proton",
    "-Dio.netty.machineId=02:00:00:00:00:01",
    "-Dio.netty.processId=1",
]
def inject(m):
    args = m.group(1).strip().strip('"')
    changed = False
    for prop in need:
        key = prop.split("=", 1)[0]
        if key not in args:
            if "--enable-native-access=ALL-UNNAMED" in args:
                args = args.replace(
                    "--enable-native-access=ALL-UNNAMED",
                    f"--enable-native-access=ALL-UNNAMED {prop}",
                    1,
                )
            else:
                args = f"{prop} {args}"
            changed = True
    if not changed:
        return m.group(0)
    return f'JvmArgs="{args}"'
t2, n = re.subn(r'^JvmArgs="?(.*?)"?\s*$', inject, t, count=1, flags=re.M)
if n == 1 and t2 != t:
    p.write_text(t2)
    print("patched")
PY
	if [[ $? -eq 0 ]]; then
		log "Ensured Proton Java/Netty NetworkInterface workarounds in instance.cfg"
	fi
}

ensure_prism_unpacked() {
	if [[ ! -f "$BUNDLE" ]]; then
		return 1
	fi
	if [[ -f "$PRISM_EXE" ]]; then
		patch_java_security_workaround || true
		return 0
	fi
	log "Pre-unpacking Minecraft bundle into prefix LocalAppData..."
	mkdir -p "$PRISM_DIR"
	if ! unzip -q -o "$BUNDLE" -d "$PRISM_DIR"; then
		log "WARNING: unzip failed"
		return 1
	fi
	if [[ -f "$PRISM_DIR/defaults/prismlauncher.cfg" && ! -f "$PRISM_DIR/Prism/prismlauncher.cfg" ]]; then
		cp -f "$PRISM_DIR/defaults/prismlauncher.cfg" "$PRISM_DIR/Prism/prismlauncher.cfg"
	fi
	stat -c '%s' "$BUNDLE" >"$PRISM_DIR/bundle.stamp"
	log "Unpacked Prism to $PRISM_DIR"
	patch_java_security_workaround || true
	return 0
}

stop_vegas_minecraft() {
	log "Stopping leftover VegasCraft Prism/Minecraft"
	pkill -f 'VegasCraft.*prismlauncher\.exe' 2>/dev/null || true
	pkill -f 'VegasCraft.*javaw\.exe' 2>/dev/null || true
}

cleanup_on_exit() {
	kill "${FOCUS_PID:-0}" 2>/dev/null || true
	stop_vegas_minecraft
}
trap 'cleanup_on_exit' EXIT INT TERM

ensure_prism_unpacked || true

cmd=("$@")
for i in "${!cmd[@]}"; do
	case "${cmd[$i]}" in
		*FalloutNVLauncher.exe|*FalloutNV.exe)
			if [[ -f "$BOOT_CMD" ]]; then
				cmd[$i]="$BOOT_CMD"
				log "Using unified boot: $BOOT_CMD"
			else
				cmd[$i]="$GAME"
			fi
			;;
	esac
done

log "run: ${cmd[*]}"
set +e
"${cmd[@]}"
rc=$?
set -e
log "Steam/Proton command exited rc=$rc"
stop_vegas_minecraft
trap - EXIT INT TERM
exit "$rc"
