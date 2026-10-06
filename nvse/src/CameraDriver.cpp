#include "CameraDriver.h"
#include "Controls.h"

namespace vegascraft
{
	void CameraDriver::Update(const proto::McState& mc)
	{
		if (!enabled_) {
			return;
		}
		// Drive FNV's first-person look from the host-owned yaw/pitch (written into McState
		// echo path via HostState; here we apply the same angles Minecraft is using).
		Controls::ApplyLook(mc.yaw, mc.pitch);
	}

	void CameraDriver::UpdateLook(float mcYawDeg, float mcPitchDeg)
	{
		if (!enabled_) {
			return;
		}
		// Throttle SetAngle — look is also applied via HostState to MC; FNV only needs occasional sync.
		static int n = 0;
		if ((n++ % 2) == 0) {
			Controls::ApplyLook(mcYawDeg, mcPitchDeg);
		}
	}
}
