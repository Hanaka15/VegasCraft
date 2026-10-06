#include "Compositor.h"
#include "Launcher.h"

#include <d3d9.h>

namespace vegascraft
{
	bool Compositor::Init(void* d3d9Device)
	{
		if (!d3d9Device) {
			return false;
		}
		device_ = d3d9Device;
		return true;
	}

	void Compositor::Shutdown()
	{
		if (staging_) {
			static_cast<IDirect3DTexture9*>(staging_)->Release();
			staging_ = nullptr;
		}
		device_ = nullptr;
		texW_ = texH_ = 0;
	}

	void Compositor::OnPresent(Link& link, void* d3d9Device)
	{
		if (!Init(d3d9Device)) {
			return;
		}
		auto* base = link.Base();
		if (!base) {
			return;
		}

		auto* ctl = link.At<proto::OverlayCtl>(proto::kOffOverlayCtl);
		const std::uint32_t state = ctl->state;
		if ((state & proto::kOverlayDirty) == 0) {
			return;
		}

		// Consume the slot so MC keeps publishing, but do not touch D3D9 yet.
		// DrawPrimitiveUP / CreateTexture during OnFramePresent was hard-crashing
		// Fallout under Proton; re-enable once we have a safer present-hook path.
		const std::uint32_t mid = state & 3u;
		ctl->state = frontSlot_;
		frontSlot_ = mid;

		auto* hdr = reinterpret_cast<proto::OverlaySlotHdr*>(base + proto::kOffOverlaySlotHdr + mid * 0x40);
		if (!loggedBlit_ && hdr->width && hdr->height) {
			loggedBlit_ = true;
			Launcher::Logf("Compositor: overlay slot ready %ux%u (blit disabled under Proton)", hdr->width, hdr->height);
		}
		(void)d3d9Device;
	}
}
