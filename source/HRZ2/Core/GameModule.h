#pragma once

#include "EntityUpdaterListener.h"
#include "Module.h"
#include "NetMessageListener.h"
#include "NetSubSystemListener.h"

namespace HRZ2
{
	class GameWorldTimeState;
	class WeatherSystem;

	class GameModule : public Module, public EntityUpdaterListener, public NetMessageListener, private NetSubSystemListener
	{
	public:
		char _pad0[0x18];
		GameWorldTimeState *m_WorldTimeState; // 0x58
		char _pad1[0x20];
		float m_WorldTimeScale;				  // 0x80
		char _pad2[0x1C0];
		WeatherSystem *m_WeatherSystem;		  // 0x248
		char _pad3[0x264];
		uint32_t m_LastGameLoadingState;	  // 0x4B4
		// bool m_IsNewGamePlus; // 0x670

		virtual const RTTI *GetRTTI() const override; // 0
		virtual ~GameModule() override;				  // 1
		virtual bool InitModule() override;			  // 4
		virtual void ExitModule() override;			  // 5
		virtual void UpdateModule() override;		  // 6
		virtual void DrawModule() override;			  // 7
		virtual bool Pause(bool) override;			  // 8
		virtual bool Continue() override;			  // 9

		// EntityUpdaterListener
		virtual void OnBeginUpdate() override; // 1
		virtual void OnEndUpdate() override;   // 2

		// NetMessageListener
		virtual void ProcessNetMessage() override; // 1

		// NetSubSystemListener
		virtual void OnStateChanged() override; // 0

		static GameModule *GetInstance()
		{
			const auto ptr = Offsets::Signature("4C 89 25 ? ? ? ? 48 85 C9 74 1E 48 8D 41 08 BB FF FF FF FF")
								 .AsRipRelative(7)
								 .ToPointer<GameModule *>();

			return *ptr;
		}
	};
	assert_offset(GameModule, m_WorldTimeState, 0x58);
	assert_offset(GameModule, m_WorldTimeScale, 0x80);
	assert_offset(GameModule, m_WeatherSystem, 0x248);
	assert_offset(GameModule, m_LastGameLoadingState, 0x4B4);
}
