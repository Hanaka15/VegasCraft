# VegasCraft — Design Doc

> Play Fallout New Vegas as the main game while *being* a Minecraft player: real Minecraft
> movement physics, inventory, items, block placing, and combat, inside the real Mojave,
> able to fight and talk to FNV NPCs.

Status: draft v0.1 · adapted from [SkyCraft](https://github.com/chasmlol/SkyCraft) DESIGN.md

---

## 1. Core principle

**Neither game is rewritten.** Minecraft runs its own game logic. Fallout New Vegas runs its
world: terrain, buildings, NPCs, AI, quests, dialogue, saves.

The two mods only **translate** between them:

- FNV tells Minecraft *what the world is shaped like* and *where the NPCs are*.
- Minecraft tells FNV *where the player is*, *what the player hit*, and *what to draw on top*.

## 2. Target environment

| Thing | Value | Notes |
|---|---|---|
| Fallout New Vegas | Steam Windows `FalloutNV.exe` (32-bit) | Not VR; TTW later |
| Script extender | [xNVSE](https://github.com/xNVSE/NVSE) **6.4.9+** | Plugin DLL via `nvse_loader.exe` / steam loader |
| Mod manager | MO2 or Vortex | Install like any NVSE plugin |
| Minecraft | **26.3 + Fabric** | Same pin as SkyCraft |
| Java | 25 | |
| C++ toolchain | Visual Studio, CMake 3.25+, Win32 | Plugin is **32-bit** |

Host hooks use xNVSE messaging: `kMessage_MainGameLoop` (physics/IPC) and
`kMessage_OnFramePresent` (D3D9 overlay).

## 3. Components

```
┌──────────────── FalloutNV.exe (32-bit) ────────────────┐   ┌──────── javaw.exe (Minecraft 26.3) ────────┐
│  vegascraft.dll  (xNVSE plugin)                         │   │  vegascraft (Fabric mod)                   │
│  WorldExporter / ActorMirror / InputBridge / HitBridge  │◀─▶│  CollisionField / ActorProxy / input       │
│  PlayerPuppet / CameraDriver / DamageApplier            │   │  LocalPlayer physics / combat              │
│  Compositor (D3D9 CPU blit → GPU later)                 │   │  offscreen world / hand / GUI layers       │
└─────────────────────────────────────────────────────────┘   └────────────────────────────────────────────┘
                    shared memory Local\VegasCraft_v1 + named events
```

## 4. Coordinate mapping

FNV is Z-up (Gamebryo). Minecraft is Y-up. Starting scale: **1 block = 70 FNV units**
(player capsule ~128 units ≈ 1.8 blocks), then measure in Phase 0.

```
mc.x =  fnv.x / 70
mc.y =  fnv.z / 70
mc.z = -fnv.y / 70
```

- **WastelandNV** → MC dimension `vegascraft:wasteland`
- **Interiors** → `vegascraft:interiors` with FormID region slots (1024×1024)

## 5–11. Subsystems

Same roles as SkyCraft: CollisionField (raycast MVP → Havok later), PlayerPuppet,
InputBridge (Pip-Boy / Esc / Activate allow-list), ActorProxy combat, overlay triple-buffer,
save snapshots keyed by FNV save id.

**Graphics difference:** FNV is **D3D9**. Phase 2 ships a CPU RGBA overlay blit into the
backbuffer after Present. D3D9Ex / shared-surface GPU interop is a stretch goal.

**Damage scaling (default):** MC→FNV `× (5 + 0.25 × NPC level)`; FNV→MC `÷ 5`.

**FNV skills (Phase 6):** melee→Melee Weapons, ranged→Guns, craft→Repair, sneak→Sneak,
armor hits→armor skill AV.

## 12. Phased plan

| # | Phase | Done when |
|---|---|---|
| 0 | Link | SHM handshake; coord tests; MC walk moves FNV puppet |
| 1 | Walk | Raycast CollisionField + CameraDriver + InputBridge |
| 2 | Overlay | Hand/GUI CPU-composited on D3D9 |
| 3 | Combat | ActorProxy, hits, death |
| 4 | Blocks | Place/break + NPC collision |
| 5 | Full world | Interiors, water, Activate arbitration |
| 6 | Polish | Dig, saves, weather/time, skills, Prism launch, LAN |

## 13. Repo layout

```
VegasCraft/
  docs/DESIGN.md
  protocol/                 vegascraft_protocol.h
  nvse/                     xNVSE plugin (Win32)
  fabric/                   Fabric mod
  tools/                    fake_fnv.py, package.ps1, …
```

## 14. Risks

| Risk | Mitigation |
|---|---|
| D3D9 compositing | CPU overlay first |
| Havok shape dump | Raycast field through Phase 4 |
| Goodsprings intro scripts | Start from a post-doc save |
| 32-bit address space | Cap overlay / ring sizes; MC is a separate 64-bit process |
