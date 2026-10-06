# Proton notes

Fallout New Vegas is 32-bit. VegasCraft still requires **Windows Prism + Windows Java**
inside **the same Proton wineprefix** as `FalloutNV.exe`, because the bridge is
`Local\VegasCraft_v1` shared memory (not a network socket).

How that happens:

1. Install the mod zip so `Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip` exists.
2. Launch via Steam + `tools/proton_nvse_launch.sh`:
   - installs ASI Loader (`dinput8.dll` + `nvse_steam_loader.asi`) for xNVSE
   - pre-unpacks Prism into the prefix
   - runs **one** Proton session on `VegasCraft_boot.cmd`, which starts Prism,
     waits for `javaw` with Wine-safe `ping` delays (not `timeout`), then
     starts `FalloutNV.exe` (MC first for focus; same wineserver)
   - sets `bStartWithHost=0` so the plugin does not launch a second Prism
3. Never use a second Linux `proton run` for Prism — Proton then hangs on
   `wineserver -w` and Fallout never starts.

`PROTON_USE_WOW64=1` is set for that single session so 64-bit Prism and 32-bit
FNV can coexist.

`PROTON_USE_WOW64=1` is set by the launch script so a 32-bit prefix can still run
64-bit Prism/Java when the Proton build supports it.

## Java / Wine

Minecraft 26’s Windows Java 25 crashes the JVM on **Wine &lt; 9.3** (including
**Proton 9.0** / Wine 9.0) with `Internal Error (0xc06d007f)` in `kernelbase.dll`
when anything calls `NetworkInterface.getAll()` (missing `iphlpapi` entry point).
The affinity warnings are harmless.

VegasCraft applies two workarounds:

1. `-Djava.security.properties=java.security.proton` — MSCAPI `Windows-PRNG`
   so early `SecureRandom` / UUID seeding does not touch NICs.
2. `-Dio.netty.machineId=02:00:00:00:00:01 -Dio.netty.processId=1` (also set
   from the Fabric mod) — Netty skips NIC enumeration when the integrated
   server opens its memory channel (mirror-world load).

Still prefer **GE-Proton 10+** or **Proton Experimental** (Wine ≥ 9.3) for FNV when
you can — other code paths may still call `NetworkInterface`.

## Display

Use **exclusive fullscreen** (`bFull Screen=1`) for acceptable input lag. The launch
script also sets `iPresentInterval=0` (no vsync) — windowed/borderless with vsync
on Proton feels like severe input delay.

`VegasCraft_boot.cmd` starts Prism **minimized**, then Fallout with `/wait`, so FNV
can take exclusive fullscreen after the launcher. Minecraft itself stays hidden
(`-Dvegascraft.startHidden=true`).
