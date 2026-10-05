#include "CameraDriver.h"

namespace vegascraft
{
	void CameraDriver::Update(const proto::McState& mc)
	{
		if (!enabled_) {
			return;
		}
		(void)mc;
		// Hook point: PlayerCamera / NiCamera world transform + FOV (horizontal↔vertical convert).
	}
}
