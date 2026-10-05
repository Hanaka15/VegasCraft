#include "Launcher.h"

#include <cstdarg>
#include <chrono>
#include <fstream>
#include <mutex>
#include <thread>

namespace vegascraft::Launcher
{
	namespace
	{
		std::atomic<Status> g_status{ Status::kOff };
		std::filesystem::path g_gameDir;
		std::mutex g_logMutex;

		std::filesystem::path LogPath()
		{
			if (!g_gameDir.empty()) {
				return g_gameDir / "Data" / "NVSE" / "Plugins" / "VegasCraft.log";
			}
			return "VegasCraft.log";
		}


		std::wstring ExpandEnv(const std::wstring& path)
		{
			wchar_t buf[MAX_PATH * 4];
			const DWORD n = ::ExpandEnvironmentStringsW(path.c_str(), buf, static_cast<DWORD>(std::size(buf)));
			return n > 0 && n <= std::size(buf) ? std::wstring(buf) : path;
		}

		bool IniBool(const std::filesystem::path& path, const wchar_t* section, const wchar_t* key, bool def)
		{
			return ::GetPrivateProfileIntW(section, key, def ? 1 : 0, path.c_str()) != 0;
		}

		std::wstring IniString(const std::filesystem::path& path, const wchar_t* section, const wchar_t* key, const wchar_t* def)
		{
			wchar_t buf[1024];
			::GetPrivateProfileStringW(section, key, def, buf, static_cast<DWORD>(std::size(buf)), path.c_str());
			return buf;
		}

		std::filesystem::path BundlePath()
		{
			return g_gameDir / "Data" / "NVSE" / "Plugins" / "VegasCraft" / "VegasCraft-Minecraft.zip";
		}

		std::filesystem::path IniPath()
		{
			return g_gameDir / "Data" / "NVSE" / "Plugins" / "VegasCraft.ini";
		}

		std::filesystem::path InstallDir()
		{
			return ExpandEnv(L"%LOCALAPPDATA%\\VegasCraft");
		}

		bool RunHidden(std::wstring command, const std::filesystem::path& cwd, DWORD timeoutMs, DWORD& exitCode)
		{
			STARTUPINFOW si{ sizeof(si) };
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			PROCESS_INFORMATION pi{};
			if (!::CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
					cwd.empty() ? nullptr : cwd.c_str(), &si, &pi)) {
				return false;
			}
			::WaitForSingleObject(pi.hProcess, timeoutMs);
			::GetExitCodeProcess(pi.hProcess, &exitCode);
			::CloseHandle(pi.hThread);
			::CloseHandle(pi.hProcess);
			return true;
		}

		bool UnpackZip(const std::filesystem::path& zip, const std::filesystem::path& dest)
		{
			std::error_code ec;
			std::filesystem::create_directories(dest, ec);

			const auto prismOk = [&] {
				return std::filesystem::exists(dest / "Prism" / "prismlauncher.exe");
			};

			// 32-bit FNV sees System32 as SysWOW64; use Sysnative for the real 64-bit tar/powershell.
			const std::wstring sysNative = ExpandEnv(L"%SystemRoot%\\Sysnative");
			const std::wstring system32 = ExpandEnv(L"%SystemRoot%\\System32");

			auto tryTar = [&](const std::wstring& dir) -> bool {
				const std::filesystem::path tarExe = std::filesystem::path(dir) / L"tar.exe";
				if (!std::filesystem::exists(tarExe)) {
					Logf("unpack: no tar at %s", tarExe.string().c_str());
					return false;
				}
				std::wstring cmd = L"\"" + tarExe.wstring() + L"\" -xf \"" + zip.wstring() + L"\" -C \"" +
								   dest.wstring() + L"\"";
				DWORD code = 1;
				if (RunHidden(std::move(cmd), dest, 10 * 60 * 1000, code) && code == 0 && prismOk()) {
					Logf("unpack: tar ok (%s)", tarExe.string().c_str());
					return true;
				}
				Logf("unpack: tar failed (%s) code=%lu", tarExe.string().c_str(), code);
				return false;
			};

			if (tryTar(sysNative) || tryTar(system32)) {
				return true;
			}

			auto tryPs = [&](const std::wstring& dir) -> bool {
				const std::filesystem::path psExe = std::filesystem::path(dir) / L"WindowsPowerShell\\v1.0\\powershell.exe";
				const std::wstring psPath = std::filesystem::exists(psExe) ? psExe.wstring() : L"powershell.exe";
				std::wstring ps = L"\"" + psPath +
								  L"\" -NoProfile -NonInteractive -Command \"Expand-Archive -LiteralPath '" +
								  zip.wstring() + L"' -DestinationPath '" + dest.wstring() + L"' -Force\"";
				DWORD code = 1;
				if (RunHidden(std::move(ps), dest, 10 * 60 * 1000, code) && code == 0 && prismOk()) {
					Logf("unpack: powershell ok");
					return true;
				}
				Logf("unpack: powershell failed (code %lu)", code);
				return false;
			};

			if (tryPs(sysNative) || tryPs(system32)) {
				return true;
			}

			return prismOk();
		}

		std::filesystem::path EnsureBundle()
		{
			const auto dir = InstallDir();
			const auto prism = dir / "Prism" / "prismlauncher.exe";
			const auto bundle = BundlePath();
			std::error_code ec;

			Logf("bundle path: %s exists=%d", bundle.string().c_str(), std::filesystem::exists(bundle) ? 1 : 0);
			Logf("install dir: %s", dir.string().c_str());

			if (!std::filesystem::exists(bundle)) {
				return {};
			}

			// Size-only stamp so Linux pre-deploy into the Proton prefix can match MSVC.
			const auto stamp = std::to_string(std::filesystem::file_size(bundle, ec));
			std::string installed;
			if (std::ifstream in{ dir / "bundle.stamp" }; in) {
				std::getline(in, installed);
			}
			if (installed == stamp && std::filesystem::exists(prism)) {
				Logf("bundle already unpacked");
				return prism;
			}

			Logf("unpacking Minecraft bundle...");
			std::filesystem::create_directories(dir, ec);
			for (const auto& entry : std::filesystem::directory_iterator(
					 dir / "Prism" / "instances" / "VegasCraft" / ".minecraft" / "mods", ec)) {
				const auto name = entry.path().filename().string();
				if (name.starts_with("vegascraft-") || name.starts_with("fabric-api-") || name.starts_with("e4mc-")) {
					std::filesystem::remove(entry.path(), ec);
				}
			}

			const auto copy = dir / "bundle.zip";
			if (!std::filesystem::copy_file(bundle, copy, std::filesystem::copy_options::overwrite_existing, ec)) {
				Logf("copy bundle failed: %s", ec.message().c_str());
				// Keep a previously extracted Prism (e.g. Linux-side pre-deploy into the prefix).
				if (std::filesystem::exists(prism)) {
					Logf("using existing prism after copy failure");
					return prism;
				}
				return {};
			}
			if (!UnpackZip(copy, dir)) {
				std::filesystem::remove(copy, ec);
				if (std::filesystem::exists(prism)) {
					Logf("unpack helpers failed — using existing prismlauncher.exe");
					std::ofstream(dir / "bundle.stamp") << stamp;
					return prism;
				}
				Logf("FAIL: unpack failed and no prismlauncher.exe under %s", dir.string().c_str());
				return {};
			}
			std::filesystem::remove(copy, ec);

			if (!std::filesystem::exists(prism)) {
				Logf("unpack finished but prismlauncher.exe missing under %s", dir.string().c_str());
				return {};
			}

			const auto cfg = dir / "Prism" / "prismlauncher.cfg";
			if (!std::filesystem::exists(cfg)) {
				std::filesystem::copy_file(dir / "defaults" / "prismlauncher.cfg", cfg, ec);
			}
			std::ofstream(dir / "bundle.stamp") << stamp;
			return prism;
		}

		bool StartProcess(const std::filesystem::path& program, const std::wstring& args)
		{
			const std::wstring dir = program.parent_path().wstring();
			std::wstring command = L"\"" + program.wstring() + L"\" " + args;
			STARTUPINFOW si{ sizeof(si) };
			PROCESS_INFORMATION pi{};
			Logf("CreateProcess: %s", std::filesystem::path(command).string().c_str());
			if (::CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr,
					dir.empty() ? nullptr : dir.c_str(), &si, &pi)) {
				Logf("CreateProcess ok pid=%lu", pi.dwProcessId);
				::CloseHandle(pi.hThread);
				::CloseHandle(pi.hProcess);
				return true;
			}
			const DWORD cpErr = ::GetLastError();
			Logf("CreateProcess failed err=%lu — trying ShellExecuteEx", cpErr);

			// Fallback: 32→64 launch under some Wine/Proton builds needs ShellExecute.
			SHELLEXECUTEINFOW sei{ sizeof(sei) };
			sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
			sei.lpVerb = L"open";
			sei.lpFile = program.c_str();
			sei.lpParameters = args.c_str();
			sei.lpDirectory = dir.empty() ? nullptr : dir.c_str();
			sei.nShow = SW_SHOWNORMAL;
			if (!::ShellExecuteExW(&sei)) {
				Logf("ShellExecuteEx failed err=%lu", ::GetLastError());
				return false;
			}
			if (sei.hProcess) {
				Logf("ShellExecuteEx ok pid handle");
				::CloseHandle(sei.hProcess);
			} else {
				Logf("ShellExecuteEx ok (no process handle)");
			}
			return true;
		}
	}

	void Logf(const char* fmt, ...)
	{
		char buf[2048];
		va_list ap;
		va_start(ap, fmt);
		std::vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);

		std::lock_guard lock(g_logMutex);
		std::fprintf(stderr, "[VegasCraft] %s\n", buf);
		OutputDebugStringA((std::string("[VegasCraft] ") + buf + "\n").c_str());
		std::error_code ec;
		std::filesystem::create_directories(LogPath().parent_path(), ec);
		if (std::ofstream out(LogPath(), std::ios::app); out) {
			SYSTEMTIME st{};
			::GetLocalTime(&st);
			out << st.wYear << '-' << st.wMonth << '-' << st.wDay << ' ' << st.wHour << ':' << st.wMinute << ':'
				<< st.wSecond << " " << buf << '\n';
		}
	}

	void SetGameDirectory(std::filesystem::path dir)
	{
		g_gameDir = std::move(dir);
		Logf("game directory: %s", g_gameDir.string().c_str());
	}

	Status GetStatus()
	{
		return g_status.load();
	}

	bool MinecraftRunning()
	{
		HANDLE mutex = ::OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\VegasCraft_v1_minecraft");
		if (mutex) {
			::CloseHandle(mutex);
			return true;
		}
		return false;
	}

	void StartMinecraft()
	{
		if (g_gameDir.empty()) {
			wchar_t mod[MAX_PATH]{};
			::GetModuleFileNameW(nullptr, mod, MAX_PATH);
			g_gameDir = std::filesystem::path(mod).parent_path();
			Logf("game directory inferred from module: %s", g_gameDir.string().c_str());
		}

		const auto ini = IniPath();
		Logf("ini: %s exists=%d", ini.string().c_str(), std::filesystem::exists(ini) ? 1 : 0);

		if (!IniBool(ini, L"Minecraft", L"bStartWithHost", true)) {
			Logf("bStartWithHost=0 — not starting Prism");
			return;
		}
		if (MinecraftRunning()) {
			Logf("Minecraft mutex already held — not starting another");
			g_status = Status::kRunning;
			return;
		}

		const std::wstring chosen = ExpandEnv(IniString(ini, L"Minecraft", L"sLauncher", L""));
		const std::wstring args = IniString(ini, L"Minecraft", L"sArguments", L"--launch VegasCraft");
		const auto bundle = BundlePath();
		const bool bundled = chosen.empty() && std::filesystem::exists(bundle);

		Logf("sLauncher empty=%d bundled=%d args=%s", chosen.empty() ? 1 : 0, bundled ? 1 : 0,
			std::filesystem::path(args).string().c_str());

		if (chosen.empty() && !bundled) {
			Logf("FAIL: no bundle at %s and no sLauncher", bundle.string().c_str());
			g_status = Status::kNoLauncher;
			return;
		}
		if (!chosen.empty() && !std::filesystem::exists(chosen)) {
			Logf("FAIL: sLauncher missing: %s", std::filesystem::path(chosen).string().c_str());
			g_status = Status::kNoLauncher;
			return;
		}

		g_status = Status::kStarting;
		std::thread([chosen, bundled, args] {
			try {
				std::filesystem::path program = !chosen.empty() ? std::filesystem::path(chosen) : std::filesystem::path{};
				if (bundled) {
					program = EnsureBundle();
					if (program.empty()) {
						g_status = Status::kFailed;
						Logf("FAIL: EnsureBundle returned empty");
						return;
					}
					if (!std::filesystem::exists(program.parent_path() / "accounts.json")) {
						g_status = Status::kSignIn;
						Logf("first run — Prism should show Microsoft sign-in");
					}
				}
				if (!StartProcess(program, args)) {
					g_status = Status::kFailed;
					return;
				}
				if (g_status != Status::kSignIn) {
					g_status = Status::kRunning;
				}
			} catch (const std::exception& ex) {
				Logf("exception in StartMinecraft thread: %s", ex.what());
				g_status = Status::kFailed;
			}
		}).detach();
	}

	void StopMinecraft() {}
}
