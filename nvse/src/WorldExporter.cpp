#include "WorldExporter.h"
#include "Coords.h"

#include <cmath>

namespace vegascraft
{
	namespace
	{
		// Collision ring entry header (matches SkyCraft / protocol: type + payload).
		constexpr std::uint32_t kColSection = 1;
	}

	void WorldExporter::PushSection(Link& link, std::int32_t sx, std::int32_t sy, std::int32_t sz, const float* aabbs, std::uint32_t count)
	{
		auto* base = link.Base();
		if (!base || count == 0) {
			return;
		}
		const std::uint64_t dataBytes = proto::kCollisionRingBytes - 0x80;
		const std::uint64_t need = (8 + 16 + count * 24ull + 7) & ~7ull;
		std::uint64_t pos = ringHead_ % dataBytes;
		if (pos + need > dataBytes) {
			// pad to end
			std::uint32_t* pad = reinterpret_cast<std::uint32_t*>(base + proto::kOffCollisionRing + 0x80 + pos);
			pad[0] = 0;
			pad[1] = static_cast<std::uint32_t>(dataBytes - pos - 8);
			ringHead_ += dataBytes - pos;
			pos = 0;
		}
		std::uint8_t* dst = base + proto::kOffCollisionRing + 0x80 + pos;
		std::memcpy(dst, &kColSection, 4);
		const std::uint32_t nbytes = static_cast<std::uint32_t>(16 + count * 24);
		std::memcpy(dst + 4, &nbytes, 4);
		std::memcpy(dst + 8, &sx, 4);
		std::memcpy(dst + 12, &sy, 4);
		std::memcpy(dst + 16, &sz, 4);
		std::memcpy(dst + 20, &count, 4);
		std::memcpy(dst + 24, aabbs, count * 24ull);
		ringHead_ += need;
		*reinterpret_cast<std::uint64_t*>(base + proto::kOffCollisionRing + 0x00) = ringHead_;
	}

	void WorldExporter::Tick(Link& link, double playerFnvX, double playerFnvY, double playerFnvZ, std::uint32_t worldId)
	{
		(void)worldId;
		if (!link.IsOpen()) {
			return;
		}
		// MVP: emit a flat floor AABB under the player in MC space (raycast RE hooks later).
		const auto mc = coords::FnvToMc(playerFnvX, playerFnvY, playerFnvZ);
		const float y = static_cast<float>(mc.y) - 0.01f;
		float box[6] = {
			static_cast<float>(mc.x) - 8.f, y - 1.f, static_cast<float>(mc.z) - 8.f,
			static_cast<float>(mc.x) + 8.f, y, static_cast<float>(mc.z) + 8.f
		};
		const std::int32_t sx = static_cast<std::int32_t>(std::floor(mc.x / 16.0));
		const std::int32_t sy = static_cast<std::int32_t>(std::floor(mc.y / 16.0));
		const std::int32_t sz = static_cast<std::int32_t>(std::floor(mc.z / 16.0));
		PushSection(link, sx, sy, sz, box, 1);
	}
}
