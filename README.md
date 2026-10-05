# VegasCraft

Play Fallout New Vegas as a Minecraft player. You move with Minecraft's physics, carry
Minecraft's inventory and HUD, and place and break blocks in the Mojave. You fight FNV NPCs
with Minecraft weapons, and they fight back.

Neither game is rewritten. Minecraft runs its own game logic; New Vegas runs its world, NPCs,
quests and saves. An **xNVSE** plugin and a Minecraft **Fabric** mod talk through shared memory
(`Local\VegasCraft_v1`). Minecraft runs hidden; New Vegas draws everything.

> **Status: early scaffold.** Architecture and Phase 0–6 module stubs are in place. Expect rough
> edges; back up your saves. Fan project — not affiliated with Bethesda, Mojang, Microsoft, or
> ZeniMax. You need to own both games.
>
> Architecture adapted from [SkyCraft](https://github.com/chasmlol/SkyCraft) (MIT).

## What works (target parity)

Same feature set as SkyCraft, mapped to FNV: movement, blocks, digging Mojave terrain, water/lava,
combat with scaled damage, skill XP hooks, F5 camera, death, Activate handoff (doors/dialogue),
MC-side LAN multiplayer.

See [docs/DESIGN.md](docs/DESIGN.md) for the phased plan (0 Link → 6 Polish).

## Requirements

**Fallout New Vegas (Windows)**

| | |
|---|---|
| Fallout New Vegas (Steam) | Windows-native. Linux/Proton not supported for v1. |
| [xNVSE](https://github.com/xNVSE/NVSE) | Matching your game executable |
| Mod Organizer 2 (recommended) | Deploy `Data/NVSE/Plugins/vegascraft.dll` |

**Minecraft**

A Microsoft account that owns **Minecraft: Java Edition**. Target: Minecraft **26.3**, Fabric,
Fabric API, Java **25** (same pin as SkyCraft).


## xNVSE on this machine

xNVSE **6.4.9** belongs in the Fallout New Vegas **game root** (next to `FalloutNV.exe`), not in a nested Nexus folder:

```bash
tools/install_xnvse.sh
# or manually copy nvse_1_4.dll, nvse_loader.exe, nvse_steam_loader.dll into the FNV folder
```

Plugins load from `Data/NVSE/Plugins/`. After a Windows build:

```bash
tools/deploy_plugin.sh
```

Start the game with `nvse_loader.exe`, or Steam with `nvse_steam_loader.dll` present (Steam overlay path).

## Building

### GitHub Actions (recommended)

Push to GitHub; the [Build](.github/workflows/build.yml) workflow produces:

- **`vegascraft-nvse-win32`** — `vegascraft.dll` + `VegasCraft.ini` (Win32 / MSVC)
- **`vegascraft-fabric`** — Fabric mod jar

Download the NVSE artifact and copy into your game:

```
Fallout New Vegas/Data/NVSE/Plugins/vegascraft.dll
Fallout New Vegas/Data/NVSE/Plugins/VegasCraft.ini
```

### Local — Fabric mod

```bash
cd fabric
./gradlew build
```

Needs JDK 25.

### Local — xNVSE plugin (Windows, Win32)

```bat
cd nvse
cmake --preset default
cmake --build --preset release
```

Requires Visual Studio (C++), CMake 3.25+.

### Protocol stand-in (no FNV)

```bash
# Windows Python with mmap tagname, or tools that open Local\VegasCraft_v1
python tools/fake_fnv.py 30
```

### Coordinate unit tests

```bash
cd fabric && ./gradlew test --tests dev.vegascraft.world.CoordsTest
```

## Controls (planned)

Minecraft has priority. FNV keep: Activate (default **E** remapped / **G**), **Esc**, Pip-Boy,
map, wait, console. **O** opens Minecraft pause.

## License

MIT — see [LICENSE](LICENSE). SkyCraft architecture and much of the Fabric/protocol design
are from [chasmlol/SkyCraft](https://github.com/chasmlol/SkyCraft).
