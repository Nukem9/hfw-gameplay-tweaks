#include <filesystem>
#include <charconv>
#include <Windows.h>
#include "ModConfiguration.h"

BOOL WINAPI RawDllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		// Start vsjitdebugger.exe if a debugger isn't already attached. GAME_DEBUGGER_REQUEST determines the
		// command line and GAME_DEBUGGER_PROC is used to hide the CreateProcessA IAT entry.
		if (char cmd[512] = {}, proc[512] = {}; !IsDebuggerPresent() &&
												GetEnvironmentVariableA("GAME_DEBUGGER_REQUEST", cmd, ARRAYSIZE(cmd)) > 0 &&
												GetEnvironmentVariableA("GAME_DEBUGGER_PROC", proc, ARRAYSIZE(proc)) > 0)
		{
			std::to_chars(cmd + strlen(cmd), std::end(cmd), GetCurrentProcessId());
			auto moduleName = proc;
			auto importName = strchr(proc, '!') + 1;
			importName[-1] = '\0';

			PROCESS_INFORMATION pi = {};

			STARTUPINFOA si = {};
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;

			auto c = reinterpret_cast<decltype(&CreateProcessA)>(GetProcAddress(GetModuleHandleA(moduleName), importName));
			c(nullptr, cmd, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi);

			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}

	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		// It's extremely unlikely that a path will be this long, but it's a one time check, so who cares.
		std::wstring fullModulePath(8192, '\0');
		uint32_t len = GetModuleFileNameW(GetModuleHandleW(nullptr), fullModulePath.data(), static_cast<DWORD>(fullModulePath.size()));

		fullModulePath.resize(len);

		try
		{
			if (fullModulePath.empty())
				throw std::runtime_error("Unable to obtain executable path via GetModuleFileNameW()");

			InternalModConfig::Initialize(std::filesystem::path(fullModulePath).remove_filename());
		}
		catch (const std::exception& e)
		{
			// Use a hardcoded buffer. std::format can't convert between wchar_t and char.
			std::wstring buffer(8192, '\0');

			swprintf_s(
				buffer.data(),
				buffer.size(),
				L"An exception has occurred on startup: %hs\n\nFailed to initialize HFW Gameplay Tweaks and Cheat Menu.\n\nExecutable path: %ws",
				e.what(),
				fullModulePath.c_str());

			MessageBoxW(nullptr, buffer.data(), L"Error", MB_ICONERROR);
		}
	}

	return TRUE;
}

extern "C" extern decltype(&RawDllMain) const _pRawDllMain = RawDllMain;
