#include "Controls.h"
#include "Coords.h"
#include "Launcher.h"
#include "nvse/PluginAPI.h"

#include <cstdio>

namespace vegascraft::Controls
{
	namespace
	{
		NVSEConsoleInterface* g_console = nullptr;
		bool g_owns = false;
		bool g_applied = false;

		constexpr std::uintptr_t kThePlayer = 0x11DEA3C;
		constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

		// Movement + Pip-Boy + Fighting + POV + Sneak. Looking stays enabled so the
		// camera can follow the puppet body (disabling Looking freezes the view).
		constexpr int kFlags =
			(1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 6);

		void* Player()
		{
			__try {
				auto** slot = reinterpret_cast<void**>(kThePlayer);
				return (slot && *slot) ? *slot : nullptr;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return nullptr;
			}
		}

		void RunLine(const char* line)
		{
			if (!g_console) {
				return;
			}
			if (g_console->RunScriptLine2) {
				g_console->RunScriptLine2(line, nullptr, true);
			} else if (g_console->RunScriptLine) {
				g_console->RunScriptLine(line, nullptr);
			}
		}
	}

	void RunScript(const char* line)
	{
		RunLine(line);
	}

	void SetConsole(void* consoleInterface)
	{
		g_console = static_cast<NVSEConsoleInterface*>(consoleInterface);
		if (g_console) {
			Launcher::Logf("Controls: NVSE console interface ready (v%u)", g_console->version);
		}
	}

	void SetMinecraftOwnsPlayer(bool owns)
	{
		if (owns == g_owns && g_applied) {
			return;
		}
		g_owns = owns;
		if (!g_console) {
			static bool warned = false;
			if (!warned) {
				warned = true;
				Launcher::Logf("Controls: WARNING no console interface — cannot DisablePlayerControls");
			}
			return;
		}
		char buf[96];
		if (owns) {
			std::snprintf(buf, sizeof(buf), "DisablePlayerControlsAltEx %d", kFlags);
			RunLine(buf);
			Launcher::Logf("Controls: Minecraft owns player (FNV move/weapons/POV off; camera follows puppet)");
		} else {
			std::snprintf(buf, sizeof(buf), "EnablePlayerControlsAltEx %d", kFlags);
			RunLine(buf);
			Launcher::Logf("Controls: FNV owns player again");
		}
		g_applied = true;
	}

	bool MinecraftOwnsPlayer()
	{
		return g_owns;
	}

	void ApplyLook(float mcYawDeg, float mcPitchDeg)
	{
		void* player = Player();
		if (!player) {
			return;
		}
		const float fnvYawDeg = coords::McYawToFnv(mcYawDeg);
		const float fnvPitchDeg = coords::McPitchToFnv(mcPitchDeg);
		__try {
			auto* base = static_cast<std::uint8_t*>(player);
			auto* rot4c = reinterpret_cast<float*>(base + 0x4C);
			rot4c[0] = fnvPitchDeg * kDegToRad;
			rot4c[2] = fnvYawDeg * kDegToRad;
			auto* rot30 = reinterpret_cast<float*>(base + 0x30);
			rot30[0] = fnvPitchDeg * kDegToRad;
			rot30[2] = fnvYawDeg * kDegToRad;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}
}
