#include <format>
#include <shared_mutex>
#include "../../ModConfiguration.h"
#include "../../ModCoreEvents.h"
#include "../Core/Entity.h"
#include "../Core/Inventory.h"
#include "../Core/JobHeaderCPU.h"
#include "../Core/Player.h"
#include "PlayerInventoryWindow.h"

namespace HRZ2::DebugUI
{
	static StreamingRefBase g_TargetRef;

	void InventoryItemSpawnCallback::OnLoaded(RTTIRefObject *Object, void *Userdata)
	{
		if constexpr (false)
			spdlog::info("Received inventory item callback with root UUID {}", Object->m_UUID);

		auto targetUUID = static_cast<PlayerInventoryWindow *>(Userdata)->m_NextItemSpawnUUID;
		auto targetCount = static_cast<PlayerInventoryWindow *>(Userdata)->m_NextItemCount;

		std::shared_lock lock(ModCoreEvents::GetInstance().m_CachedDataMutex);
		TrySpawn(targetUUID, targetCount);
	}

	void InventoryItemSpawnCallback::OnUnloaded(RTTIRefObject *Object, void *Userdata) {}

	bool InventoryItemSpawnCallback::TrySpawn(const GGUUID& ItemUUID, uint32_t ItemCount)
	{
		if (ItemCount <= 0)
			return true;

		for (const auto entry : ModCoreEvents::GetInstance().m_CachedInventoryItems)
		{
			if (entry->m_UUID != ItemUUID)
				continue;

			JobHeaderCPU::SubmitCallable(
				[ItemCount, item = Ref(static_cast<EntityResource *>(entry))]()
				{
					auto entity = Player::GetLocalPlayer() ? Player::GetLocalPlayer()->m_Entity : nullptr;

					if (!entity)
						return;

					std::lock_guard lock(entity->m_EntityAccessMutex);
					auto inventory = entity->m_Components.FindComponentByRTTI<Inventory>(RTTI::FindTypeByName("Inventory"));

					if (!inventory)
						return;

					// Try inventory first, then fallback to stash
					if (!inventory->AddItem(item, ItemCount, true))
					{
						const auto addOverflowItem = Offsets::Signature("44 88 4C 24 20 44 89 44 24 18 53 55 56 57 41 55 41 57 48 83 EC 68")
														 .ToPointer<bool(EntityComponent *, EntityResource *, int, bool)>();

						addOverflowItem(
							entity->m_Components.FindComponentByRTTI(RTTI::FindTypeByName("InventoryOverflowComponent")),
							item,
							ItemCount,
							true);
					}
				});

			return true;
		}

		return false;
	}

	void PlayerInventoryWindow::Render()
	{
		ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin(GetId().c_str(), &m_WindowOpen))
		{
			ImGui::End();
			return;
		}

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "WARNING:");
		ImGui::SameLine();
		ImGui::TextWrapped("Spawning, adding, or deleting quest items can permanently break game progression. Create a new save before "
						   "using this tool. Use at your own risk.");

		m_NameFilter.Draw();
		ImGui::Checkbox("Show only player inventory items", &m_FilterItemsInPlayerInventory);
		ImGui::SameLine();
		ImGui::Checkbox("Show localized names", &m_ShowLocalizedItemNames);

		const ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
										   ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
										   ImGuiTableFlags_SizingFixedFit;

		if (ImGui::BeginTable("inventory_item_list", 3, tableFlags))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Count");
			ImGui::TableSetupColumn("Resource UUID");
			ImGui::TableSetupScrollFreeze(1, 1);
			ImGui::TableHeadersRow();

			struct SortEntry
			{
				const GGUUID *UUID = nullptr;
				std::variant<String, const std::string *> Name; // Performance reasons
				InventoryItem *Hint = nullptr;

				operator const char *() const
				{
					if (Name.index() == 0)
						return std::get<0>(Name).c_str();

					return std::get<1>(Name)->c_str();
				}

				bool operator<(const SortEntry& Other) const
				{
					if (auto result = strcmp(*this, Other); result != 0)
						return result < 0;

					return (UUID && Other.UUID) ? *UUID < *Other.UUID : false;
				}
			};

			// Combine both the player's inventory and the cached item list
			auto playerEntity = Player::GetLocalPlayer() ? Player::GetLocalPlayer()->m_Entity : nullptr;

			if (playerEntity)
			{
				std::lock_guard lock(playerEntity->m_EntityAccessMutex);
				auto playerInventory = playerEntity->m_Components.FindComponentByRTTI<Inventory>(RTTI::FindTypeByName("Inventory"));

				std::vector<SortEntry> sortedItems;
				std::unordered_set<GGUUID> knownInventoryItems;

				if (playerInventory)
				{
					for (const auto& item : playerInventory->m_Items)
					{
						auto& s = sortedItems.emplace_back(
							item.Value->m_EntityResource ? &item.Value->m_EntityResource->m_UUID : nullptr,
							item.Value->GetDisplayName(),
							item.Value);

						if (s.UUID)
						{
							if (!m_ShowLocalizedItemNames) // Delocalize it. Fast binary search.
							{
								const auto itr = std::lower_bound(
									ModConfiguration.CachedInventoryItems.begin(),
									ModConfiguration.CachedInventoryItems.end(),
									*s.UUID);

								if (itr != ModConfiguration.CachedInventoryItems.end() && itr->UUID == *s.UUID)
									s.Name = &itr->Name;
							}

							knownInventoryItems.emplace(*s.UUID);
						}
					}
				}

				if (!m_FilterItemsInPlayerInventory)
				{
					for (const auto& item : ModConfiguration.CachedInventoryItems)
					{
						if (!knownInventoryItems.contains(item.UUID))
							sortedItems.emplace_back(&item.UUID, &item.Name, nullptr);
					}
				}

				// Sort and filter
				std::erase_if(
					sortedItems,
					[&](const auto& Entry)
					{
						return !m_NameFilter.PassFilter(Entry);
					});

				std::ranges::sort(sortedItems);

				// Then draw
				for (const auto& entry : sortedItems)
				{
					const auto resourceUUID = entry.UUID ? *entry.UUID : GGUUID {};

					if (entry.UUID)
						ImGui::PushID(entry.UUID);
					else
						ImGui::PushID(entry.Hint);
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Selectable(entry, false, ImGuiSelectableFlags_SpanAllColumns);
					DrawTableContextMenu(entry.Hint, resourceUUID);

					if (entry.Hint)
					{
						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%u", entry.Hint->m_Amount);
					}

					ImGui::TableSetColumnIndex(2);
					ImGui::Text(std::format("{}", resourceUUID).c_str());

					ImGui::PopID();
				}
			}

			ImGui::EndTable();
		}

		ImGui::End();
	}

	void PlayerInventoryWindow::DrawTableContextMenu(InventoryItem *Item, const GGUUID& ItemUUID)
	{
		if (!ImGui::BeginPopupContextItem("IVITListRowPopup", ImGuiPopupFlags_MouseButtonLeft))
			return;

		int64_t itemCount = 0;

		if (ImGui::Selectable("Add One", false, 0, ImVec2(200, 0)))
			itemCount += 1;

		if (ImGui::Selectable("Add Five"))
			itemCount += 5;

		// Only able to remove existing items
		if (Item)
		{
			if (ImGui::Selectable("Add Double"))
				itemCount += static_cast<int64_t>(Item->m_Amount) * 2;

			ImGui::Selectable("##sepsel1", false, ImGuiSelectableFlags_Disabled);

			if (ImGui::Selectable("Remove One"))
				itemCount -= 1;

			if (ImGui::Selectable("Remove Five"))
				itemCount -= 5;

			if (ImGui::Selectable("Remove Half"))
				itemCount -= std::max(Item->m_Amount / 2, 1u);

			ImGui::Selectable("##sepsel2", false, ImGuiSelectableFlags_Disabled);

			if (ImGui::Selectable("Remove All"))
				itemCount -= Item->m_Amount;
		}

		if (itemCount < 0)
		{
			// Removal doesn't need special handling
			JobHeaderCPU::SubmitCallable([itemCount, item = Ref(Item)]
			{
				auto entity = Player::GetLocalPlayer() ? Player::GetLocalPlayer()->m_Entity : nullptr;

				if (!entity)
					return;

				std::lock_guard lock(entity->m_EntityAccessMutex);
				auto inventory = entity->m_Components.FindComponentByRTTI<Inventory>(RTTI::FindTypeByName("Inventory"));

				if (!inventory)
					return;

				inventory->RemoveItem(item, static_cast<uint32_t>(-itemCount), EInventoryItemRemoveType::Destroy, true);
			});
		}
		else if (itemCount != 0)
		{
			m_NextItemSpawnUUID = ItemUUID;
			m_NextItemCount = static_cast<uint32_t>(itemCount);

			const bool itemWasAlreadySpawned = [&]()
			{
				std::shared_lock lock(ModCoreEvents::GetInstance().m_CachedDataMutex);
				return m_LoaderCallback.TrySpawn(m_NextItemSpawnUUID, m_NextItemCount);
			}();

			if (!itemWasAlreadySpawned)
			{
				// We have to manually resolve a root UUID now
				const auto itr = std::lower_bound(
					ModConfiguration.CachedInventoryItems.begin(),
					ModConfiguration.CachedInventoryItems.end(),
					m_NextItemSpawnUUID);

				if (itr != ModConfiguration.CachedInventoryItems.end() && itr->UUID == m_NextItemSpawnUUID)
				{
					auto streamingManager = StreamingManager::GetInstance();

					g_TargetRef.Clear();
					streamingManager->Register2(g_TargetRef, {}, itr->RootUUID);
					streamingManager->RegisterCallback(g_TargetRef, EStreamingRefCallbackMode::OnLoad, &m_LoaderCallback, this);
					streamingManager->Resolve(g_TargetRef, EStreamingRefPriority::Normal);
				}
			}
		}

		ImGui::EndPopup();
	}

	bool PlayerInventoryWindow::Close()
	{
		return !m_WindowOpen;
	}

	std::string PlayerInventoryWindow::GetId() const
	{
		return "Player Inventory";
	}
}
