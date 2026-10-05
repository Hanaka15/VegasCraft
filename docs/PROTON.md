# Proton notes

Fallout New Vegas is 32-bit. VegasCraft still requires **Windows Prism + Windows Java**
inside **the same Proton wineprefix** as `FalloutNV.exe`, because the bridge is
`Local\VegasCraft_v1` shared memory (not a network socket).

How that happens:

1. Install the mod zip so `Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip` exists.
2. Launch FNV via Steam + `tools/proton_nvse_launch.sh` (starts `FalloutNV.exe`, loads xNVSE).
3. `vegascraft.dll` unpacks the zip to the prefix’s `%LOCALAPPDATA%\VegasCraft` and
   `CreateProcess`es `prismlauncher.exe --launch VegasCraft`.

Do **not** start Linux Prism from the launch script — that process cannot see Wine’s
`Local\` mappings.

`PROTON_USE_WOW64=1` is set by the launch script so a 32-bit prefix can still run
64-bit Prism/Java when the Proton build supports it. Prefer Proton Experimental / 9+.
