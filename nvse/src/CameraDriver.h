#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 1: overwrite FNV camera with Minecraft view / FOV.
	class CameraDriver
	{
	public:
		void Update(const proto::McState& mc);
		void SetEnabled(bool on) { enabled_ = on; }

	private:
		bool enabled_{ true };
	};
}
