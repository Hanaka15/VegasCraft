#!/usr/bin/env bash
# Copy xNVSE release files into the Fallout New Vegas game root.
set -euo pipefail
FNV="${1:-$HOME/.local/share/Steam/steamapps/common/Fallout New Vegas enplczru}"
SRC="${2:-}"

if [[ -z "$SRC" ]]; then
  # Prefer an already-extracted folder next to the game, else fail with help.
  for d in "$FNV"/New\ Vegas\ Script\ Extender*; do
    if [[ -d "$d" && -f "$d/nvse_loader.exe" ]]; then
      SRC="$d"
      break
    fi
  done
fi

if [[ -z "$SRC" || ! -f "$SRC/nvse_loader.exe" ]]; then
  echo "Usage: $0 [FNV_DIR] [xNVSE_EXTRACT_DIR]"
  echo "Could not find nvse_loader.exe under an xNVSE folder."
  exit 1
fi

cp -v "$SRC"/nvse_1_4.dll "$FNV/"
cp -v "$SRC"/nvse_loader.exe "$FNV/"
cp -v "$SRC"/nvse_steam_loader.dll "$FNV/"
[[ -f "$SRC/nvse_editor_1_4.dll" ]] && cp -v "$SRC/nvse_editor_1_4.dll" "$FNV/"
mkdir -p "$FNV/Data/NVSE/Plugins"
[[ -f "$SRC/Data/NVSE/nvse_config.ini" ]] && cp -v "$SRC/Data/NVSE/nvse_config.ini" "$FNV/Data/NVSE/"
echo "xNVSE installed into $FNV"
ls -la "$FNV"/nvse_*.dll "$FNV"/nvse_loader.exe
