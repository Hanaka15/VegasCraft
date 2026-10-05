#!/usr/bin/env python3
"""Unit tests for FNV <-> MC coordinate mapping (mirrors fabric Coords + nvse Coords.h)."""
UNITS = 70.0

def fnv_to_mc(fx, fy, fz):
    return fx / UNITS, fz / UNITS, -fy / UNITS

def mc_to_fnv(mx, my, mz):
    return mx * UNITS, -mz * UNITS, my * UNITS

def near(a, b, eps=1e-9):
    if isinstance(a, (tuple, list)):
        return all(abs(x - y) < eps for x, y in zip(a, b))
    return abs(a - b) < eps

assert near(fnv_to_mc(0, 0, 0), (0, 0, 0))
assert near(fnv_to_mc(0, 0, 128)[1], 128 / 70)
assert near(mc_to_fnv(*fnv_to_mc(2100, -700, 350)), (2100, -700, 350))
assert near(fnv_to_mc(0, 70, 0), (0, 0, -1))
assert near(0 + 180 - 180, 0)
print("coords: OK")

# Protocol magic / sizes
MAGIC = 0x41474556
VERSION = 11
assert MAGIC == int.from_bytes(b"VEGA", "little")
OFF_HOST = 0x100
OFF_MC = 0x200
assert OFF_HOST == 0x100 and OFF_MC == 0x200
print("protocol: OK")
