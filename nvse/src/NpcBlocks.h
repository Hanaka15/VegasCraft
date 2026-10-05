#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 4: keep invisible FNV collision boxes in sync with MC BlockChange / RenSolids.
	class NpcBlocks
	{
	public:
		void DrainRenderSolids(Link& link);
		void Clear();

	private:
		std::uint64_t renderTail_{ 0 };
	};
}
