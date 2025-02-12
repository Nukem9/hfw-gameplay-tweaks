#pragma once

#include <shared_mutex>
#include "../PCore/Common.h"
#include "RTTIRefObject.h"
#include "WeakPtr.h"
#include "CameraEntity.h"

namespace HRZ2
{
	class AIFaction;

	class Player : public RTTIRefObject, public WeakPtrRTTITarget
	{
	public:
		char _pad0[0x18];							 // m_Name @ 0x38
		Entity *m_Entity;							 // 0x48
		char _pad1[0x10];
		AIFaction *m_Faction;						 // 0x60
		char _pad2[0x88];
		Array<UnknownCameraEntityRef> m_CameraStack; // 0xF0
		SharedMutex m_CameraStackMutex;				 // 0x100
		char _pad3[0x20];

		virtual const RTTI *GetRTTI() const override; // 0
		virtual ~Player() override;					  // 1
		virtual void UnknownPlayer04();				  // 4
		virtual void UnknownPlayer05();				  // 5
		virtual void UnknownPlayer06();				  // 6
		virtual void UnknownPlayer07();				  // 7
		virtual void UnknownPlayer08();				  // 8
		virtual void UnknownPlayer09();				  // 9
		virtual void UnknownPlayer10();				  // 10
		virtual void UnknownPlayer11();				  // 11
		virtual void UnknownPlayer12();				  // 12
		virtual void UnknownPlayer13();				  // 13
		virtual void UnknownPlayer14();				  // 14
		virtual void UnknownPlayer15();				  // 15
		virtual void UnknownPlayer16();				  // 16
		virtual void UnknownPlayer17();				  // 17

		CameraEntity *GetLastActivatedCamera()
		{
			std::shared_lock lock(m_CameraStackMutex);

			if (m_CameraStack.empty())
				return nullptr;

			return m_CameraStack.back();
		}

		static Player *GetLocalPlayer(uint32_t Index = 0)
		{
			const auto func = Offsets::Signature("40 57 48 83 EC 30 48 63 F9 48 8B 0D ? ? ? ? 48 85 C9")
								  .ToPointer<decltype(GetLocalPlayer)>();

			return func(Index);
		}
	};
	assert_offset(Player, m_Entity, 0x48);
	assert_offset(Player, m_Faction, 0x60);
	assert_offset(Player, m_CameraStack, 0xF0);
	static_assert(sizeof(Player) == 0x128);
}
