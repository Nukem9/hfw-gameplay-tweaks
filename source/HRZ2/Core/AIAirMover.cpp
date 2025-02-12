#include "../../ModConfiguration.h"

namespace HRZ2
{
	class AIAirMover;

	DECLARE_HOOK_TRANSACTION(AIAirMover)
	{
		if (!ModConfiguration.UnlockMountRestrictions)
			return;

		Memory::Patch(
			Offsets::Signature("C5 FA 11 8E A8 01 00 00 74 0C 48 8D 8E 68 02 00 00 E8"),
			{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // Remove the height limit entirely (FLT_MAX)

#if 0
		Memory::Patch(
			Offsets::Relative(0x102B795),
			{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xEB, 0x6C }); // remove variable access for debugging

		Memory::Patch(
			Offsets::Relative(0x102A27C),
			{ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xEB, 0x44 }); // remove variable access for debugging
#endif
	};
}
