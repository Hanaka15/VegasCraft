#!/usr/bin/env bash
# Steam launch option for Fallout New Vegas (Proton) + xNVSE:
#   bash /home/hanaka/VegasCraft/tools/proton_nvse_launch.sh %command%
#
# Steam must start FalloutNV.exe (not nvse_loader.exe) or you get P:0000065432.
# xNVSE loads via nvse_steam_loader.dll in the game folder.
#
# Minecraft / Prism is started by vegascraft.dll once that plugin is built and
# installed to Data/NVSE/Plugins/. This script cannot invent that DLL.
set -euo pipefail

FNV="${VEGASCRAFT_FNV:-$HOME/.local/share/Steam/steamapps/common/Fallout New Vegas enplczru}"
GAME="$FNV/FalloutNV.exe"
STEAM_LOADER="$FNV/nvse_steam_loader.dll"
NVSE_DLL="$FNV/nvse_1_4.dll"
PLUGIN="$FNV/Data/NVSE/Plugins/vegascraft.dll"
LOG="${XDG_RUNTIME_DIR:-/tmp}/vegascraft-launch.log"

log() { printf '%s\n' "$*" | tee -a "$LOG" >&2; }

: >"$LOG"
log "VegasCraft launch $(date -Iseconds)"

if [[ ! -f "$GAME" ]]; then
	log "ERROR: missing $GAME"
	exec "$@"
fi

if [[ ! -f "$STEAM_LOADER" || ! -f "$NVSE_DLL" ]]; then
	log "WARNING: xNVSE incomplete in $FNV (need nvse_steam_loader.dll + nvse_1_4.dll)"
fi

if [[ ! -f "$PLUGIN" ]]; then
	log "NOTE: vegascraft.dll is NOT installed at:"
	log "  $PLUGIN"
	log "Minecraft/Prism will NOT auto-start. The .ini alone does nothing."
	log "Build the Win32 NVSE plugin on Windows, then: tools/deploy_plugin.sh"
else
	log "Found plugin: $PLUGIN"
fi

# Optional: start Linux Prism for Fabric-only testing (shared memory will NOT
# connect to Proton FNV — Windows MC inside the prefix is required for that).
if [[ "${VEGASCRAFT_START_PRISM:-0}" == "1" ]] && command -v prismlauncher >/dev/null 2>&1; then
	INSTANCE="${VEGASCRAFT_PRISM_INSTANCE:-26.3}"
	log "VEGASCRAFT_START_PRISM=1 → launching Prism instance '$INSTANCE' (Linux; no SHM link)"
	prismlauncher --launch "$INSTANCE" >>"$LOG" 2>&1 &
	disown || true
fi

export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:+$WINEDLLOVERRIDES;}nvse_steam_loader.dll=n,b"

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
