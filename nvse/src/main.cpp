#include "Game.h"

#include "nvse/PluginAPI.h"

#include <filesystem>

namespace
{
	PluginHandle g_pluginHandle = kPluginHandle_Invalid;
	NVSEMessagingInterface* g_messaging = nullptr;
	bool g_bridgeReady = false;

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

	void EnsureBridge(const char* reason)
	{
		vegascraft::Launcher::Logf("EnsureBridge (%s) ready=%d", reason, g_bridgeReady ? 1 : 0);
		if (g_bridgeReady) {
			return;
		}
		g_bridgeReady = true;
		// Shared memory only — Prism is started by VegasCraft_boot.cmd under Proton
		// (and optionally by Launcher::StartMinecraft when bStartWithHost=1 on Windows).
		if (!vegascraft::Game::Get().Init()) {
			vegascraft::Launcher::Logf("WARNING: Game::Init failed (%s)", reason);
			g_bridgeReady = false;
		}
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
			EnsureBridge(msg->type == NVSEMessagingInterface::kMessage_DeferredInit ? "DeferredInit" : "PostLoad");
			break;
		case NVSEMessagingInterface::kMessage_NewGame:
		case NVSEMessagingInterface::kMessage_PostLoadGame:
			EnsureBridge("NewGameOrLoad");
			game.OnNewGameOrLoad();
			break;
		case NVSEMessagingInterface::kMessage_MainGameLoop:
			if (g_bridgeReady) {
				game.OnFrame();
			}
			break;
		case NVSEMessagingInterface::kMessage_OnFramePresent:
			if (g_bridgeReady) {
				game.OnPresent(msg->data);
			}
			break;
		case NVSEMessagingInterface::kMessage_ExitGame:
		case NVSEMessagingInterface::kMessage_ExitToMainMenu:
		case NVSEMessagingInterface::kMessage_ExitGame_Console:
			game.Shutdown();
			g_bridgeReady = false;
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
	info->version = 3;

	if (nvse->isEditor) {
		return false;
	}
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
	// Keep Load minimal: no std::thread (crashes under Proton/Wine during NVSE Load).
	g_pluginHandle = nvse->GetPluginHandle();
	EnsureGameDir(nvse);
	vegascraft::Launcher::Logf("NVSEPlugin_Load ok handle=%u", g_pluginHandle);

	g_messaging = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
	if (g_messaging) {
		g_messaging->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);
		vegascraft::Launcher::Logf("messaging listener registered");
	} else {
		vegascraft::Launcher::Logf("WARNING: messaging interface null");
		EnsureBridge("Load-no-messaging");
	}

	return true;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_DETACH) {
		vegascraft::Game::Get().Shutdown();
	}
	return TRUE;
}
