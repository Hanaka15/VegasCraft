#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 1: raycast grid around the player → CollisionField AABBs for Minecraft.
	class WorldExporter
	{
	public:
		void Tick(Link& link, double playerFnvX, double playerFnvY, double playerFnvZ, std::uint32_t worldId);
		void BumpEpoch() { ++epoch_; }
		std::uint32_t Epoch() const { return epoch_; }

	private:
		void PushSection(Link& link, std::int32_t sx, std::int32_t sy, std::int32_t sz, const float* aabbs, std::uint32_t count);

		std::uint32_t epoch_{ 1 };
		std::uint64_t ringHead_{ 0 };
	};
}
