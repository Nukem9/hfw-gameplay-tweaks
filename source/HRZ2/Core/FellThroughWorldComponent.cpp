#include "../../ModConfiguration.h"

namespace HRZ2
{
	class FellThroughWorldComponent;

	DECLARE_HOOK_TRANSACTION(FellThroughWorldComponent)
	{
		if (ModConfiguration.UnlockMapBoundaries)
			Memory::Patch(Offsets::Signature("48 8B 05 ? ? ? ? C5 FB 10 22 48 8B 88 90 04 00"), { 0xB0, 0x01, 0xC3 });
	};
}
