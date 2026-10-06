#pragma once

#include "PCH.h"

namespace vegascraft::Focus
{
	// Re-assert foreground for a few frames without SW_RESTORE (keeps exclusive fullscreen).
	void Kick(int frames = 180);
	void Tick();
}
