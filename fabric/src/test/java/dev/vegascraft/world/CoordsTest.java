package dev.vegascraft.world;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;

class CoordsTest {
	private static final double EPS = 1e-9;

	@Test
	void fnvOriginMapsToMcOrigin() {
		assertArrayEquals(new double[] { 0, 0, 0 }, Coords.fnvToMc(0, 0, 0), EPS);
	}

	@Test
	void playerHeightAboutOnePointEightBlocks() {
		// FNV player ~128 units tall → 128/70 ≈ 1.828 blocks
		double[] mc = Coords.fnvToMc(0, 0, 128);
		assertEquals(128.0 / 70.0, mc[1], EPS);
	}

	@Test
	void roundTripPosition() {
		double[] mc = Coords.fnvToMc(2100, -700, 350);
		double[] fnv = Coords.mcToFnv(mc[0], mc[1], mc[2]);
		assertArrayEquals(new double[] { 2100, -700, 350 }, fnv, EPS);
	}

	@Test
	void northAxesAgree() {
		// FNV +Y north becomes MC -Z
		double[] mc = Coords.fnvToMc(0, 70, 0);
		assertEquals(0, mc[0], EPS);
		assertEquals(0, mc[1], EPS);
		assertEquals(-1, mc[2], EPS);
	}

	@Test
	void yawPitchInvert() {
		assertEquals(0f, Coords.mcYawToFnv(Coords.fnvYawToMc(0f)), 1e-4f);
		assertEquals(45f, Coords.mcYawToFnv(Coords.fnvYawToMc(45f)), 1e-4f);
		assertEquals(10f, Coords.mcPitchToFnv(Coords.fnvPitchToMc(10f)), 1e-4f);
	}
}
