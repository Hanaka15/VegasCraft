#!/usr/bin/env bash
# Steam launch option for Fallout New Vegas (Proton) + xNVSE + VegasCraft:
#   bash /home/hanaka/VegasCraft/tools/proton_nvse_launch.sh %command%
#
# Steam must start FalloutNV.exe (not nvse_loader.exe) or you get P:0000065432.
# xNVSE loads via nvse_steam_loader.dll.
#
# Minecraft is Windows Prism inside FNV's Proton wineprefix (not Linux Prism),
# so Local\VegasCraft_v1 shared memory works. The NVSE plugin also CreateProcess-
# es Prism; this script pre-unpacks the bundle and starts Prism via proton as a
# Proton/WOW64 safety net when 32-bit CreateProcess of 64-bit Prism is flaky.
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
	if [[ -x "$PRISM_EXE" || -f "$PRISM_EXE" ]]; then
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

find_proton() {
	if [[ -n "${VEGASCRAFT_PROTON:-}" && -x "${VEGASCRAFT_PROTON}" ]]; then
		printf '%s\n' "$VEGASCRAFT_PROTON"
		return 0
	fi
	local stl_conf="$HOME/.config/steamtinkerlaunch/gamecfgs/id/${APPID}.conf"
	if [[ -f "$stl_conf" ]]; then
		local ver
		ver="$(rg -n '^USEPROTON=' "$stl_conf" | head -1 | cut -d= -f2- | tr -d '"')"
		if [[ -n "$ver" ]]; then
			# Resolve common Proton install dirs by folder name prefix
			local cand
			for cand in \
				"$HOME/.local/share/Steam/steamapps/common/Proton 9.0 (Beta)/proton" \
				"$HOME/.local/share/Steam/steamapps/common/Proton 9.0/proton" \
				"$HOME/.local/share/Steam/steamapps/common/Proton - Experimental/proton" \
				"$HOME/.local/share/Steam/compatibilitytools.d/"*/proton
			do
				if [[ -x "$cand" ]]; then
					printf '%s\n' "$cand"
					return 0
				fi
			done
		fi
	fi
	local p
	for p in \
		"$HOME/.local/share/Steam/steamapps/common/Proton 9.0 (Beta)/proton" \
		"$HOME/.local/share/Steam/steamapps/common/Proton 9.0/proton" \
		"$HOME/.local/share/Steam/steamapps/common/Proton - Experimental/proton"
	do
		if [[ -x "$p" ]]; then
			printf '%s\n' "$p"
			return 0
		fi
	done
	return 1
}

start_windows_prism() {
	ensure_prism_unpacked || return 0
	if [[ ! -f "$PRISM_EXE" ]]; then
		log "WARNING: no prismlauncher.exe after unpack"
		return 0
	fi
	local proton
	if ! proton="$(find_proton)"; then
		log "WARNING: could not find proton — relying on NVSE CreateProcess only"
		return 0
	fi
	log "Starting Windows Prism via: $proton"
	# Same compatdata/wineserver as FNV → Local\VegasCraft_v1 is shared.
	(
		# Give the wineserver / FNV a moment; plugin may also try CreateProcess.
		sleep 4
		if pgrep -f 'prismlauncher.exe' >/dev/null 2>&1; then
			log "prismlauncher.exe already running — skip script start"
			exit 0
		fi
		log "proton run Prism --launch VegasCraft"
		"$proton" run "$PRISM_EXE" --launch VegasCraft >>"$LOG" 2>&1 || log "WARNING: proton Prism exit $?"
	) &
	disown || true
	log "Windows Prism starter pid $!"
}

cmd=("$@")
for i in "${!cmd[@]}"; do
	case "${cmd[$i]}" in
		*FalloutNVLauncher.exe)
			cmd[$i]="$GAME"
			;;
	esac
done

start_windows_prism || true

log "exec: ${cmd[*]}"
exec "${cmd[@]}"
