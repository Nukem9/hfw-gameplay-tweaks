#include "../../ModConfiguration.h"

namespace HRZ2
{
	class UnifiedEntitlementManagerNoOp;

	DECLARE_HOOK_TRANSACTION(UnifiedEntitlementManagerNoOp)
	{
		if (!ModConfiguration.UnlockEntitlementExtras)
			return;

		auto isUnifiedEntitlementAvailableHookLoc = Offsets::Signature(
			"48 83 EC 28 48 83 C1 50 E8 ? ? ? ? 83 F8 FF 0F 95 C0 48 83 C4 28 C3");
		Memory::Patch(isUnifiedEntitlementAvailableHookLoc, { 0xB0, 0x01, 0xC3 });

		auto isUnifiedEntitlementMountedHookLoc = Offsets::Signature(
			"48 83 EC 28 48 83 C1 50 E8 ? ? ? ? 33 C9 83 F8 FF 0F 95 C1 8B C1 48 83 C4 28 C3");
		Memory::Patch(isUnifiedEntitlementMountedHookLoc, { 0xB0, 0x01, 0xC3 });
	};
}
