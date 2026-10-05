# VegasCraft phase checklist

Implementation lives under `nvse/` (host) and `fabric/` (Minecraft). Game RE hooks are stubbed
until resolved against `FalloutNV.exe` on Windows.

| Phase | Host modules | Fabric | Status |
|---|---|---|---|
| 0 Link | `Link`, `PlayerPuppet`, `Coords` | `VegasLink`, `Proto`, `Coords` | Scaffold + layout/coord tests |
| 1 Walk | `WorldExporter`, `CameraDriver`, `InputBridge` | `SkyCollision` / mixins (from SkyCraft) | Scaffold |
| 2 Overlay | `Compositor` (D3D9 CPU blit) | `FrameExporter` | Scaffold |
| 3 Combat | `ActorMirror`, `Combat` | `SkyCombat`, actor proxies | Scaffold |
| 4 Blocks | `NpcBlocks` | world render ring / solids | Scaffold |
| 5 World | `WorldContext` (water, Activate) | `SkyWater` | Scaffold |
| 6 Polish | `Dig`, `Skills`, `Launcher` | dig client, Discord, e4mc LAN | Scaffold |

Run `python3 tools/test_coords.py` after protocol changes.
