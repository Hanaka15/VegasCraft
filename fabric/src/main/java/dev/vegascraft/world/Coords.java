package dev.vegascraft.world;

/**
 * Fallout New Vegas (Gamebryo Z-up) <-> Minecraft (Y-up) coordinate mapping.
 * 1 block = {@link #UNITS_PER_BLOCK} FNV units (player ~128 units ≈ 1.8 blocks).
 */
public final class Coords {
	public static final double UNITS_PER_BLOCK = 70.0;

	private Coords() {
	}

	public static double[] fnvToMc(double fx, double fy, double fz) {
		return new double[] { fx / UNITS_PER_BLOCK, fz / UNITS_PER_BLOCK, -fy / UNITS_PER_BLOCK };
	}

	public static double[] mcToFnv(double mx, double my, double mz) {
		return new double[] { mx * UNITS_PER_BLOCK, -mz * UNITS_PER_BLOCK, my * UNITS_PER_BLOCK };
	}

	public static float fnvYawToMc(float fnvYawDeg) {
		return fnvYawDeg + 180.0f;
	}

	public static float mcYawToFnv(float mcYawDeg) {
		return mcYawDeg - 180.0f;
	}

	public static float fnvPitchToMc(float fnvPitchDeg) {
		return -fnvPitchDeg;
	}

	public static float mcPitchToFnv(float mcPitchDeg) {
		return -mcPitchDeg;
	}
}
