#include "Dig.h"

namespace vegascraft
{
	void Dig::OnExplosion(float mcX, float mcY, float mcZ, float radiusBlocks)
	{
		if (!enabled_) {
			return;
		}
		(void)mcX;
		(void)mcY;
		(void)mcZ;
		(void)radiusBlocks;
		// Carve crater into Havok / navmesh proxy; mark RenDug bits.
	}

	void Dig::DrainDug(Link& link)
	{
		if (!enabled_) {
			return;
		}
		auto* base = link.Base();
		if (!base) {
			return;
		}
		const std::uint64_t head = *reinterpret_cast<std::uint64_t*>(base + proto::kOffRenderRing);
		const std::uint64_t dataBytes = proto::kRenRingDataBytes;
		while (renderTail_ < head) {
			std::uint64_t pos = renderTail_ % dataBytes;
			const std::uint8_t* p = base + proto::kOffRenderRing + 0x80 + pos;
			std::uint32_t typ = 0, n = 0;
			std::memcpy(&typ, p, 4);
			std::memcpy(&n, p + 4, 4);
			if (typ == proto::kRenPad) {
				renderTail_ += dataBytes - pos;
				continue;
			}
			if (typ == proto::kRenDug && enabled_) {
				// Remove FNV collision/visual for dug bits in section.
			}
			renderTail_ += (8ull + n + 7ull) & ~7ull;
		}
		// Note: NpcBlocks also drains the render ring — production code should share one consumer.
		*reinterpret_cast<std::uint64_t*>(base + proto::kOffRenderRing + 0x40) = renderTail_;
	}
}
