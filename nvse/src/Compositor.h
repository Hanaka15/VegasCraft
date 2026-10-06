#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Consumes Minecraft overlay slots. GPU blit is currently disabled under Proton
	// (DrawPrimitiveUP during OnFramePresent hard-crashed Fallout).
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
		bool loggedBlit_{ false };
	};
}
