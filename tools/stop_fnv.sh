#!/usr/bin/env bash
# Emergency stop for a stuck Fallout New Vegas + VegasCraft session.
set -euo pipefail
log() { printf '%s\n' "$*"; }
log "Stopping Fallout New Vegas / VegasCraft..."
pkill -f 'AppId=22490' 2>/dev/null || true
pkill -f 'FalloutNV\.exe' 2>/dev/null || true
pkill -f 'VegasCraft/Prism/prismlauncher\.exe' 2>/dev/null || true
pkill -f 'VegasCraft/Prism/.*/javaw\.exe' 2>/dev/null || true
pkill -f 'steamtinkerlaunch waitforexitandrun.*Fallout' 2>/dev/null || true
pkill -f 'compatdata/22490' 2>/dev/null || true
sleep 0.5
# force
pkill -9 -f 'AppId=22490' 2>/dev/null || true
pkill -9 -f 'FalloutNV\.exe' 2>/dev/null || true
pkill -9 -f 'VegasCraft/Prism' 2>/dev/null || true
pkill -9 -f 'compatdata/22490' 2>/dev/null || true
log "Done."
