#include "Compositor.h"

namespace vegascraft
{
	bool Compositor::Init(void* d3d9Device)
	{
		device_ = d3d9Device;
		return device_ != nullptr;
	}

	void Compositor::Shutdown()
	{
		staging_ = nullptr;
		device_ = nullptr;
		texW_ = texH_ = 0;
	}

	void Compositor::OnPresent(Link& link, void* d3d9Device)
	{
		(void)d3d9Device;
		auto* base = link.Base();
		if (!base || !device_) {
			return;
		}
		auto* ctl = link.At<proto::OverlayCtl>(proto::kOffOverlayCtl);
		const std::uint32_t state = ctl->state;
		if ((state & proto::kOverlayDirty) == 0) {
			return;
		}
		const std::uint32_t mid = state & 3u;
		ctl->state = frontSlot_;
		frontSlot_ = mid;

		auto* hdr = reinterpret_cast<proto::OverlaySlotHdr*>(base + proto::kOffOverlaySlotHdr + mid * 0x40);
		if (hdr->width == 0 || hdr->height == 0) {
			return;
		}
		const std::uint8_t* pixels = base + proto::kOffOverlayPixels + mid * proto::kOverlaySlotBytes;
		(void)pixels;
		// Hook: LockRect staging texture, memcpy RGBA, StretchRect to backbuffer with alpha blend.
		texW_ = hdr->width;
		texH_ = hdr->height;
	}
}
