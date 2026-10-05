# Proton notes

Fallout New Vegas is 32-bit. VegasCraft still requires **Windows Prism + Windows Java**
inside **the same Proton wineprefix** as `FalloutNV.exe`, because the bridge is
`Local\VegasCraft_v1` shared memory (not a network socket).

How that happens:

1. Install the mod zip so `Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip` exists.
2. Launch FNV via Steam + `tools/proton_nvse_launch.sh` (starts `FalloutNV.exe`).
   The script installs Ultimate ASI Loader as `dinput8.dll` + `nvse_steam_loader.asi`
   so xNVSE actually loads under Proton, pre-unpacks the Minecraft bundle, then
   starts Windows Prism with `proton run` **only after** `FalloutNV.exe` is alive
   (avoids the wineserver race that blocks the FNV window).
3. `vegascraft.dll` also tries `CreateProcess` on Prism in-prefix when NVSE loads.

Do **not** start Linux Prism. Do **not** set `PROTON_USE_WOW64=1` on the FNV
process itself (breaks 32-bit NVSE); the script enables WOW64 only for the Prism
child.

`PROTON_USE_WOW64=1` is set by the launch script so a 32-bit prefix can still run
64-bit Prism/Java when the Proton build supports it.

## Java / Wine

Minecraft 26’s Windows Java 25 calls `NetworkInterface.getAll()` while seeding
`SecureRandom`. That crashes the JVM on **Wine &lt; 9.3** (including **Proton 9.0**,
which ships Wine 9.0) with `Internal Error (0xc06d007f)` in `kernelbase.dll`.
The affinity warnings are harmless.

VegasCraft’s instance sets `-Djava.security.properties=java.security.proton` so
MSCAPI `Windows-PRNG` is used instead (avoids that call at Mixin init).

Still prefer **GE-Proton 10+** or **Proton Experimental** (Wine ≥ 9.3) for FNV when
you can — later networking code may touch `NetworkInterface` for real.
