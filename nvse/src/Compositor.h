#pragma once

#include "PCH.h"
#include "Link.h"

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace vegascraft
{
	// Phase 2: CPU-blit Minecraft overlay slots onto the D3D9 backbuffer after Present.
	class Compositor
	{
	public:
		bool Init(void* d3d9Device);
		void Shutdown();
		void OnPresent(Link& link, void* d3d9Device);

	private:
		void* device_{ nullptr };
		void* staging_{ nullptr };
		std::uint32_t texW_{ 0 };
		std::uint32_t texH_{ 0 };
		std::uint32_t frontSlot_{ 2 };
	};
}
