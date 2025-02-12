#include "../../ModConfiguration.h"
#include "../DebugUI/DebugUI.h"
#include "NxD3DImpl.h"
#include "NxDXGIImpl.h"

namespace HRZ2
{
	bool (*OriginalPresent)(NxDXGIImpl *DXGIImpl, void *PresentArgs);
	bool HookedPresent(NxDXGIImpl *Thisptr, void *PresentArgs)
	{
		const static bool uiInitialized = [&]()
		{
			if (!ModConfiguration.EnableDebugMenu)
				return false;

			DebugUI::Initialize(Thisptr);
			return true;
		}();

		if (uiInitialized)
		{
			DebugUI::RenderUI();
			DebugUI::RenderUID3D(NxD3DImpl::GetSingleton(), Thisptr);
		}

		return OriginalPresent(Thisptr, PresentArgs);
	}

	DECLARE_HOOK_TRANSACTION(NxDXGIImpl)
	{
		// NxDXGIImpl::Present vfunc is 10th index in NxDXGIImpl's virtual table
		const auto vtableEntryNxDXGIImpl = Offsets::Signature(
										 "48 8D 0D ? ? ? ? 66 89 68 08 48 89 08 40 88 68 0A 48 89 68 0C 48 89 68 18 48 89 68 20")
										 .AsRipRelative(7)
										 .AsAdjusted(sizeof(void *) * 10)
										 .ToPointer<uintptr_t>();

		Hooks::WriteJump(*vtableEntryNxDXGIImpl, &HookedPresent, &OriginalPresent);
	};
}
