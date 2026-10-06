#include "Focus.h"
#include "Launcher.h"

namespace vegascraft::Focus
{
	namespace
	{
		struct EnumCtx
		{
			DWORD pid;
			HWND best;
			int bestArea;
		};

		BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lp)
		{
			auto* ctx = reinterpret_cast<EnumCtx*>(lp);
			DWORD pid = 0;
			::GetWindowThreadProcessId(hwnd, &pid);
			if (pid != ctx->pid || !::IsWindowVisible(hwnd)) {
				return TRUE;
			}
			RECT rc{};
			if (!::GetClientRect(hwnd, &rc)) {
				return TRUE;
			}
			const int area = (rc.right - rc.left) * (rc.bottom - rc.top);
			// Prefer the largest visible top-level window (the game, not a tiny helper).
			if (area > ctx->bestArea && (rc.right - rc.left) >= 320 && (rc.bottom - rc.top) >= 240) {
				ctx->bestArea = area;
				ctx->best = hwnd;
			}
			return TRUE;
		}

		int g_framesLeft = 0;
		bool g_logged = false;
	}

	void Kick(int frames)
	{
		g_framesLeft = frames > g_framesLeft ? frames : g_framesLeft;
	}

	void Tick()
	{
		if (g_framesLeft <= 0) {
			return;
		}
		--g_framesLeft;

		EnumCtx ctx{ ::GetCurrentProcessId(), nullptr, 0 };
		::EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&ctx));
		HWND hwnd = ctx.best;
		if (!hwnd) {
			hwnd = ::FindWindowW(nullptr, L"Fallout: New Vegas");
		}
		if (!hwnd) {
			return;
		}

		// Do not SW_RESTORE — that can dump exclusive fullscreen into windowed.
		::ShowWindow(hwnd, SW_SHOW);
		::BringWindowToTop(hwnd);

		HWND fg = ::GetForegroundWindow();
		DWORD fgTid = 0;
		if (fg) {
			fgTid = ::GetWindowThreadProcessId(fg, nullptr);
		}
		const DWORD myTid = ::GetCurrentThreadId();
		if (fgTid && fgTid != myTid) {
			::AttachThreadInput(fgTid, myTid, TRUE);
		}
		::SetForegroundWindow(hwnd);
		::SetActiveWindow(hwnd);
		::SetFocus(hwnd);
		if (fgTid && fgTid != myTid) {
			::AttachThreadInput(fgTid, myTid, FALSE);
		}

		if (!g_logged) {
			g_logged = true;
			RECT rc{};
			::GetClientRect(hwnd, &rc);
			Launcher::Logf("Focus: raised Fallout HWND client=%dx%d", rc.right, rc.bottom);
			if (rc.right < 640 || rc.bottom < 480) {
				Launcher::Logf("Focus: WARNING tiny client — exclusive FS needs Wine virtual desktop (VegasCraft_boot)");
			}
		}
	}
}
