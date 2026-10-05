#pragma once

#include "PCH.h"

namespace vegascraft::coords
{
	inline constexpr double kUnitsPerBlock = proto::kUnitsPerBlock;

	struct Vec3
	{
		double x{}, y{}, z{};
	};

	// FNV Gamebryo: X east, Y north, Z up.
	// Minecraft: X east, Y up, Z south.
	inline Vec3 FnvToMc(double fx, double fy, double fz)
	{
		return { fx / kUnitsPerBlock, fz / kUnitsPerBlock, -fy / kUnitsPerBlock };
	}

	inline Vec3 McToFnv(double mx, double my, double mz)
	{
		return { mx * kUnitsPerBlock, -mz * kUnitsPerBlock, my * kUnitsPerBlock };
	}

	// FNV Z rotation (yaw): 0 = +Y (north). Minecraft yaw: 0 = +Z (south), increases clockwise.
	inline float FnvYawToMc(float fnvYawDeg)
	{
		return fnvYawDeg + 180.0f;
	}

	inline float McYawToFnv(float mcYawDeg)
	{
		return mcYawDeg - 180.0f;
	}

	inline float FnvPitchToMc(float fnvPitchDeg)
	{
		return -fnvPitchDeg;
	}

	inline float McPitchToFnv(float mcPitchDeg)
	{
		return -mcPitchDeg;
	}
}
