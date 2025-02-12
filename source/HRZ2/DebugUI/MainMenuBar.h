#pragma once

#include "../Core/WorldTransform.h"
#include "DebugUIWindow.h"

namespace HRZ2::DebugUI
{
	class MainMenuBar : public Window
	{
	public:
		enum class FreeCamMode
		{
			Off,
			Free,
			Noclip,
		};

		static inline bool m_IsVisible;

		static inline FreeCamMode m_FreeCamMode;
		static inline WorldTransform m_FreeCamPosition;

		static inline bool m_PauseGame;
		static inline bool m_PauseAIProcessing;
		static inline bool m_TimescaleOverride;
		static inline bool m_TimescaleOverrideInMenus;
		static inline float m_Timescale = 1.0f;
		static inline float m_LODRangeModifier = 1.0f;

		static inline bool m_EnableGodMode;
		static inline bool m_EnableDemigodMode;
		static inline bool m_EnableInfiniteClipAmmo;
		static inline bool m_EnableAutoNeutralFaction;

		MainMenuBar();
		virtual void Render() override;
		virtual bool Close() override;
		virtual std::string GetId() const override;

		static void ToggleVisibility();
		static void TogglePauseGameLogic();
		static void TogglePauseAIProcessing();
		static void TogglePauseTimeOfDay();
		static void ToggleQuickSave();
		static void ToggleQuickLoad();
		static void ToggleTimescaleOverride();
		static void AdjustTimescale(float Adjustment);
		static void ToggleFreeflyCamera();
		static void ToggleNoclip();

	private:
		void DrawGameplayMenu();
		void DrawCheatsMenu();
		void DrawMiscellaneousMenu();
	};
}
