#include "PlayerPuppet.h"
#include "Controls.h"
#include "Launcher.h"

#include <cmath>
#include <cstdio>

namespace vegascraft
{
	namespace
	{
		bool g_logged = false;
		int g_frame = 0;
	}

	void PlayerPuppet::ApplyFnvTransform(double x, double y, double z, float yawDegFnv, float pitchDegFnv)
	{
		lastFnv_ = { x, y, z };
		(void)yawDegFnv;
		(void)pitchDegFnv;

		// Safe path only: NVSE console. Never call guessed engine addresses under Proton.
		// Throttle a little — 5 script lines every frame can hitch; every other frame is enough
		// while MC physics is ~20 Hz anyway.
		if ((g_frame++ & 1) != 0) {
			return;
		}

		char buf[128];
		std::snprintf(buf, sizeof(buf), "player.SetPos X %.4f", x);
		Controls::RunScript(buf);
		std::snprintf(buf, sizeof(buf), "player.SetPos Y %.4f", y);
		Controls::RunScript(buf);
		std::snprintf(buf, sizeof(buf), "player.SetPos Z %.4f", z);
		Controls::RunScript(buf);

		if (!g_logged) {
			g_logged = true;
			Launcher::Logf("PlayerPuppet: console SetPos to %.1f %.1f %.1f (safe path)", x, y, z);
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
		ApplyFnvTransform(fnv.x, fnv.y, fnv.z, coords::McYawToFnv(mc.yaw), coords::McPitchToFnv(mc.pitch));
	}
}
