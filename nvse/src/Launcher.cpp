#include "Launcher.h"

#include <cstdarg>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <thread>

namespace vegascraft::Launcher
{
	namespace
	{
		std::atomic<Status> g_status{ Status::kOff };

		void Log(const char* fmt, ...)
		{
			char buf[1024];
			va_list ap;
			va_start(ap, fmt);
			std::vsnprintf(buf, sizeof(buf), fmt, ap);
			va_end(ap);
			std::fprintf(stderr, "[VegasCraft] %s\n", buf);
			OutputDebugStringA((std::string("[VegasCraft] ") + buf + "\n").c_str());
		}

		std::wstring Widen(const std::string& utf8)
		{
			if (utf8.empty()) {
				return {};
			}
			const int n = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
			std::wstring out(static_cast<std::size_t>(n), L'\0');
			::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), n);
			return out;
		}

		std::wstring ExpandEnv(const std::wstring& path)
		{
			wchar_t buf[MAX_PATH * 4];
			const DWORD n = ::ExpandEnvironmentStringsW(path.c_str(), buf, static_cast<DWORD>(std::size(buf)));
			return n > 0 && n <= std::size(buf) ? std::wstring(buf) : path;
		}

		bool IniBool(const wchar_t* path, const wchar_t* section, const wchar_t* key, bool def)
		{
			return ::GetPrivateProfileIntW(section, key, def ? 1 : 0, path) != 0;
		}

		std::wstring IniString(const wchar_t* path, const wchar_t* section, const wchar_t* key, const wchar_t* def)
		{
			wchar_t buf[1024];
			::GetPrivateProfileStringW(section, key, def, buf, static_cast<DWORD>(std::size(buf)), path);
			return buf;
		}

		// Bundle next to the plugin (same layout as SkyCraft).
		const std::filesystem::path kBundle = "Data/NVSE/Plugins/VegasCraft/VegasCraft-Minecraft.zip";
		const std::filesystem::path kIni = "Data/NVSE/Plugins/VegasCraft.ini";

		std::filesystem::path InstallDir()
		{
			return ExpandEnv(L"%LOCALAPPDATA%\\VegasCraft");
		}

		std::filesystem::path FindInstalledPrism()
		{
			for (const wchar_t* candidate : {
					 L"%LOCALAPPDATA%\\Programs\\PrismLauncher\\prismlauncher.exe",
					 L"%ProgramFiles%\\PrismLauncher\\prismlauncher.exe",
				 }) {
				std::filesystem::path p = ExpandEnv(candidate);
				if (std::filesystem::exists(p)) {
					return p;
				}
			}
			return {};
		}

		bool RunHidden(const std::wstring& command, const std::filesystem::path& cwd, DWORD timeoutMs, DWORD& exitCode)
		{
			std::wstring mutableCmd = command;
			STARTUPINFOW si{ sizeof(si) };
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			PROCESS_INFORMATION pi{};
			if (!::CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
					cwd.empty() ? nullptr : cwd.c_str(), &si, &pi)) {
				return false;
			}
			::WaitForSingleObject(pi.hProcess, timeoutMs);
			::GetExitCodeProcess(pi.hProcess, &exitCode);
			::CloseHandle(pi.hThread);
			::CloseHandle(pi.hProcess);
			return true;
		}

		// Prefer PowerShell Expand-Archive (present under Proton); fall back to tar.exe (native Win10+).
		bool UnpackZip(const std::filesystem::path& zip, const std::filesystem::path& dest)
		{
			std::error_code ec;
			std::filesystem::create_directories(dest, ec);

			const std::wstring ps =
				L"powershell.exe -NoProfile -NonInteractive -Command \"Expand-Archive -LiteralPath '" + zip.wstring() +
				L"' -DestinationPath '" + dest.wstring() + L"' -Force\"";
			DWORD code = 1;
			if (RunHidden(ps, dest, 10 * 60 * 1000, code) && code == 0) {
				return true;
			}

			std::wstring tar = L"\"" + ExpandEnv(L"%SystemRoot%\\System32\\tar.exe") + L"\" -xf \"" + zip.wstring() +
							   L"\" -C \"" + dest.wstring() + L"\"";
			code = 1;
			if (RunHidden(tar, dest, 10 * 60 * 1000, code) && code == 0) {
				return true;
			}
			Log("unpack failed (powershell exit / tar exit)");
			return false;
		}

		std::filesystem::path EnsureBundle()
		{
			const auto dir = InstallDir();
			const auto prism = dir / "Prism" / "prismlauncher.exe";
			std::error_code ec;
			if (!std::filesystem::exists(kBundle)) {
				Log("bundle missing: %s", kBundle.string().c_str());
				return {};
			}
			const auto stamp = std::format("{} {}", std::filesystem::file_size(kBundle, ec),
				std::filesystem::last_write_time(kBundle, ec).time_since_epoch().count());
			std::string installed;
			if (std::ifstream in{ dir / "bundle.stamp" }; in) {
				std::getline(in, installed);
			}
			if (installed == stamp && std::filesystem::exists(prism)) {
				return prism;
			}

			Log("unpacking VegasCraft Minecraft bundle to %s", dir.string().c_str());
			std::filesystem::create_directories(dir, ec);
			// Drop old mod jars so the new bundle's versions win.
			for (const auto& entry : std::filesystem::directory_iterator(
					 dir / "Prism" / "instances" / "VegasCraft" / ".minecraft" / "mods", ec)) {
				const auto name = entry.path().filename().string();
				if (name.starts_with("vegascraft-") || name.starts_with("fabric-api-") || name.starts_with("e4mc-")) {
					std::filesystem::remove(entry.path(), ec);
				}
			}

			const auto copy = dir / "bundle.zip";
			if (!std::filesystem::copy_file(kBundle, copy, std::filesystem::copy_options::overwrite_existing, ec)) {
				Log("couldn't copy bundle (%s)", ec.message().c_str());
				return {};
			}
			if (!UnpackZip(copy, dir)) {
				std::filesystem::remove(copy, ec);
				return {};
			}
			std::filesystem::remove(copy, ec);

			if (!std::filesystem::exists(prism)) {
				Log("unpack ok but prismlauncher.exe missing");
				return {};
			}

			const auto cfg = dir / "Prism" / "prismlauncher.cfg";
			if (!std::filesystem::exists(cfg)) {
				std::filesystem::copy_file(dir / "defaults" / "prismlauncher.cfg", cfg, ec);
			} else {
				std::ifstream in(cfg, std::ios::binary);
				std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
				in.close();
				if (text.find("LowMemWarning=") == std::string::npos) {
					const auto at = text.find("[General]");
					if (at != std::string::npos) {
						const auto eol = text.find('\n', at);
						text.insert(eol == std::string::npos ? text.size() : eol + 1, "LowMemWarning=false\n");
					} else {
						text += "\n[General]\nLowMemWarning=false\n";
					}
					std::ofstream(cfg, std::ios::binary | std::ios::trunc) << text;
				}
			}
			std::ofstream(dir / "bundle.stamp") << stamp;
			return prism;
		}

		bool StartProcess(const std::filesystem::path& program, const std::wstring& args)
		{
			const auto ext = program.extension().wstring();
			const bool script = _wcsicmp(ext.c_str(), L".bat") == 0 || _wcsicmp(ext.c_str(), L".cmd") == 0;
			const std::wstring dir = program.parent_path().wstring();
			std::wstring command = script ? L"cmd.exe /c \"\"" + program.wstring() + L"\" " + args + L"\""
										  : L"\"" + program.wstring() + L"\" " + args;

			STARTUPINFOW si{ sizeof(si) };
			PROCESS_INFORMATION pi{};
			// Detached child in THIS process's Wine/Proton prefix (critical for Local\ shared memory).
			if (!::CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, script ? CREATE_NO_WINDOW : 0, nullptr,
					dir.empty() ? nullptr : dir.c_str(), &si, &pi)) {
				Log("CreateProcess failed for %s (err %lu)", program.string().c_str(), ::GetLastError());
				return false;
			}
			::CloseHandle(pi.hThread);
			::CloseHandle(pi.hProcess);
			Log("started %s (same prefix / CreateProcess)", program.string().c_str());
			return true;
		}
	}

	Status GetStatus()
	{
		return g_status.load();
	}

	bool MinecraftRunning()
	{
		// Held by Fabric VegasLink.announceRunning() while MC is up.
		HANDLE mutex = ::OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\VegasCraft_v1_minecraft");
		if (mutex) {
			::CloseHandle(mutex);
			return true;
		}
		return false;
	}

	void StartMinecraft()
	{
		const std::wstring iniPath = kIni.wstring();
		if (!IniBool(iniPath.c_str(), L"Minecraft", L"bStartWithHost", true)) {
			Log("Minecraft: bStartWithHost=0");
			return;
		}
		if (MinecraftRunning()) {
			Log("Minecraft: already running (mutex)");
			g_status = Status::kRunning;
			std::thread([] {
				for (int i = 0; i < 60; ++i) {
					std::this_thread::sleep_for(std::chrono::seconds(1));
					if (!MinecraftRunning()) {
						std::this_thread::sleep_for(std::chrono::seconds(3));
						StartMinecraft();
						return;
					}
				}
			}).detach();
			return;
		}

		const std::wstring chosen = ExpandEnv(IniString(iniPath.c_str(), L"Minecraft", L"sLauncher", L""));
		const std::wstring args = IniString(iniPath.c_str(), L"Minecraft", L"sArguments", L"--launch VegasCraft");
		const bool bundled = chosen.empty() && std::filesystem::exists(kBundle);
		const std::filesystem::path installed = chosen.empty() && !bundled ? FindInstalledPrism() : std::filesystem::path{};

		if (chosen.empty() && !bundled && installed.empty()) {
			Log("Minecraft: no VegasCraft-Minecraft.zip and no Prism; set sLauncher or install the bundle");
			g_status = Status::kNoLauncher;
			return;
		}
		if (!chosen.empty() && !std::filesystem::exists(chosen)) {
			Log("Minecraft: sLauncher path missing");
			g_status = Status::kNoLauncher;
			return;
		}

		g_status = Status::kStarting;
		std::thread([chosen, installed, bundled, args] {
			std::filesystem::path program = !chosen.empty() ? std::filesystem::path(chosen) : installed;
			if (bundled) {
				program = EnsureBundle();
				if (program.empty()) {
					g_status = Status::kFailed;
					return;
				}
				if (!std::filesystem::exists(program.parent_path() / "accounts.json")) {
					g_status = Status::kSignIn;
				}
			}
			if (!StartProcess(program, args)) {
				g_status = Status::kFailed;
				return;
			}
			if (g_status != Status::kSignIn) {
				g_status = Status::kRunning;
			}
		}).detach();
	}

	void StopMinecraft()
	{
		// MC quits when the host heartbeats stop / process exits; avoid killing the wineprefix child aggressively.
	}
}
