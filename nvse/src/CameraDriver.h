#pragma once

#include "PCH.h"

namespace vegascraft
{
	namespace proto
	{
		struct McState;
	}

	class CameraDriver
	{
	public:
		void SetEnabled(bool e) { enabled_ = e; }
		void Update(const proto::McState& mc);
		void UpdateLook(float mcYawDeg, float mcPitchDeg);

	private:
		bool enabled_{ true };
	};
}
