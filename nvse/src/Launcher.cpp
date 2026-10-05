#include "Launcher.h"

#include <fstream>
#include <string>
#include <vector>

namespace vegascraft
{
	bool Launcher::LoadIni(const char* path)
	{
		std::ifstream in(path);
		if (!in) {
			return false;
		}
		std::string line;
		while (std::getline(in, line)) {
			if (line.rfind("bStartWithSkyrim", 0) == 0 || line.rfind("bStartWithFnv", 0) == 0 || line.rfind("bStartWithHost", 0) == 0) {
				auto eq = line.find('=');
				if (eq != std::string::npos) {
					startWithHost = line.find('1', eq) != std::string::npos;
				}
			}
		}
		return true;
	}

	bool Launcher::StartMinecraft()
	{
		if (!startWithHost || running_) {
			return running_;
		}
		if (launcher.empty()) {
			// Default: %LOCALAPPDATA%\VegasCraft Prism instance — filled when bundle lands.
			return false;
		}
		STARTUPINFOW si{};
		si.cb = sizeof(si);
		std::wstring cmd = L"\"" + launcher + L"\" " + arguments;
		std::vector<wchar_t> buf(cmd.begin(), cmd.end());
		buf.push_back(0);
		if (!::CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi_)) {
			return false;
		}
		running_ = true;
		return true;
	}

	void Launcher::StopMinecraft()
	{
		if (!running_) {
			return;
		}
		::TerminateProcess(pi_.hProcess, 0);
		::CloseHandle(pi_.hProcess);
		::CloseHandle(pi_.hThread);
		pi_ = {};
		running_ = false;
	}
}
