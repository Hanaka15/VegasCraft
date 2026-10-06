#pragma once

#include "PCH.h"
#include "Link.h"
#include "Launcher.h"

namespace vegascraft
{
	// Phase 1: collision export around the player for Minecraft.
	class WorldExporter
	{
	public:
		void Tick(Link& link, double playerFnvX, double playerFnvY, double playerFnvZ, std::uint32_t worldId);
		void BumpEpoch();
		std::uint32_t Epoch() const { return epoch_; }

	private:
		std::uint32_t epoch_{ 1 };
		std::uint64_t ringHead_{ 0 };
		bool clearedEpoch_{ false };
	};
}
