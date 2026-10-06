#include "Game.h"

#include "nvse/PluginAPI.h"
#include "Focus.h"

#include <cstring>

namespace
{
	PluginHandle g_pluginHandle = kPluginHandle_Invalid;
	NVSEMessagingInterface* g_messaging = nullptr;
	const NVSEInterface* g_nvse = nullptr;
	bool g_bridgeReady = false;

	void EnsureGameDir()
	{
		char dir[MAX_PATH]{};
		if (g_nvse && g_nvse->GetRuntimeDirectory) {
			const char* runtime = g_nvse->GetRuntimeDirectory();
			if (runtime && runtime[0]) {
				std::strncpy(dir, runtime, MAX_PATH - 1);
			}
		}
		if (!dir[0]) {
			char mod[MAX_PATH]{};
			::GetModuleFileNameA(nullptr, mod, MAX_PATH);
			std::strncpy(dir, mod, MAX_PATH - 1);
			if (char* slash = std::strrchr(dir, '\\')) {
				*slash = '\0';
			} else if (char* slash = std::strrchr(dir, '/')) {
				*slash = '\0';
			}
		}
		vegascraft::Launcher::SetGameDirectory(dir);
	}

	void EnsureBridge(const char* reason)
	{
		if (!g_bridgeReady) {
			EnsureGameDir();
		}
		vegascraft::Launcher::Logf("EnsureBridge (%s) ready=%d", reason, g_bridgeReady ? 1 : 0);
		if (g_bridgeReady) {
			return;
		}
		g_bridgeReady = true;
		// Shared memory only — Prism is started by VegasCraft_boot.cmd under Proton.
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
			vegascraft::Focus::Kick(240);
			break;
		case NVSEMessagingInterface::kMessage_NewGame:
		case NVSEMessagingInterface::kMessage_PostLoadGame:
			EnsureBridge("NewGameOrLoad");
			vegascraft::Focus::Kick(120);
			game.OnNewGameOrLoad();
			break;
		case NVSEMessagingInterface::kMessage_MainGameLoop:
			vegascraft::Focus::Tick();
			if (g_bridgeReady) {
				game.OnFrame();
			}
			break;
		case NVSEMessagingInterface::kMessage_OnFramePresent:
			// msg->data is int* loadingScreen (NOT IDirect3DDevice9*). Device comes from NiDX9Renderer.
			if (g_bridgeReady) {
				bool loading = false;
				if (msg->data && msg->dataLen >= sizeof(int)) {
					loading = *static_cast<int*>(msg->data) != 0;
				}
				game.OnPresent(loading);
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
	info->version = 5;

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
	// Proton-safe Load: no std::thread, no std::filesystem, no file logging here.
	// Heavy init runs on NVSE messaging (PostLoad / DeferredInit).
	g_nvse = nvse;
	g_pluginHandle = nvse->GetPluginHandle();

	g_messaging = static_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
	if (g_messaging) {
		g_messaging->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);
	} else {
		EnsureBridge("Load-no-messaging");
	}

	return true;
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID)
{
	return TRUE;
}
