#include "Game.h"

#include "nvse/PluginAPI.h"

#include <chrono>
#include <filesystem>
#include <thread>

namespace
{
	PluginHandle g_pluginHandle = kPluginHandle_Invalid;
	NVSEMessagingInterface* g_messaging = nullptr;
	NVSEInterface* g_nvse = nullptr;
	bool g_startedMc = false;

	void EnsureGameDir(const NVSEInterface* nvse)
	{
		std::filesystem::path dir;
		if (nvse && nvse->GetRuntimeDirectory) {
			dir = nvse->GetRuntimeDirectory();
		}
		if (dir.empty()) {
			wchar_t mod[MAX_PATH]{};
			::GetModuleFileNameW(nullptr, mod, MAX_PATH);
			dir = std::filesystem::path(mod).parent_path();
		}
		vegascraft::Launcher::SetGameDirectory(std::move(dir));
	}

	void TryStartMinecraft(const char* reason)
	{
		vegascraft::Launcher::Logf("TryStartMinecraft (%s) already=%d", reason, g_startedMc ? 1 : 0);
		if (g_startedMc) {
			return;
		}
		g_startedMc = true;
		vegascraft::Game::Get().Init();  // creates link + starts MC
	}

	void MessageHandler(NVSEMessagingInterface::Message* msg)
	{
		if (!msg) {
			return;
		}
		auto& game = vegascraft::Game::Get();
		switch (msg->type) {
		case NVSEMessagingInterface::kMessage_PostLoad:
		case NVSEMessagingInterface::kMessage_PostPostLoad:
		case NVSEMessagingInterface::kMessage_DeferredInit:
			TryStartMinecraft(msg->type == NVSEMessagingInterface::kMessage_DeferredInit ? "DeferredInit" : "PostLoad");
			break;
		case NVSEMessagingInterface::kMessage_NewGame:
		case NVSEMessagingInterface::kMessage_PostLoadGame:
			game.OnNewGameOrLoad();
			break;
		case NVSEMessagingInterface::kMessage_MainGameLoop:
			game.OnFrame();
			break;
		case NVSEMessagingInterface::kMessage_OnFramePresent:
			game.OnPresent(msg->data);
			break;
		case NVSEMessagingInterface::kMessage_ExitGame:
		case NVSEMessagingInterface::kMessage_ExitToMainMenu:
		case NVSEMessagingInterface::kMessage_ExitGame_Console:
			game.Shutdown();
			g_startedMc = false;
			break;
		default:
			break;
		}
	}
}

extern "C" __declspec(dllexport) bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "VegasCraft";
	info->version = 2;

	if (nvse->isEditor) {
		return false;
	}
	// Don't hard-fail on minor NVSE newer/older packing quirks under Proton.
	if (nvse->runtimeVersion < RUNTIME_VERSION_1_4_0_525) {
		return false;
	}
	if (nvse->isNogore) {
		return false;
	}
	return true;
}

extern "C" __declspec(dllexport) bool NVSEPlugin_Load(const NVSEInterface* nvse)
{
	g_nvse = const_cast<NVSEInterface*>(nvse);
	g_pluginHandle = nvse->GetPluginHandle();
	EnsureGameDir(nvse);

	vegascraft::Launcher::Logf("NVSEPlugin_Load handle=%u runtime=%08X nvse=%08X", g_pluginHandle, nvse->runtimeVersion,
		nvse->nvseVersion);

	g_messaging = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
	if (g_messaging) {
		g_messaging->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);
		vegascraft::Launcher::Logf("registered NVSE messaging listener (iface ver hint)");
	} else {
		vegascraft::Launcher::Logf("WARNING: Messaging interface null — starting Minecraft from Load anyway");
	}

	// Proton/STL sometimes delays or skips DeferredInit visibility; kick off after a short delay too.
	std::thread([] {
		std::this_thread::sleep_for(std::chrono::seconds(3));
		TryStartMinecraft("Load+3s");
	}).detach();

	return true;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_DETACH) {
		vegascraft::Game::Get().Shutdown();
	}
	return TRUE;
}
