#pragma once

#include "PCH.h"

namespace vegascraft::FnvPlayer
{
	// Safe read of the local player's feet / look from the live PlayerCharacter.
	// Returns false if the pointer is null or the read faults (menu, load, bad save).
	bool TryRead(double& outX, double& outY, double& outZ, float& outPitchDeg, float& outYawDeg);
}
