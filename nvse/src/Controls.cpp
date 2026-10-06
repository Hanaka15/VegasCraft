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
		int g_reapply = 0;

		// Movement + Looking + Pip-Boy + Fighting + POV + Sneak.
		// Looking MUST be off while MC owns: we drive yaw/pitch via SetAngle + HostState.
		// Leaving Looking on fought cursor-recenter and made look feel dead under Wine.
		constexpr int kFlags =
			(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 6);

		void RunLine(const char* line)
		{
			if (!g_console) {
				return;
			}
			__try {
				if (g_console->RunScriptLine2) {
					g_console->RunScriptLine2(line, nullptr, true);
				} else if (g_console->RunScriptLine) {
					g_console->RunScriptLine(line, nullptr);
				}
			} __except (EXCEPTION_EXECUTE_HANDLER) {
			}
		}

		void ApplyOwns(bool owns)
		{
			char buf[96];
			if (owns) {
				std::snprintf(buf, sizeof(buf), "DisablePlayerControlsAltEx %d", kFlags);
			} else {
				std::snprintf(buf, sizeof(buf), "EnablePlayerControlsAltEx %d", kFlags);
			}
			RunLine(buf);
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
			// Re-assert periodically — loads / scripts clear AltEx flags.
			if (owns && ++g_reapply >= 120) {
				g_reapply = 0;
				ApplyOwns(true);
			}
			return;
		}
		g_owns = owns;
		g_reapply = 0;
		if (!g_console) {
			static bool warned = false;
			if (!warned) {
				warned = true;
				Launcher::Logf("Controls: WARNING no console interface — cannot DisablePlayerControls");
			}
			return;
		}
		ApplyOwns(owns);
		g_applied = true;
		static int logs = 0;
		if (logs++ < 6) {
			Launcher::Logf(owns
				? "Controls: Minecraft owns player (FNV move/look/weapons off)"
				: "Controls: FNV owns player again");
		}
	}

	bool MinecraftOwnsPlayer()
	{
		return g_owns;
	}

	void ApplyLook(float mcYawDeg, float mcPitchDeg)
	{
		const float fnvYawDeg = coords::McYawToFnv(mcYawDeg);
		const float fnvPitchDeg = coords::McPitchToFnv(mcPitchDeg);
		char buf[96];
		std::snprintf(buf, sizeof(buf), "player.SetAngle Z %.4f", fnvYawDeg);
		RunLine(buf);
		std::snprintf(buf, sizeof(buf), "player.SetAngle X %.4f", fnvPitchDeg);
		RunLine(buf);
	}
}
