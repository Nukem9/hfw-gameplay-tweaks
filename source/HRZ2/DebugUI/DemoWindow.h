#pragma once

#include "DebugUIWindow.h"

namespace HRZ2::DebugUI
{
	class DemoWindow : public Window
	{
	private:
		bool m_WindowOpen = true;

	public:
		virtual void Render() override;
		virtual bool Close() override;
		virtual std::string GetId() const override;
	};
}
