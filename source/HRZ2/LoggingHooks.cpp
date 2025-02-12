#include <Windows.h>

namespace LoggingHooks
{
	void WINAPI hk_OutputDebugStringA(LPCSTR OutputString)
	{
		if (!OutputString)
			return;

		// Remove trailing newline
		auto finalLength = strlen(OutputString);

		if (finalLength > 0 && OutputString[finalLength - 1] == '\n')
			finalLength--;

		spdlog::info("{:.{}s}", OutputString, finalLength);
	}

	DECLARE_HOOK_TRANSACTION(LoggingHooks)
	{
		Hooks::RedirectImport(nullptr, "KERNEL32.dll", "OutputDebugStringA", &hk_OutputDebugStringA);

		// Kill crash logger
		Memory::Patch(Offsets::Signature("40 53 48 83 EC 20 80 79 38 00 48 8B D9 75 4C"), { 0xC3 });

		// Kill telemetry logger
		Memory::Patch(Offsets::Signature("E8 ? ? ? ? 0F B6 F8 47 38 ? ? 75 05 E8"), { 0xB0, 0x01, 0x90, 0x90, 0x90 });
	};
}
