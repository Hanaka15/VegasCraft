#include "WorldContext.h"

namespace vegascraft
{
	void WorldContext::OnCellChange(std::uint32_t formId, bool interior)
	{
		worldId_ = formId;
		interior_ = interior;
	}

	bool WorldContext::TryActivateHost()
	{
		// Hook: compare MC UseTarget distance vs FNV crosshair ref; call Activate if host wins.
		return false;
	}

	void WorldContext::Tick(Link& link)
	{
		if (!link.IsOpen()) {
			return;
		}
		auto* grid = link.At<proto::WaterGrid>(proto::kOffWaterGrid);
		++waterSeq_;
		grid->seq = waterSeq_ * 2u - 1u;
		grid->worldId = worldId_;
		// Fill surface[] from FNV water bodies (raycast / water planes) — Phase 5 RE.
		for (std::uint32_t i = 0; i < proto::kWaterGridSize * proto::kWaterGridSize; ++i) {
			grid->surface[i] = proto::kNoWater;
		}
		grid->seq = waterSeq_ * 2u;
	}
}
