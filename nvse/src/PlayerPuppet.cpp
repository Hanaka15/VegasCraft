#include "PlayerPuppet.h"

namespace vegascraft
{
	void PlayerPuppet::ApplyFnvTransform(double x, double y, double z, float yaw, float pitch)
	{
		lastFnv_ = { x, y, z };
		(void)yaw;
		(void)pitch;
		// Hook point: write TESObjectREFR position / Havok capsule when Game.cpp RE is filled in.
	}

	void PlayerPuppet::Update(Link& link)
	{
		if (!enabled_ || !link.IsOpen()) {
			return;
		}
		proto::McState mc{};
		if (!link.ReadMcState(mc)) {
			return;
		}
		if ((mc.flags & proto::kMcInWorld) == 0) {
			return;
		}
		const auto fnv = coords::McToFnv(mc.x, mc.y, mc.z);
		ApplyFnvTransform(fnv.x, fnv.y, fnv.z, coords::McYawToFnv(mc.yaw), coords::McPitchToFnv(mc.pitch));
	}
}
