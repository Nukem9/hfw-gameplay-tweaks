#pragma once

#include <imgui.h>
#include "../Core/IStreamingManager.h"
#include "../Core/WorldTransform.h"
#include "DebugUIWindow.h"

namespace HRZ2
{
	class RTTIRefObject;
}

namespace HRZ2::DebugUI
{
	class EntitySpawnerLoaderCallback : public IStreamingRefCallback
	{
	public:
		virtual ~EntitySpawnerLoaderCallback() = default;
		virtual void OnLoaded(RTTIRefObject *Object, void *Userdata) override;
		virtual void OnUnloaded(RTTIRefObject *Object, void *Userdata) override;
	};

	class EntitySpawnerWindow : public Window
	{
	private:
		bool m_WindowOpen = true;

		uint32_t m_OutstandingSpawnCount = 0;
		WorldTransform m_NextSpawnTransform;
		size_t m_NextSpawnSelectedIndex = 0;
		RTTIRefObject *m_NextFaction = nullptr;

		SharedMutex m_FactionSetupMutex;
		std::vector<std::pair<Ref<RTTIRefObject>, RTTIRefObject *>> m_FactionSetsPending;

		EntitySpawnerLoaderCallback m_LoaderCallback;
		bool m_StreamerRequestPending = false;

		static inline size_t m_LastSelectedSetupIndex = std::numeric_limits<size_t>::max();
		static inline ImGuiTextFilter m_SpawnerNameFilter;
		static inline std::atomic_bool m_DoSpawnOnNextFrame;

	public:
		virtual void Render() override;
		virtual bool Close() override;
		virtual std::string GetId() const override;

		static void ForceSpawnEntityClick();

	private:
		void RunSpawnCommands();
		WorldTransform GetSpawnTransform(uint32_t Type, const WorldPosition& CustomPosition);
	};
}
