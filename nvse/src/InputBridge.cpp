#include "InputBridge.h"
#include "Controls.h"
#include "Launcher.h"

#include <atomic>
#include <cmath>

namespace vegascraft
{
	namespace
	{
		struct KeyMap
		{
			int vk;
			std::uint16_t sdl;
		};

		constexpr KeyMap kKeys[] = {
			{ 'W', 26 }, { 'A', 4 }, { 'S', 22 }, { 'D', 7 }, { ' ', 44 }, { VK_SHIFT, 225 }, { VK_CONTROL, 224 },
			{ VK_MENU, 226 }, { 'E', 8 }, { 'Q', 20 }, { 'R', 21 }, { 'F', 9 }, { 'C', 6 }, { 'X', 27 }, { 'Z', 29 },
			{ '1', 30 }, { '2', 31 }, { '3', 32 }, { '4', 33 }, { '5', 34 }, { '6', 35 }, { '7', 36 }, { '8', 37 },
			{ '9', 38 }, { '0', 39 }, { VK_TAB, 43 }, { 'T', 23 }, { VK_OEM_2, 56 }, { 'O', 18 }, { 'G', 10 },
			{ 'H', 11 }, { 'J', 13 }, { 'M', 16 }, { VK_F5, 62 }, { VK_ESCAPE, 41 }, { VK_OEM_3, 53 },
			{ VK_LEFT, 80 }, { VK_RIGHT, 79 }, { VK_UP, 82 }, { VK_DOWN, 81 },
		};

		bool keyDown[256]{};
		bool mouseDown[5]{};
		float lookDx = 0.f;
		float lookDy = 0.f;
		bool wasOwning = false;
		bool haveCenter = false;
		POINT centerScreen{};

		std::atomic<int> scrollAccum{ 0 };
		WNDPROC prevWndProc = nullptr;
		HWND hookedHwnd = nullptr;

		LRESULT CALLBACK InputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			if (msg == WM_MOUSEWHEEL) {
				scrollAccum.fetch_add(GET_WHEEL_DELTA_WPARAM(wParam), std::memory_order_relaxed);
			}
			return prevWndProc ? ::CallWindowProcW(prevWndProc, hwnd, msg, wParam, lParam)
							   : ::DefWindowProcW(hwnd, msg, wParam, lParam);
		}

		HWND FindGameHwnd()
		{
			struct Ctx
			{
				DWORD pid;
				HWND best;
				int bestArea;
			} ctx{ ::GetCurrentProcessId(), nullptr, 0 };
			::EnumWindows(
				[](HWND hwnd, LPARAM lp) -> BOOL {
					auto* c = reinterpret_cast<Ctx*>(lp);
					DWORD pid = 0;
					::GetWindowThreadProcessId(hwnd, &pid);
					if (pid != c->pid || !::IsWindowVisible(hwnd)) {
						return TRUE;
					}
					RECT rc{};
					if (!::GetClientRect(hwnd, &rc)) {
						return TRUE;
					}
					const int area = (rc.right - rc.left) * (rc.bottom - rc.top);
					if (area > c->bestArea && (rc.right - rc.left) >= 320) {
						c->bestArea = area;
						c->best = hwnd;
					}
					return TRUE;
				},
				reinterpret_cast<LPARAM>(&ctx));
			return ctx.best;
		}

		void EnsureWindowHook()
		{
			HWND hwnd = FindGameHwnd();
			if (!hwnd || hwnd == hookedHwnd) {
				return;
			}
			if (hookedHwnd && prevWndProc) {
				::SetWindowLongPtrW(hookedHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(prevWndProc));
			}
			prevWndProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(InputWndProc)));
			hookedHwnd = hwnd;
			Launcher::Logf("InputBridge: WM_MOUSEWHEEL subclass on hwnd %p", hwnd);
		}

		bool UpdateCenter()
		{
			if (!hookedHwnd) {
				return false;
			}
			RECT rc{};
			if (!::GetClientRect(hookedHwnd, &rc)) {
				return false;
			}
			centerScreen = { (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
			::ClientToScreen(hookedHwnd, &centerScreen);
			return true;
		}

		bool IsAllowListedVk(int vk)
		{
			return vk == VK_ESCAPE || vk == VK_OEM_3 || vk == 'M' || vk == 'J' || vk == 'G';
		}
	}

	bool InputBridge::ShouldSwallow(std::uint16_t sdlScancode) const
	{
		if (mode_ == Mode::HostMenu) {
			return false;
		}
		return sdlScancode != 41 && sdlScancode != 53 && sdlScancode != 16 && sdlScancode != 13 && sdlScancode != 10;
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

	void InputBridge::OnRawKey(std::uint16_t, bool) {}
	void InputBridge::OnMouseButton(std::uint16_t, bool) {}

	void InputBridge::OnMouseMove(std::int32_t dx, std::int32_t dy)
	{
		lookDx += static_cast<float>(dx);
		lookDy += static_cast<float>(dy);
	}

	void InputBridge::OnScroll(std::int32_t notches120)
	{
		pendingScroll_ += notches120;
	}

	void InputBridge::ConsumeLook(float& dx, float& dy)
	{
		dx = lookDx;
		dy = lookDy;
		lookDx = lookDy = 0.f;
	}

	void InputBridge::Flush(Link& link)
	{
		EnsureWindowHook();

		const bool owning = Controls::MinecraftOwnsPlayer();
		if (!link.IsOpen() || mode_ == Mode::HostMenu || !owning) {
			if (wasOwning && link.IsOpen()) {
				Push(link, proto::kInReleaseAll, 0, 0);
			}
			wasOwning = false;
			haveCenter = false;
			std::memset(keyDown, 0, sizeof(keyDown));
			std::memset(mouseDown, 0, sizeof(mouseDown));
			return;
		}

		if (!wasOwning) {
			wasOwning = true;
			haveCenter = false;
			std::memset(keyDown, 0, sizeof(keyDown));
			std::memset(mouseDown, 0, sizeof(mouseDown));
			Launcher::Logf("InputBridge: Minecraft owns — cursor-delta look (Wine-safe)");
		}

		for (const auto& km : kKeys) {
			const bool down = (::GetAsyncKeyState(km.vk) & 0x8000) != 0;
			const int idx = km.vk & 0xFF;
			if (down == keyDown[idx]) {
				continue;
			}
			keyDown[idx] = down;
			if (mode_ != Mode::McScreen && IsAllowListedVk(km.vk) && km.vk != 'O') {
				continue;
			}
			Push(link, proto::kInKey, km.sdl, down ? 1 : 0);
		}

		static const int vks[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
		static const std::uint16_t sdlBtn[] = { 1, 3, 2, 4, 5 };
		for (int i = 0; i < 5; ++i) {
			const bool down = (::GetAsyncKeyState(vks[i]) & 0x8000) != 0;
			if (down == mouseDown[i]) {
				continue;
			}
			mouseDown[i] = down;
			Push(link, proto::kInMouseButton, sdlBtn[i], down ? 1 : 0);
		}

		pendingScroll_ += scrollAccum.exchange(0, std::memory_order_relaxed);
		if (pendingScroll_ != 0) {
			Push(link, proto::kInScroll, 0, pendingScroll_);
			pendingScroll_ = 0;
		}

		if (mode_ == Mode::McScreen) {
			haveCenter = false;
			POINT cur{};
			if (::GetCursorPos(&cur)) {
				Push(link, proto::kInCursor, 0, cur.x, cur.y);
			}
			return;
		}

		// FPS mouse: read delta from window center, then warp back. Works under Wine/Proton;
		// Raw Input relative deltas often do not. Same pattern games use without DirectInput.
		if (!haveCenter && UpdateCenter()) {
			::SetCursorPos(centerScreen.x, centerScreen.y);
			haveCenter = true;
		} else if (haveCenter) {
			POINT cur{};
			if (::GetCursorPos(&cur)) {
				lookDx += static_cast<float>(cur.x - centerScreen.x);
				lookDy += static_cast<float>(cur.y - centerScreen.y);
				::SetCursorPos(centerScreen.x, centerScreen.y);
			}
		}
	}
}
