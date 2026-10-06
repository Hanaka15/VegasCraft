#include "WorldExporter.h"
#include "Coords.h"

#include <cmath>
#include <cstring>

namespace vegascraft
{
	namespace
	{
		std::uint8_t* RingWrite(Link& link, std::uint64_t& ringHead, std::uint32_t type, std::uint32_t payloadBytes)
		{
			auto* base = link.Base();
			if (!base) {
				return nullptr;
			}
			const std::uint64_t dataBytes = proto::kCollisionRingBytes - 0x80;
			const std::uint64_t need = (8ull + payloadBytes + 7ull) & ~7ull;
			std::uint64_t pos = ringHead % dataBytes;
			if (pos + need > dataBytes) {
				std::uint32_t* pad = reinterpret_cast<std::uint32_t*>(base + proto::kOffCollisionRing + 0x80 + pos);
				pad[0] = proto::kColPad;
				pad[1] = static_cast<std::uint32_t>(dataBytes - pos - 8);
				ringHead += dataBytes - pos;
				pos = 0;
			}
			std::uint8_t* dst = base + proto::kOffCollisionRing + 0x80 + pos;
			std::memcpy(dst, &type, 4);
			std::memcpy(dst + 4, &payloadBytes, 4);
			ringHead += need;
			*reinterpret_cast<std::uint64_t*>(base + proto::kOffCollisionRing + 0x00) = ringHead;
			return dst + 8;
		}

		void FillSolidBits(std::uint64_t bits[8])
		{
			for (int i = 0; i < 8; ++i) {
				bits[i] = ~0ull;
			}
		}
	}

	void WorldExporter::BumpEpoch()
	{
		++epoch_;
		clearedEpoch_ = false;
	}

	void WorldExporter::Tick(Link& link, double playerFnvX, double playerFnvY, double playerFnvZ, std::uint32_t worldId)
	{
		(void)worldId;
		if (!link.IsOpen()) {
			return;
		}

		if (!clearedEpoch_) {
			auto* payload = RingWrite(link, ringHead_, proto::kColClear, 4);
			if (payload) {
				std::memcpy(payload, &epoch_, 4);
				clearedEpoch_ = true;
				Launcher::Logf("collision clear epoch=%u", epoch_);
			}
		}

		// MVP floor under the player until real FNV raycasts exist.
		// Send both exact triangles (smooth feet) and solid voxel blocks (holdUntilReady / vanilla queries).
		const auto mc = coords::FnvToMc(playerFnvX, playerFnvY, playerFnvZ);
		const float y = static_cast<float>(mc.y) - 0.01f;
		const float x0 = static_cast<float>(mc.x) - 8.f;
		const float x1 = static_cast<float>(mc.x) + 8.f;
		const float z0 = static_cast<float>(mc.z) - 8.f;
		const float z1 = static_cast<float>(mc.z) + 8.f;

		const std::int32_t bx = static_cast<std::int32_t>(std::floor(mc.x));
		const std::int32_t by = static_cast<std::int32_t>(std::floor(mc.y)) - 1;
		const std::int32_t bz = static_cast<std::int32_t>(std::floor(mc.z));

		constexpr std::uint32_t kTriCount = 2;
		constexpr std::uint32_t kTriPayload = 32 + kTriCount * 40;
		if (auto* payload = RingWrite(link, ringHead_, proto::kColTris, kTriPayload)) {
			proto::ColRegion region{};
			region.minX = bx - 8;
			region.minY = by - 1;
			region.minZ = bz - 8;
			region.maxX = bx + 8;
			region.maxY = by + 1;
			region.maxZ = bz + 8;
			region.epoch = epoch_;
			region.count = kTriCount;
			std::memcpy(payload, &region, sizeof(region));

			proto::ColTri tris[2]{};
			const float v0[9] = { x0, y, z0, x1, y, z0, x1, y, z1 };
			const float v1[9] = { x0, y, z0, x1, y, z1, x0, y, z1 };
			std::memcpy(tris[0].v, v0, sizeof(v0));
			std::memcpy(tris[1].v, v1, sizeof(v1));
			tris[0].flags = proto::kTriTerrain | proto::kTriDiggable | (proto::kDigDirt << proto::kTriMaterialShift);
			tris[1].flags = tris[0].flags;
			std::memcpy(payload + 32, tris, sizeof(tris));
		}

		// 3×3 solid blocks under feet so isKnown / hasSolidBelow release the hold quickly.
		constexpr std::int32_t kHalf = 1;
		constexpr std::uint32_t kBlockCount = static_cast<std::uint32_t>((kHalf * 2 + 1) * (kHalf * 2 + 1));
		constexpr std::uint32_t kRegPayload = 32 + kBlockCount * 80;
		if (auto* payload = RingWrite(link, ringHead_, proto::kColRegion, kRegPayload)) {
			proto::ColRegion region{};
			region.minX = bx - kHalf;
			region.minY = by;
			region.minZ = bz - kHalf;
			region.maxX = bx + kHalf;
			region.maxY = by;
			region.maxZ = bz + kHalf;
			region.epoch = epoch_;
			region.count = kBlockCount;
			std::memcpy(payload, &region, sizeof(region));

			std::uint8_t* dst = payload + 32;
			for (std::int32_t x = region.minX; x <= region.maxX; ++x) {
				for (std::int32_t z = region.minZ; z <= region.maxZ; ++z) {
					proto::ColBlock blk{};
					blk.x = x;
					blk.y = by;
					blk.z = z;
					FillSolidBits(blk.bits);
					std::memcpy(dst, &blk, sizeof(blk));
					dst += sizeof(blk);
				}
			}
		}
	}
}
