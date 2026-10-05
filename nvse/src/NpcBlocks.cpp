#include "NpcBlocks.h"

namespace vegascraft
{
	void NpcBlocks::Clear()
	{
		renderTail_ = 0;
		// Destroy spawned collision refs.
	}

	void NpcBlocks::DrainRenderSolids(Link& link)
	{
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
			if (typ == proto::kRenSolids) {
				// Spawn/update Havok box statics for solid bits in this section (Phase 4).
			} else if (typ == proto::kRenDug) {
				// Remove FNV collision/visual for dug bits (Phase 6 Dig).
			}
			renderTail_ += (8ull + n + 7ull) & ~7ull;
		}
		*reinterpret_cast<std::uint64_t*>(base + proto::kOffRenderRing + 0x40) = renderTail_;
	}
}
