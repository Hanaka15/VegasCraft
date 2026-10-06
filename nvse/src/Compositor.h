#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Blit Minecraft overlay slots onto the D3D9 backbuffer after Present.
	class Compositor
	{
	public:
		bool Init(void* d3d9Device);
		void Shutdown();
		void OnPresent(Link& link, void* d3d9Device);

	private:
		bool EnsureTexture(std::uint32_t w, std::uint32_t h);

		void* device_{ nullptr };
		void* staging_{ nullptr };  // IDirect3DTexture9*
		std::uint32_t texW_{ 0 };
		std::uint32_t texH_{ 0 };
		std::uint32_t frontSlot_{ 2 };
		bool loggedCreate_{ false };
		bool loggedBlit_{ false };
	};
}
