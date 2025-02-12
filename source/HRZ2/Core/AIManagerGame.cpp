#include "../DebugUI/MainMenuBar.h"

namespace HRZ2
{
	class AIAgent;
	class AIManagerGame;

	void HookedAIManagerGameUpdateAIJob(AIAgent *Agent, void *a2)
	{
		if (DebugUI::MainMenuBar::m_PauseAIProcessing)
			return;

		(*(void(__fastcall **)(void *, void *))(*(__int64 *)Agent + 32))(Agent, a2);
	}

	DECLARE_HOOK_TRANSACTION(AIManagerGame)
	{
		Hooks::WriteJump(
			Offsets::Signature("48 8B 0B 48 8B 53 10 48 8B 01 48 83 C4 20 5B 48 FF 60 20").AsAdjusted(15),
			&HookedAIManagerGameUpdateAIJob);
	};
}
