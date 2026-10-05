#!/usr/bin/env bash
# Steam launch option for Fallout New Vegas (Proton) + xNVSE + VegasCraft:
#   bash /home/hanaka/VegasCraft/tools/proton_nvse_launch.sh %command%
#
# Order under Proton (keeps FNV focus):
#   1) Windows Prism / Minecraft in the FNV wineprefix
#   2) FalloutNV.exe once javaw is up
#
# Starting Prism *after* FNV steals focus and is hard to alt-tab back from.
# Starting Prism during FNV wineserver init can block FalloutNV.exe entirely —
# so Prism goes first and we wait for it before exec'ing the game.
set -euo pipefail

FNV="${VEGASCRAFT_FNV:-$HOME/.local/share/Steam/steamapps/common/Fallout New Vegas enplczru}"
GAME="$FNV/FalloutNV.exe"
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
	log "WARNING: xNVSE incomplete"
fi
if [[ ! -f "$ASI_LOADER" || ! -f "$ASI_NVSE" ]]; then
	log "WARNING: missing dinput8.dll / nvse_steam_loader.asi — NVSE may not load under Proton"
fi
if [[ ! -f "$PLUGIN" ]]; then
	log "WARNING: missing $PLUGIN"
fi
if [[ ! -f "$BUNDLE" ]]; then
	log "WARNING: missing $BUNDLE"
else
	log "Found Minecraft bundle: $BUNDLE"
fi

# Plugin must not CreateProcess Prism again (would steal focus after FNV is up).
if [[ -f "$PLUGIN_INI" ]]; then
	sed -i 's/^bStartWithHost\s*=\s*1/bStartWithHost = 0/' "$PLUGIN_INI" || true
	log "Set bStartWithHost=0 (Prism owned by this launch script)"
fi

# Native NVSE + ASI dinput8. No PROTON_USE_WOW64 on FNV itself.
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

mc_running() {
	pgrep -f 'VegasCraft/Prism/.*javaw\.exe' >/dev/null 2>&1 \
		|| pgrep -f 'VegasCraft/Prism/prismlauncher\.exe' >/dev/null 2>&1
}

start_minecraft_first() {
	ensure_prism_unpacked || return 0
	[[ -f "$PRISM_EXE" ]] || {
		log "WARNING: no prismlauncher.exe"
		return 0
	}
	local proton
	proton="$(find_proton)" || {
		log "WARNING: no proton — cannot start Prism before FNV"
		return 0
	}

	if mc_running; then
		log "Minecraft already running"
		return 0
	fi

	log "Starting Windows Prism first (proton=$proton)"
	(
		export STEAM_COMPAT_DATA_PATH="$COMPAT"
		export STEAM_COMPAT_CLIENT_INSTALL_PATH="${STEAM_COMPAT_CLIENT_INSTALL_PATH:-$HOME/.local/share/Steam}"
		export PROTON_USE_WOW64=1
		"$proton" run "$PRISM_EXE" --launch VegasCraft >>"$LOG" 2>&1 || log "WARNING: proton Prism exit $?"
	) &
	disown || true

	# Wait until javaw is up so wineserver is settled before FNV joins.
	local i
	for i in $(seq 1 120); do
		if pgrep -f 'VegasCraft/Prism/.*javaw\.exe' >/dev/null 2>&1; then
			log "Minecraft javaw up after ${i}s — starting Fallout"
			# Brief settle so MC finishes grabbing input before FNV takes focus.
			sleep 2
			return 0
		fi
		sleep 1
	done
	log "WARNING: javaw not seen in 120s — starting Fallout anyway"
	return 0
}

start_minecraft_first || true

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
