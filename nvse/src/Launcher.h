#pragma once

#include "PCH.h"

#include <atomic>

namespace vegascraft::Launcher
{
	enum class Status : std::uint32_t
	{
		kOff = 0,
		kStarting,
		kSignIn,    // first run: Prism needs Microsoft account
		kRunning,
		kNoLauncher,
		kFailed,
	};

	Status GetStatus();
	bool MinecraftRunning();

	// Unpack Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip → %LOCALAPPDATA%\VegasCraft
	// and start portable Prism (--launch VegasCraft). Under Proton this is still a Windows
	// CreateProcess inside FNV's wineprefix, so Local\VegasCraft_v1 shared memory works.
	void StartMinecraft();

	// Does not kill Prism/MC (they quit themselves); reserved for future cleanup.
	void StopMinecraft();
}
