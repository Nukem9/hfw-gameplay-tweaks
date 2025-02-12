#pragma once

#include "RTTIRefObject.h"

namespace HRZ2
{
	class GameWorldTimeState : public RTTIRefObject
	{
	public:
		float m_TimeOfDay;							  // 0x20
		float m_Unknown24;							  // 0x24
		uint32_t m_Day;								  // 0x28
		bool m_EnableDayNightCycle;					  // 0x2C
		bool m_IsPaused;							  // 0x2D

		virtual const RTTI *GetRTTI() const override; // 0
		virtual ~GameWorldTimeState() override;		  // 1

		void SetTimeOfDay(float Time, float FastForwardDuration)
		{
			const auto func = Offsets::Signature("C5 FA 10 59 20 C5 F8 57 C0 C5 F8 2F D0 76 33 C5 F8 2F CB")
								  .ToPointer<void(GameWorldTimeState *, float, float)>();

			func(this, Time, FastForwardDuration);
		}
	};
	assert_offset(GameWorldTimeState, m_EnableDayNightCycle, 0x2C);
}
