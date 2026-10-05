#include "InputBridge.h"

namespace vegascraft
{
	namespace
	{
		bool IsAllowListed(std::uint16_t code)
		{
			// SDL scancodes: ESC, TAB (Pip-Boy), M (map). Activate remapped separately.
			return code == 41 || code == 43 || code == 16;
		}
	}

	bool InputBridge::ShouldSwallow(std::uint16_t sdlScancode) const
	{
		if (mode_ == Mode::HostMenu) {
			return false;
		}
		return !IsAllowListed(sdlScancode);
	}

	void InputBridge::Push(Link& link, std::uint16_t type, std::uint16_t code, std::int32_t a, std::int32_t b, std::int32_t c)
	{
		auto* base = link.Base();
		if (!base) {
			return;
		}
		const std::uint64_t off = proto::kOffInputRing + 0x80 + (head_ % proto::kInputRingEntries) * 16ull;
		std::uint8_t* dst = base + off;
		std::memcpy(dst + 0, &type, 2);
		std::memcpy(dst + 2, &code, 2);
		std::memcpy(dst + 4, &a, 4);
		std::memcpy(dst + 8, &b, 4);
		std::memcpy(dst + 12, &c, 4);
		++head_;
		*reinterpret_cast<std::uint64_t*>(base + proto::kOffInputRing + 0x00) = head_;
	}

	void InputBridge::OnRawKey(std::uint16_t sdlScancode, bool down)
	{
		(void)sdlScancode;
		(void)down;
	}

	void InputBridge::OnMouseButton(std::uint16_t button, bool down)
	{
		(void)button;
		(void)down;
	}

	void InputBridge::OnMouseMove(std::int32_t dx, std::int32_t dy)
	{
		(void)dx;
		(void)dy;
	}

	void InputBridge::OnScroll(std::int32_t notches120)
	{
		(void)notches120;
	}

	void InputBridge::Flush(Link& link)
	{
		(void)link;
		// Game.cpp hooks call On* then Flush each frame once OS input is wired.
	}
}
