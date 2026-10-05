#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 5: cell/worldspace context, water grid, Activate arbitration.
	class WorldContext
	{
	public:
		void Tick(Link& link);
		std::uint32_t WorldId() const { return worldId_; }
		bool IsInterior() const { return interior_; }
		void OnCellChange(std::uint32_t formId, bool interior);

		// Nearest of MC use-target vs FNV crosshair Activate wins.
		bool TryActivateHost();

	private:
		std::uint32_t worldId_{ 0 };
		bool interior_{ false };
		std::uint32_t waterSeq_{ 0 };
	};
}
