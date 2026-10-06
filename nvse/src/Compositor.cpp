#include "Compositor.h"
#include "Launcher.h"

#include <d3d9.h>

namespace vegascraft
{
	namespace
	{
		// FalloutNV 1.4.0.525 — NiDX9Renderer singleton + IDirect3DDevice9* field.
		// (Same layout NVTF / Gamebryo FO3-NV use: device sits at +0x280 after NiRenderer.)
		constexpr std::uintptr_t kNiDX9RendererSingleton = 0x11C73B4;
		constexpr std::uintptr_t kDeviceOffset = 0x280;

		struct OverlayVertex
		{
			float x, y, z, rhw;
			float u, v;
		};

		constexpr DWORD kFvf = D3DFVF_XYZRHW | D3DFVF_TEX1;

		IDirect3DDevice9* GetGameDevice()
		{
			__try {
				auto* slot = reinterpret_cast<std::uint8_t**>(kNiDX9RendererSingleton);
				if (!slot || !*slot) {
					return nullptr;
				}
				auto** devicePtr = reinterpret_cast<IDirect3DDevice9**>(*slot + kDeviceOffset);
				if (!devicePtr || !*devicePtr) {
					return nullptr;
				}
				// Basic COM sanity: vtable pointer should be in user address space.
				const auto* vtable = *reinterpret_cast<const void* const*>(*devicePtr);
				if (!vtable || reinterpret_cast<std::uintptr_t>(vtable) < 0x10000) {
					return nullptr;
				}
				return *devicePtr;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return nullptr;
			}
		}
	}

	void Compositor::Shutdown()
	{
		if (staging_) {
			static_cast<IDirect3DTexture9*>(staging_)->Release();
			staging_ = nullptr;
		}
		texW_ = texH_ = 0;
	}

	bool Compositor::EnsureTexture(void* devicePtr, std::uint32_t w, std::uint32_t h)
	{
		auto* device = static_cast<IDirect3DDevice9*>(devicePtr);
		if (!device || w < 64 || h < 64 || w > proto::kMaxOverlayW || h > proto::kMaxOverlayH) {
			return false;
		}
		if (staging_ && texW_ == w && texH_ == h) {
			return true;
		}
		if (staging_) {
			static_cast<IDirect3DTexture9*>(staging_)->Release();
			staging_ = nullptr;
			texW_ = texH_ = 0;
		}

		IDirect3DTexture9* tex = nullptr;
		HRESULT hr = device->CreateTexture(w, h, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr);
		if (FAILED(hr) || !tex) {
			hr = device->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr);
		}
		if (FAILED(hr) || !tex) {
			Launcher::Logf("Compositor: CreateTexture %ux%u failed hr=%08lX", w, h, static_cast<unsigned long>(hr));
			return false;
		}
		staging_ = tex;
		texW_ = w;
		texH_ = h;
		return true;
	}

	bool Compositor::BlitOnce(Link& link, void* devicePtr)
	{
		auto* device = static_cast<IDirect3DDevice9*>(devicePtr);
		auto* base = link.Base();
		if (!device || !base) {
			return false;
		}

		auto* ctl = link.At<proto::OverlayCtl>(proto::kOffOverlayCtl);
		const std::uint32_t state = ctl->state;
		if ((state & proto::kOverlayDirty) == 0) {
			return true;
		}
		const std::uint32_t mid = state & 3u;
		ctl->state = frontSlot_;
		frontSlot_ = mid;

		auto* hdr = reinterpret_cast<proto::OverlaySlotHdr*>(base + proto::kOffOverlaySlotHdr + mid * 0x40);
		if (hdr->width < 64 || hdr->height < 64) {
			return true;
		}
		if (!EnsureTexture(device, hdr->width, hdr->height)) {
			return false;
		}

		auto* tex = static_cast<IDirect3DTexture9*>(staging_);
		D3DLOCKED_RECT locked{};
		const DWORD lockFlags = D3DLOCK_DISCARD;
		if (FAILED(tex->LockRect(0, &locked, nullptr, lockFlags))) {
			if (FAILED(tex->LockRect(0, &locked, nullptr, 0))) {
				return false;
			}
		}

		const std::uint8_t* srcBase = base + proto::kOffOverlayPixels + mid * proto::kOverlaySlotBytes;
		const bool bottomUp = (hdr->flags & 1u) != 0;
		auto* dstRows = static_cast<std::uint8_t*>(locked.pBits);
		for (std::uint32_t y = 0; y < hdr->height; ++y) {
			const std::uint32_t srcY = bottomUp ? (hdr->height - 1u - y) : y;
			const auto* src = reinterpret_cast<const std::uint32_t*>(srcBase + static_cast<std::size_t>(srcY) * hdr->width * 4u);
			auto* dst = reinterpret_cast<std::uint32_t*>(dstRows + static_cast<std::size_t>(y) * locked.Pitch);
			for (std::uint32_t x = 0; x < hdr->width; ++x) {
				const std::uint32_t rgba = src[x];
				dst[x] = (rgba & 0xFF00FF00u) | ((rgba & 0x000000FFu) << 16) | ((rgba & 0x00FF0000u) >> 16);
			}
		}
		tex->UnlockRect(0);

		IDirect3DSurface9* back = nullptr;
		if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back) {
			return false;
		}
		D3DSURFACE_DESC bb{};
		back->GetDesc(&bb);
		back->Release();

		const float bw = static_cast<float>(bb.Width);
		const float bh = static_cast<float>(bb.Height);
		OverlayVertex quad[4] = {
			{ 0.f, 0.f, 0.f, 1.f, 0.f, 0.f },
			{ bw, 0.f, 0.f, 1.f, 1.f, 0.f },
			{ 0.f, bh, 0.f, 1.f, 0.f, 1.f },
			{ bw, bh, 0.f, 1.f, 1.f, 1.f },
		};

		DWORD oldZ = 0, oldAlpha = 0, oldSrc = 0, oldDst = 0, oldFog = 0, oldLighting = 0, oldCull = 0;
		device->GetRenderState(D3DRS_ZENABLE, &oldZ);
		device->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlpha);
		device->GetRenderState(D3DRS_SRCBLEND, &oldSrc);
		device->GetRenderState(D3DRS_DESTBLEND, &oldDst);
		device->GetRenderState(D3DRS_FOGENABLE, &oldFog);
		device->GetRenderState(D3DRS_LIGHTING, &oldLighting);
		device->GetRenderState(D3DRS_CULLMODE, &oldCull);

		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_FOGENABLE, FALSE);
		device->SetRenderState(D3DRS_LIGHTING, FALSE);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		device->SetTexture(0, tex);
		device->SetFVF(kFvf);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(OverlayVertex));
		device->SetTexture(0, nullptr);

		device->SetRenderState(D3DRS_ZENABLE, oldZ);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlpha);
		device->SetRenderState(D3DRS_SRCBLEND, oldSrc);
		device->SetRenderState(D3DRS_DESTBLEND, oldDst);
		device->SetRenderState(D3DRS_FOGENABLE, oldFog);
		device->SetRenderState(D3DRS_LIGHTING, oldLighting);
		device->SetRenderState(D3DRS_CULLMODE, oldCull);

		if (!loggedOk_) {
			loggedOk_ = true;
			Launcher::Logf("Compositor: overlay blit ok %ux%u -> backbuffer %ux%u", hdr->width, hdr->height, bb.Width,
				bb.Height);
		}
		return true;
	}

	void Compositor::OnPresent(Link& link, bool isLoadingScreen)
	{
		if (disabled_ || isLoadingScreen || !link.IsOpen()) {
			return;
		}

		IDirect3DDevice9* device = GetGameDevice();
		if (!device) {
			return;
		}

		bool ok = false;
		__try {
			ok = BlitOnce(link, device);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			ok = false;
		}

		if (!ok) {
			if (!loggedFail_) {
				loggedFail_ = true;
				Launcher::Logf("Compositor: blit failed — disabling overlay to keep FNV stable");
			}
			disabled_ = true;
			Shutdown();
		}
	}
}
