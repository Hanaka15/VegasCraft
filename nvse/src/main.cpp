#include "Game.h"

#include "nvse/PluginAPI.h"

#include <cstdio>

namespace
{
	PluginHandle g_pluginHandle = kPluginHandle_Invalid;
	NVSEMessagingInterface* g_messaging = nullptr;
	NVSEInterface* g_nvse = nullptr;
	NVSEConsoleInterface* g_console = nullptr;

	void MessageHandler(NVSEMessagingInterface::Message* msg)
	{
		if (!msg) {
			return;
		}
		auto& game = vegascraft::Game::Get();
		switch (msg->type) {
		case NVSEMessagingInterface::kMessage_DeferredInit:
			game.Init();
			break;
		case NVSEMessagingInterface::kMessage_NewGame:
		case NVSEMessagingInterface::kMessage_PostLoadGame:
			game.OnNewGameOrLoad();
			break;
		case NVSEMessagingInterface::kMessage_MainGameLoop:
			game.OnFrame();
			break;
		case NVSEMessagingInterface::kMessage_OnFramePresent:
			// data may be IDirect3DDevice9* on some xNVSE builds; pass through when present.
			game.OnPresent(msg->data);
			break;
		case NVSEMessagingInterface::kMessage_ExitGame:
		case NVSEMessagingInterface::kMessage_ExitToMainMenu:
		case NVSEMessagingInterface::kMessage_ExitGame_Console:
			game.Shutdown();
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
	info->version = 1;

	if (nvse->isEditor) {
		return false;
	}
	if (nvse->nvseVersion < PACKED_NVSE_VERSION) {
		std::fprintf(stderr, "[VegasCraft] NVSE too old (got %08X need %08X)\n",
			nvse->nvseVersion, PACKED_NVSE_VERSION);
		return false;
	}
	if (nvse->runtimeVersion < RUNTIME_VERSION_1_4_0_525) {
		std::fprintf(stderr, "[VegasCraft] runtime too old (got %08X)\n", nvse->runtimeVersion);
		return false;
	}
	if (nvse->isNogore) {
		std::fprintf(stderr, "[VegasCraft] NoGore is not supported\n");
		return false;
	}
	return true;
}

extern "C" __declspec(dllexport) bool NVSEPlugin_Load(const NVSEInterface* nvse)
{
	g_nvse = const_cast<NVSEInterface*>(nvse);
	g_pluginHandle = nvse->GetPluginHandle();
	g_messaging = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
	g_console = static_cast<NVSEConsoleInterface*>(nvse->QueryInterface(kInterface_Console));

	if (g_messaging) {
		g_messaging->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);
	}

	if (g_console && g_console->RunScriptLine2) {
		// Optional: announce once after DeferredInit via Game::Init.
	}

	std::fprintf(stderr, "[VegasCraft] loaded (handle %u, runtime %08X, nvse %08X) dir=%s\n",
		g_pluginHandle,
		nvse->runtimeVersion,
		nvse->nvseVersion,
		nvse->GetRuntimeDirectory ? nvse->GetRuntimeDirectory() : "?");
	return true;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_DETACH) {
		vegascraft::Game::Get().Shutdown();
	}
	return TRUE;
}
