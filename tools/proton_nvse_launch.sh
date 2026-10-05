#!/usr/bin/env bash
# Steam launch option for Fallout New Vegas (Proton) + xNVSE + VegasCraft:
#   bash /home/hanaka/VegasCraft/tools/proton_nvse_launch.sh %command%
#
# Steam must start FalloutNV.exe (not nvse_loader.exe) or you get P:0000065432.
# xNVSE loads via nvse_steam_loader.dll.
#
# Do NOT start Linux Prism here. Minecraft must be the Windows Prism the NVSE
# plugin unpacks/starts via CreateProcess so it shares FNV's Proton wineprefix
# (Local\VegasCraft_v1 shared memory). Bundle path inside the game:
#   Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip
set -euo pipefail

FNV="${VEGASCRAFT_FNV:-$HOME/.local/share/Steam/steamapps/common/Fallout New Vegas enplczru}"
GAME="$FNV/FalloutNV.exe"
STEAM_LOADER="$FNV/nvse_steam_loader.dll"
NVSE_DLL="$FNV/nvse_1_4.dll"
PLUGIN="$FNV/Data/NVSE/Plugins/vegascraft.dll"
BUNDLE="$FNV/Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip"
LOG="${XDG_RUNTIME_DIR:-/tmp}/vegascraft-launch.log"

log() { printf '%s\n' "$*" | tee -a "$LOG" >&2; }

: >"$LOG"
log "VegasCraft launch $(date -Iseconds)"

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
