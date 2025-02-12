#pragma once

#include "RTTIRefObject.h"

namespace HRZ2
{
	class Module : public RTTIRefObject
	{
	public:
		int m_PauseRequests;						  // 0x20

		virtual const RTTI *GetRTTI() const override; // 0
		virtual ~Module() override;					  // 1
		virtual bool InitModule();					  // 4
		virtual void ExitModule();					  // 5
		virtual void UpdateModule();				  // 6
		virtual void DrawModule();					  // 7
		virtual bool Pause(bool);					  // 8
		virtual bool Continue();					  // 9

		bool IsPaused() const
		{
			return m_PauseRequests > 0;
		}
	};
}
