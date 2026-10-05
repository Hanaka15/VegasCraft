#pragma once

#include "PCH.h"

#include <atomic>
#include <filesystem>
#include <string>

namespace vegascraft::Launcher
{
	enum class Status : std::uint32_t
	{
		kOff = 0,
		kStarting,
		kSignIn,
		kRunning,
		kNoLauncher,
		kFailed,
	};

	void SetGameDirectory(std::filesystem::path dir);
	Status GetStatus();
	bool MinecraftRunning();

	// Unpack Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip → %LOCALAPPDATA%\VegasCraft
	// and CreateProcess portable Prism in this process's Wine/Proton prefix.
	void StartMinecraft();
	void StopMinecraft();

	void Logf(const char* fmt, ...);
}
