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
		double g_lastGoodZ = 0;
		bool g_haveGoodZ = false;
	}

	void PlayerPuppet::ApplyFnvTransform(double x, double y, double z, float yawDegFnv, float pitchDegFnv)
	{
		lastFnv_ = { x, y, z };
		(void)yawDegFnv;
		(void)pitchDegFnv;

		// Never drag the FNV body into a void fall — that kills the save with "fell off the world".
		if (g_haveGoodZ && z < g_lastGoodZ - 512.0) {
			static int warns = 0;
			if (warns++ < 5) {
				Launcher::Logf("PlayerPuppet: skip SetPos void fall z=%.0f (last good %.0f)", z, g_lastGoodZ);
			}
			return;
		}
		if (!g_haveGoodZ || z > g_lastGoodZ - 64.0) {
			g_lastGoodZ = z;
			g_haveGoodZ = true;
		}

		// Throttle console SetPos — every 4th frame is enough for the FNV camera body.
		if ((g_frame++ & 3) != 0) {
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

	void PlayerPuppet::ResetFallGuard()
	{
		g_haveGoodZ = false;
		g_lastGoodZ = 0;
		g_logged = false;
	}
}
