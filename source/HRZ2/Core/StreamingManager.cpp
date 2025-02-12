#include "../../ModCoreEvents.h"
#include "../Core/IStreamingManager.h"

namespace HRZ2
{
	StreamingManager *(*OriginalStreamingManagerConstructor)(void *Memory);
	StreamingManager *HookedStreamingManagerConstructor(void *Memory)
	{
		auto manager = OriginalStreamingManagerConstructor(Memory);

		if (manager)
			manager->m_StreamingSystem->RegisterEventHandler(&ModCoreEvents::GetInstance());

		return manager;
	}

	DECLARE_HOOK_TRANSACTION(StreamingManager)
	{
		const auto callAddress = Offsets::Signature("48 8B 01 BA ? ? ? 00 FF 90 88 00 00 00 33 DB 48 85 C0 74 0A 48 8B C8 E8")
									 .AsAdjusted(0x18);

		Memory::Patch(callAddress, { 0xE9 }); // CALL->JMP fixup to avoid interference with other mods
		Hooks::WriteCall(callAddress, &HookedStreamingManagerConstructor, &OriginalStreamingManagerConstructor);
	};
}
