#include "PlayerPuppet.h"
#include "Controls.h"
#include "Launcher.h"

#include <cstdio>

namespace vegascraft
{
	namespace
	{
		constexpr std::uintptr_t kThePlayer = 0x11DEA3C;
		// TESObjectREFR (FNV 1.4.0.525): NiPoint3 pos @ +0x40
		constexpr std::uintptr_t kRefPos = 0x40;

		constexpr std::uintptr_t kSetPosCandidates[] = {
			0x008D4A80,
			0x005759A0,
			0x005751E0,
			0x008D32C0,
		};

		struct NiPoint3
		{
			float x, y, z;
		};

		using SetPosFn = void(__thiscall*)(void* self, NiPoint3* pos);

		SetPosFn g_setPos = nullptr;
		bool g_logged = false;
		bool g_triedResolve = false;

		void* Player()
		{
			__try {
				auto** slot = reinterpret_cast<void**>(kThePlayer);
				return (slot && *slot) ? *slot : nullptr;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return nullptr;
			}
		}

		bool TrySetPosCall(SetPosFn fn, void* player, NiPoint3* pos)
		{
			__try {
				fn(player, pos);
				return true;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return false;
			}
		}

		void ResolveSetPos(void* player, NiPoint3* probe)
		{
			if (g_triedResolve) {
				return;
			}
			g_triedResolve = true;
			for (auto addr : kSetPosCandidates) {
				auto* fn = reinterpret_cast<SetPosFn>(addr);
				if (TrySetPosCall(fn, player, probe)) {
					g_setPos = fn;
					Launcher::Logf("PlayerPuppet: TESObjectREFR::SetPos @ %08X", static_cast<unsigned>(addr));
					return;
				}
			}
			Launcher::Logf("PlayerPuppet: SetPos ThisCall unresolved — console SetPos fallback");
		}
	}

	void PlayerPuppet::ApplyFnvTransform(double x, double y, double z, float yawDegFnv, float pitchDegFnv)
	{
		lastFnv_ = { x, y, z };
		void* player = Player();
		if (!player) {
			return;
		}

		NiPoint3 pos{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };

		__try {
			auto* raw = reinterpret_cast<float*>(static_cast<std::uint8_t*>(player) + kRefPos);
			raw[0] = pos.x;
			raw[1] = pos.y;
			raw[2] = pos.z;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}

		if (!g_setPos) {
			ResolveSetPos(player, &pos);
		}
		bool ok = false;
		if (g_setPos) {
			ok = TrySetPosCall(g_setPos, player, &pos);
		}
		if (!ok) {
			static int throttle = 0;
			if ((throttle++ % 2) == 0) {
				char buf[128];
				std::snprintf(buf, sizeof(buf), "player.SetPos X %.4f", x);
				Controls::RunScript(buf);
				std::snprintf(buf, sizeof(buf), "player.SetPos Y %.4f", y);
				Controls::RunScript(buf);
				std::snprintf(buf, sizeof(buf), "player.SetPos Z %.4f", z);
				Controls::RunScript(buf);
			}
		}

		// Look: MC degrees for ApplyLook
		Controls::ApplyLook(coords::FnvYawToMc(yawDegFnv), coords::FnvPitchToMc(pitchDegFnv));

		if (!g_logged) {
			g_logged = true;
			Launcher::Logf("PlayerPuppet: moving FNV player to %.1f %.1f %.1f", x, y, z);
		}
	}

	void PlayerPuppet::Update(Link& link)
	{
		if (!enabled_ || !link.IsOpen()) {
			return;
		}
		proto::McState mc{};
		if (!link.ReadMcState(mc)) {
			return;
		}
		if ((mc.flags & proto::kMcInWorld) == 0) {
			return;
		}
		const auto fnv = coords::McToFnv(mc.x, mc.y, mc.z);
		// Use host-integrated look when available (Game owns lookYaw_); fall back to MC state.
		ApplyFnvTransform(fnv.x, fnv.y, fnv.z, coords::McYawToFnv(mc.yaw), coords::McPitchToFnv(mc.pitch));
	}
}
