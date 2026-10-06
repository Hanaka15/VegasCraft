#include "FnvPlayer.h"

#include <cmath>

namespace vegascraft::FnvPlayer
{
	namespace
	{
		// xNVSE GameObjects.cpp — FalloutNV.exe 1.4.0.525
		constexpr std::uintptr_t kPlayerSingleton = 0x011DEA3C;

		// TESObjectREFR layout (xNVSE GameObjects.h): rot @ +0x24, pos @ +0x30 (radians / units).
		constexpr std::uintptr_t kOffRotX = 0x24;
		constexpr std::uintptr_t kOffPosX = 0x30;

		constexpr float kRadToDeg = 57.2957795f;
	}

	bool TryRead(double& outX, double& outY, double& outZ, float& outPitchDeg, float& outYawDeg)
	{
		__try {
			auto** slot = reinterpret_cast<std::uint8_t**>(kPlayerSingleton);
			std::uint8_t* player = slot ? *slot : nullptr;
			if (!player) {
				return false;
			}
			const float* pos = reinterpret_cast<const float*>(player + kOffPosX);
			const float* rot = reinterpret_cast<const float*>(player + kOffRotX);
			const float x = pos[0];
			const float y = pos[1];
			const float z = pos[2];
			const float pitchRad = rot[0];
			const float yawRad = rot[2];
			if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
				return false;
			}
			// Reject the origin dump that appears before the world is loaded.
			if (x == 0.f && y == 0.f && z == 0.f) {
				return false;
			}
			outX = x;
			outY = y;
			outZ = z;
			outPitchDeg = pitchRad * kRadToDeg;
			outYawDeg = yawRad * kRadToDeg;
			return true;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}
}
