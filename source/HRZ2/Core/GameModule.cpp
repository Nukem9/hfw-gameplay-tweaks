#include <xbyak/xbyak.h>
#include "../../ModConfiguration.h"
#include "../DebugUI/MainMenuBar.h"
#include "GameModule.h"

namespace HRZ2
{
	void (*OriginalUpdateTimeScale)(GameModule *);

	void HookedUpdateTimeScale(GameModule *Thisptr)
	{
		OriginalUpdateTimeScale(Thisptr);

		if (DebugUI::MainMenuBar::m_TimescaleOverride && (!Thisptr->IsPaused() || DebugUI::MainMenuBar::m_TimescaleOverrideInMenus))
			Thisptr->m_WorldTimeScale = DebugUI::MainMenuBar::m_Timescale;

		if (DebugUI::MainMenuBar::m_PauseGame)
			Thisptr->m_WorldTimeScale = 0.0f;
	}

	void HookedResumeGameDuringTileStreaming(GameModule *Thisptr)
	{
		if ((Thisptr->m_LastGameLoadingState & 0x400) == 0)
			Thisptr->Continue();
	}

	void HookedPauseGameDuringTileStreaming(GameModule *Thisptr, bool a2)
	{
		if ((Thisptr->m_LastGameLoadingState & 0x400) == 0) // Ingame while player active? ("Compiling Shaders")
			Thisptr->Pause(a2);
	}

	DECLARE_HOOK_TRANSACTION(GameModule)
	{
		Hooks::WriteJump(
			Offsets::Signature("8B 41 20 C5 FA 10 81 58 01 00 00 C5 E0 57 DB"),
			&HookedUpdateTimeScale,
			&OriginalUpdateTimeScale);

		if (ModConfiguration.ForceNoShaderLoadingScreen)
		{
			class MovRcxThenCallHookGen : Xbyak::CodeGenerator
			{
			public:
				const uintptr_t HookAddress;

				MovRcxThenCallHookGen(uintptr_t HookLocation, void *TargetFunction) : HookAddress(HookLocation)
				{
					mov(rcx, rsi);
					jmp(ptr[rip]);
					dq(reinterpret_cast<uintptr_t>(TargetFunction));
				}

				void Patch()
				{
					Memory::Fill(HookAddress, 0x90, 6);
					Hooks::WriteCall(HookAddress, getCode());
				}
			};

			static MovRcxThenCallHookGen hookGenResume(
				Offsets::Signature("48 8B CE FF 50 48 48 8B 05 ? ? ? ? C6 80 ? ? ? ? 00"),
				&HookedResumeGameDuringTileStreaming);
			hookGenResume.Patch();

			static MovRcxThenCallHookGen hookGenPause(
				Offsets::Signature("48 8B CE FF 50 40 F6 86 B4 04 00 00 B0 0F 85"),
				&HookedPauseGameDuringTileStreaming);
			hookGenPause.Patch();

			// Disable loading menu call without touching m_LastGameLoadingState
			Memory::Patch(hookGenPause.HookAddress + 0xD, { 0x90, 0xE9 });
		}
	};
}
