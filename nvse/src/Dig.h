#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 6: dig FNV terrain into MC blocks; apply RenDug holes; explosions.
	class Dig
	{
	public:
		void SetEnabled(bool on) { enabled_ = on; }
		bool Enabled() const { return enabled_; }

		void DrainDug(Link& link);
		void OnExplosion(float mcX, float mcY, float mcZ, float radiusBlocks);

	private:
		bool enabled_{ true };
		std::uint64_t renderTail_{ 0 };
	};
}
