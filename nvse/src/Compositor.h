#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Blit Minecraft overlay onto the D3D9 backbuffer just before Present.
	class Compositor
	{
	public:
		void Shutdown();
		void OnPresent(Link& link, bool isLoadingScreen);

	private:
		bool EnsureTexture(void* device, std::uint32_t w, std::uint32_t h);
		bool BlitOnce(Link& link, void* device);

		void* staging_{ nullptr };  // IDirect3DTexture9*
		std::uint32_t texW_{ 0 };
		std::uint32_t texH_{ 0 };
		std::uint32_t frontSlot_{ 2 };
		bool loggedOk_{ false };
		bool loggedFail_{ false };
		bool disabled_{ false };
		std::uint32_t failStreak_{ 0 };
	};
}
