#pragma once

#include "PCH.h"
#include "Coords.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 0: Minecraft is authoritative for feet position; FNV player is a puppet.
	class PlayerPuppet
	{
	public:
		void Update(Link& link);
		void SetEnabled(bool on) { enabled_ = on; }
		bool Enabled() const { return enabled_; }

		// Filled by Game hooks when RE is available; until then debug teleport via console/script.
		void ApplyFnvTransform(double x, double y, double z, float yaw, float pitch);
		void ResetFallGuard();

	private:
		bool enabled_{ true };
		coords::Vec3 lastFnv_{};
	};
}
