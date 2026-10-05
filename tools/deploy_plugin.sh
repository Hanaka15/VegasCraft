#!/usr/bin/env bash
# Deploy VegasCraft plugin + ini into the local FNV Data/NVSE/Plugins folder.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FNV="${VEGASCRAFT_FNV_DIR:-$HOME/.local/share/Steam/steamapps/common/Fallout New Vegas enplczru}"
DEST="$FNV/Data/NVSE/Plugins"
DLL_CANDIDATES=(
  "$ROOT/nvse/build/Release/vegascraft.dll"
  "$ROOT/nvse/build/RelWithDebInfo/vegascraft.dll"
  "$ROOT/nvse/build/Debug/vegascraft.dll"
)

mkdir -p "$DEST"
cp -v "$ROOT/nvse/VegasCraft.ini" "$DEST/"

deployed=0
for dll in "${DLL_CANDIDATES[@]}"; do
  if [[ -f "$dll" ]]; then
    cp -v "$dll" "$DEST/vegascraft.dll"
    deployed=1
    break
  fi
done

if [[ "$deployed" -eq 0 ]]; then
  echo "No vegascraft.dll built yet. On Windows:"
  echo "  cd nvse && cmake --preset default && cmake --build --preset release"
  echo "Then re-run this script (or copy the DLL manually to:"
  echo "  $DEST"
else
  echo "Deployed to $DEST"
fi

echo "Launch FNV with: $FNV/nvse_loader.exe (or Steam + nvse_steam_loader.dll)"
ls -la "$DEST"
