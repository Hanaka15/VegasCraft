#!/usr/bin/env bash
# Steam launch option for Fallout New Vegas (Proton) + xNVSE + VegasCraft:
#   bash /home/hanaka/VegasCraft/tools/proton_nvse_launch.sh %command%
#
# Steam must start FalloutNV.exe (not nvse_loader.exe) or you get P:0000065432.
#
# NVSE: Proton does not always inject nvse_steam_loader.dll the way Windows Steam
# does. We ship Ultimate ASI Loader as dinput8.dll + nvse_steam_loader.asi so
# NVSE/vegascraft load inside FalloutNV.exe.
#
# Prism: start Windows Prism via proton only AFTER FalloutNV.exe is alive, with
# PROTON_USE_WOW64 on that child only. Do not start Prism during FNV wineserver
# init (that races and blocks the game window).
set -euo pipefail

FNV="${VEGASCRAFT_FNV:-$HOME/.local/share/Steam/steamapps/common/Fallout New Vegas enplczru}"
GAME="$FNV/FalloutNV.exe"
STEAM_LOADER="$FNV/nvse_steam_loader.dll"
NVSE_DLL="$FNV/nvse_1_4.dll"
ASI_LOADER="$FNV/dinput8.dll"
ASI_NVSE="$FNV/nvse_steam_loader.asi"
PLUGIN="$FNV/Data/NVSE/Plugins/vegascraft.dll"
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

# Ensure ASI chainload for NVSE under Proton
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

if [[ ! -f "$STEAM_LOADER" || ! -f "$NVSE_DLL" ]]; then
	log "WARNING: xNVSE incomplete (need nvse_steam_loader.dll + nvse_1_4.dll)"
fi
if [[ ! -f "$ASI_LOADER" || ! -f "$ASI_NVSE" ]]; then
	log "WARNING: missing dinput8.dll / nvse_steam_loader.asi — NVSE may not load under Proton"
fi
if [[ ! -f "$PLUGIN" ]]; then
	log "WARNING: missing $PLUGIN — install the CI/mod zip"
fi
if [[ ! -f "$BUNDLE" ]]; then
	log "WARNING: missing $BUNDLE"
else
	log "Found Minecraft bundle: $BUNDLE"
fi

# Native NVSE + ASI dinput8. Do NOT set PROTON_USE_WOW64 on the FNV process —
# it can break 32-bit Steam/NVSE injection. WOW64 is enabled only for Prism.
export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:+$WINEDLLOVERRIDES;}dinput8.dll=n,b;nvse_steam_loader.dll=n,b;nvse_1_4.dll=n,b"
export STEAM_COMPAT_DATA_PATH="${STEAM_COMPAT_DATA_PATH:-$COMPAT}"
export STEAM_COMPAT_CLIENT_INSTALL_PATH="${STEAM_COMPAT_CLIENT_INSTALL_PATH:-$HOME/.local/share/Steam}"
unset PROTON_USE_WOW64 || true

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
	if rg -q 'java\.security\.properties=java\.security\.proton' "$cfg"; then
		return 0
	fi
	python3 - "$cfg" <<'PY'
import pathlib, re, sys
p = pathlib.Path(sys.argv[1])
t = p.read_text()
def inject(m):
    args = m.group(1)
    if "java.security.properties" in args:
        return m.group(0)
    needle = "--enable-native-access=ALL-UNNAMED"
    prop = "-Djava.security.properties=java.security.proton"
    if needle in args:
        args = args.replace(needle, f"{needle} {prop}", 1)
    else:
        args = f"{prop} {args}"
    return f'JvmArgs="{args}"'
t2, n = re.subn(r'^JvmArgs="?(.*?)"?\s*$', inject, t, count=1, flags=re.M)
if n == 1:
    p.write_text(t2)
PY
	log "Patched instance.cfg for Proton Java SecureRandom workaround"
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

find_proton() {
	if [[ -n "${VEGASCRAFT_PROTON:-}" && -x "${VEGASCRAFT_PROTON}" ]]; then
		printf '%s\n' "$VEGASCRAFT_PROTON"
		return 0
	fi
	local p
	for p in \
		"$HOME/.local/share/Steam/steamapps/common/Proton 9.0 (Beta)/proton" \
		"$HOME/.local/share/Steam/steamapps/common/Proton 9.0/proton" \
		"$HOME/.local/share/Steam/steamapps/common/Proton - Experimental/proton" \
		"$HOME/.local/share/Steam/compatibilitytools.d/GE-Proton10-34/proton" \
		"$HOME/.local/share/Steam/compatibilitytools.d/"*/proton
	do
		if [[ -x "$p" ]]; then
			printf '%s\n' "$p"
			return 0
		fi
	done
	return 1
}

start_windows_prism_when_fnv_ready() {
	ensure_prism_unpacked || return 0
	[[ -f "$PRISM_EXE" ]] || return 0
	local proton
	proton="$(find_proton)" || {
		log "WARNING: no proton for Prism fallback"
		return 0
	}
	log "Will start Windows Prism after FalloutNV.exe is running (proton=$proton)"
	(
		# Wait for the real game process (not reaper/STL wrappers).
		for _ in $(seq 1 90); do
			if pgrep -f 'Fallout New Vegas.*/FalloutNV\.exe' >/dev/null 2>&1 \
				|| pgrep -f 'Z:.*FalloutNV\.exe' >/dev/null 2>&1; then
				break
			fi
			sleep 1
		done
		if ! pgrep -f 'FalloutNV\.exe' >/dev/null 2>&1; then
			log "WARNING: FalloutNV.exe not seen — not starting Prism from script"
			exit 0
		fi
		# Let NVSE + first frames settle; avoid wineserver startup race.
		sleep 8
		if pgrep -f 'VegasCraft/Prism/.*javaw\.exe' >/dev/null 2>&1 \
			|| pgrep -f 'VegasCraft/Prism/prismlauncher\.exe' >/dev/null 2>&1; then
			log "Prism/Minecraft already running — skip script start"
			exit 0
		fi
		log "Starting Windows Prism in-prefix (PROTON_USE_WOW64=1)"
		export STEAM_COMPAT_DATA_PATH="$COMPAT"
		export STEAM_COMPAT_CLIENT_INSTALL_PATH="${STEAM_COMPAT_CLIENT_INSTALL_PATH:-$HOME/.local/share/Steam}"
		export PROTON_USE_WOW64=1
		"$proton" run "$PRISM_EXE" --launch VegasCraft >>"$LOG" 2>&1 || log "WARNING: proton Prism exit $?"
	) &
	disown || true
}

ensure_prism_unpacked || true
start_windows_prism_when_fnv_ready || true

cmd=("$@")
for i in "${!cmd[@]}"; do
	case "${cmd[$i]}" in
		*FalloutNVLauncher.exe)
			cmd[$i]="$GAME"
			;;
	esac
done

log "exec: ${cmd[*]}"
exec "${cmd[@]}"
