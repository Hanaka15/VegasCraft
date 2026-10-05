#pragma once

#include "PCH.h"

namespace vegascraft
{
	// Phase 6: start Prism / custom launcher with the VegasCraft instance.
	class Launcher
	{
	public:
		bool LoadIni(const char* path);
		bool StartMinecraft();
		void StopMinecraft();

		bool startWithHost{ true };
		std::wstring launcher;
		std::wstring arguments{ L"--launch VegasCraft" };

	private:
		PROCESS_INFORMATION pi_{};
		bool running_{ false };
	};
}
