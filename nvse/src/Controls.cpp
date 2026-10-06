#include "Controls.h"
#include "Coords.h"
#include "Launcher.h"
#include "nvse/PluginAPI.h"

#include <cmath>
#include <cstdio>

namespace vegascraft::Controls
{
	namespace
	{
		NVSEConsoleInterface* g_console = nullptr;
		bool g_owns = false;
		bool g_applied = false;

		// FalloutNV.exe 1.4.0.525
		constexpr std::uintptr_t kThePlayer = 0x11DEA3C;
		// TESObjectREFR: NiPoint3 pos @ +0x24, rot radians (X pitch, Y roll, Z yaw) @ +0x30
		constexpr std::uintptr_t kRefRot = 0x30;

		constexpr int kFlags =
			(1 << 0) |  // Movement
			(1 << 1) |  // Looking
			(1 << 2) |  // Pip-Boy
			(1 << 3) |  // Fighting
			(1 << 4) |  // POV / scroll zoom
			(1 << 6);   // Sneak

		void Run(const char* line)
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

		void* Player()
		{
			__try {
				auto** slot = reinterpret_cast<void**>(kThePlayer);
				return (slot && *slot) ? *slot : nullptr;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return nullptr;
			}
		}

		constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
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
			Run(buf);
			Launcher::Logf("Controls: Minecraft owns player (FNV move/look/weapons/POV off)");
		} else {
			std::snprintf(buf, sizeof(buf), "EnablePlayerControlsAltEx %d", kFlags);
			Run(buf);
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
			auto* rot = reinterpret_cast<float*>(static_cast<std::uint8_t*>(player) + kRefRot);
			rot[0] = fnvPitchDeg * kDegToRad;  // X
			rot[2] = fnvYawDeg * kDegToRad;    // Z
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}
}
