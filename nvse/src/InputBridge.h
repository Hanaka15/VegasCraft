#pragma once

#include "PCH.h"
#include "Link.h"

namespace vegascraft
{
	// Phase 1: swallow most FNV input, forward to MC input ring; allow-list Activate/Esc/Pip-Boy/Map.
	class InputBridge
	{
	public:
		enum class Mode
		{
			Gameplay,
			McScreen,
			HostMenu,
		};

		void SetMode(Mode m) { mode_ = m; }
		Mode GetMode() const { return mode_; }

		void OnRawKey(std::uint16_t sdlScancode, bool down);
		void OnMouseButton(std::uint16_t button, bool down);
		void OnMouseMove(std::int32_t dx, std::int32_t dy);
		void OnScroll(std::int32_t notches120);
		void Flush(Link& link);
		bool ShouldSwallow(std::uint16_t sdlScancode) const;

	private:
		Mode mode_{ Mode::Gameplay };
		std::uint64_t head_{ 0 };

		void Push(Link& link, std::uint16_t type, std::uint16_t code, std::int32_t a, std::int32_t b = 0, std::int32_t c = 0);
	};
}
