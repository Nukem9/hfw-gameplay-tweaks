#pragma once

#include <imgui.h>
#include "../Core/IStreamingManager.h"
#include "DebugUIWindow.h"

namespace HRZ2
{
	class RTTIRefObject;
	class InventoryItem;
}

namespace HRZ2::DebugUI
{
	class InventoryItemSpawnCallback : public IStreamingRefCallback
	{
	public:
		virtual ~InventoryItemSpawnCallback() = default;
		virtual void OnLoaded(RTTIRefObject *Object, void *Userdata) override;
		virtual void OnUnloaded(RTTIRefObject *Object, void *Userdata) override;
		bool TrySpawn(const GGUUID& ItemUUID, uint32_t ItemCount);
	};

	class PlayerInventoryWindow : public Window
	{
		friend class InventoryItemSpawnCallback;

	private:
		bool m_WindowOpen = true;

		InventoryItemSpawnCallback m_LoaderCallback;
		GGUUID m_NextItemSpawnUUID;
		uint32_t m_NextItemCount = 0;

		ImGuiTextFilter m_NameFilter;
		bool m_FilterItemsInPlayerInventory = false;
		bool m_ShowLocalizedItemNames = true;

	public:
		virtual void Render() override;
		virtual bool Close() override;
		virtual std::string GetId() const override;

	private:
		void DrawTableContextMenu(InventoryItem *Item, const GGUUID& ItemUUID);
	};
}
