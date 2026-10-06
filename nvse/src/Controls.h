#pragma once

#include "PCH.h"

namespace vegascraft::Controls
{
	// SkyCraft-style: while Minecraft drives the player, FNV must not move, look, fight,
	// change POV (scroll zoom), or open the Pip-Boy. Uses NVSE DisablePlayerControlsAltEx
	// (cleared on load; we re-apply every takeover).
	void SetConsole(void* consoleInterface);
	void SetMinecraftOwnsPlayer(bool owns);
	bool MinecraftOwnsPlayer();

	// Write FNV first-person look from Minecraft yaw/pitch (degrees, MC convention).
	void ApplyLook(float mcYawDeg, float mcPitchDeg);
}
