#include "../DebugUI/MainMenuBar.h"
#include "IStreamingManager.h"

namespace HRZ2
{
	class ThirdPersonPlayerCameraComponent
	{
	public:
		char _pad[0x248];
		StreamingRef<RTTIRefObject> m_CameraModeResource;
	};

	void(*OriginalUpdateInternal)(ThirdPersonPlayerCameraComponent *Thisptr, float Unknown);
	void HookedUpdateInternal(ThirdPersonPlayerCameraComponent *Thisptr, float Unknown)
	{
		if (DebugUI::MainMenuBar::m_FreeCamMode == DebugUI::MainMenuBar::FreeCamMode::Free)
			return;

		OriginalUpdateInternal(Thisptr, Unknown);
	}

	DECLARE_HOOK_TRANSACTION(ThirdPersonPlayerCameraComponent)
	{
		Hooks::WriteJump(
			Offsets::Signature("48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 41 56 48 81 EC B0 02 00 00 48 8B 79 48 45 33 F6 48 8B D9"),
			&HookedUpdateInternal,
			&OriginalUpdateInternal);
	};
}
