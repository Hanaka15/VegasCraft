#!/usr/bin/env bash
# Steam launch option for Fallout New Vegas (Proton) + xNVSE + VegasCraft:
#   bash /home/hanaka/VegasCraft/tools/proton_nvse_launch.sh %command%
#
# Steam must start FalloutNV.exe (not nvse_loader.exe) or you get P:0000065432.
# xNVSE loads via nvse_steam_loader.dll.
#
# Do NOT start Prism with a second `proton run` here — that races FNV's
# waitforexitandrun session and can leave FalloutNV.exe never appearing while
# Minecraft flashes. Windows Prism must be CreateProcess'd by vegascraft.dll
# inside the same wineprefix/wineserver as the game.
set -euo pipefail

FNV="${VEGASCRAFT_FNV:-$HOME/.local/share/Steam/steamapps/common/Fallout New Vegas enplczru}"
GAME="$FNV/FalloutNV.exe"
STEAM_LOADER="$FNV/nvse_steam_loader.dll"
NVSE_DLL="$FNV/nvse_1_4.dll"
PLUGIN="$FNV/Data/NVSE/Plugins/vegascraft.dll"
BUNDLE="$FNV/Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip"
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

if [[ ! -f "$STEAM_LOADER" || ! -f "$NVSE_DLL" ]]; then
	log "WARNING: xNVSE incomplete (need nvse_steam_loader.dll + nvse_1_4.dll)"
fi

if [[ ! -f "$PLUGIN" ]]; then
	log "WARNING: missing $PLUGIN — install the CI/mod zip"
fi

if [[ ! -f "$BUNDLE" ]]; then
	log "WARNING: missing $BUNDLE — Prism will not auto-start until the Minecraft bundle is installed"
else
	log "Found Minecraft bundle: $BUNDLE"
fi

export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:+$WINEDLLOVERRIDES;}nvse_steam_loader.dll=n,b"
# Help 32-bit FNV prefixes launch 64-bit Prism/Java when the Proton build supports it.
export PROTON_USE_WOW64="${PROTON_USE_WOW64:-1}"
export STEAM_COMPAT_DATA_PATH="${STEAM_COMPAT_DATA_PATH:-$COMPAT}"
export STEAM_COMPAT_CLIENT_INSTALL_PATH="${STEAM_COMPAT_CLIENT_INSTALL_PATH:-$HOME/.local/share/Steam}"

patch_java_security_workaround() {
	# Wine < 9.3 (Proton 9): Java 25 SecureRandom → NetworkInterface.getAll crash.
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
		log "Prism already present: $PRISM_EXE"
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

ensure_prism_unpacked || true

cmd=("$@")
for i in "${!cmd[@]}"; do
	case "${cmd[$i]}" in
		*FalloutNVLauncher.exe)
			cmd[$i]="$GAME"
			;;
	esac
done

log "exec: ${cmd[*]}"
log "Prism will be started in-prefix by vegascraft.dll (not a second proton run)"
exec "${cmd[@]}"
