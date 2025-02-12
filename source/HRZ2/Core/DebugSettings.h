#pragma once

#include "RTTIObject.h"

namespace HRZ2
{
	class DebugSettings : public RTTIObject
	{
	public:
		char _pad0[0x16];
		bool m_SPAllUnlocked;						  // 0x1E Simulates game completed
		char _pad1[0x9];
		bool m_InfiniteSizeClip;					  // 0x28 Bottomless clip
		bool m_Inexhaustible;						  // 0x29 Infinite stamina
		char _pad2[0x2];
		bool m_ApplyPhotoModeSettingsIngame;		  // 0x2C
		char _pad3[0x3];
		bool m_InfiniteAmmo;						  // 0x30

		virtual const RTTI *GetRTTI() const override; // 0
		virtual ~DebugSettings() override;			  // 1

		static DebugSettings *GetInstance()
		{
			const auto ptr = Offsets::Signature("48 8B 05 ? ? ? ? 80 78 29 00 75 2E")
								 .AsRipRelative(7)
								 .ToPointer<DebugSettings *>();

			return *ptr;
		}
	};
	assert_offset(DebugSettings, m_SPAllUnlocked, 0x1E);
	assert_offset(DebugSettings, m_Inexhaustible, 0x29);
	assert_offset(DebugSettings, m_ApplyPhotoModeSettingsIngame, 0x2C);
	assert_offset(DebugSettings, m_InfiniteAmmo, 0x30);
}
