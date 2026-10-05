# Protocol layout + coordinate helpers shared by nvse and fabric.

## Files

- `vegascraft_protocol.h` — single source of truth (magic `VEGA` / `0x41474556`, mapping `Local\VegasCraft_v1`)
- Fabric mirror: `fabric/src/main/java/dev/vegascraft/link/Proto.java`
- Host creates the mapping; Minecraft opens it

## Layout tests

```bash
python3 tools/test_coords.py
```

When JDK 25 is available:

```bash
cd fabric && ./gradlew test --tests dev.vegascraft.world.CoordsTest
```
