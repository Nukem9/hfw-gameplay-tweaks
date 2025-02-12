#include <algorithm>
#include <format>
#include <mutex>
#include "../../ModConfiguration.h"
#include "../../ModCoreEvents.h"
#include "../Core/Entity.h"
#include "../Core/JobHeaderCPU.h"
#include "../Core/Player.h"
#include "../PCore/UUID.h"
#include "EntitySpawnerWindow.h"

namespace HRZ2::DebugUI
{
	static StreamingRefBase g_TargetRef;

	void EntitySpawnerLoaderCallback::OnLoaded(RTTIRefObject *Object, void *Userdata)
	{
		if constexpr (false)
			spdlog::info("Received entity spawner callback with root UUID {}", Object->m_UUID);
	}

	void EntitySpawnerLoaderCallback::OnUnloaded(RTTIRefObject *Object, void *Userdata) {}

	void EntitySpawnerWindow::Render()
	{
		ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin(GetId().c_str(), &m_WindowOpen))
		{
			ImGui::End();
			return;
		}

		// Draw entity list
		m_SpawnerNameFilter.Draw();

		if (ImGui::BeginListBox("##SpawnSetupSelector", ImVec2(-FLT_MIN, -200)))
		{
			for (size_t i = 0; i < ModConfiguration.CachedSpawnSetups.size(); i++)
			{
				const auto& spawnSetup = ModConfiguration.CachedSpawnSetups[i];

				char fullName[256] = {};
				std::format_to_n(fullName, std::size(fullName) - 1, "{}, {}", spawnSetup.UUID, spawnSetup.Name);

				if (m_SpawnerNameFilter.PassFilter(fullName))
				{
					const bool isSelected = m_LastSelectedSetupIndex == i;

					if (ImGui::Selectable(fullName, isSelected, ImGuiSelectableFlags_AllowDoubleClick))
					{
						m_LastSelectedSetupIndex = i;

						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							ForceSpawnEntityClick();
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndListBox();
		}

		// Draw settings
		static int spawnCount = 1;
		static int spawnLocationType = 0;
		static WorldPosition customSpawnPosition;
		static RTTIRefObject *customFaction = nullptr;

		const bool allowSpawn = m_LastSelectedSetupIndex < ModConfiguration.CachedSpawnSetups.size() && m_OutstandingSpawnCount == 0;

		ImGui::Separator();
		ImGui::BeginDisabled(!allowSpawn);
		ImGui::PushItemWidth(300);
		ImGui::InputInt("##entitycount", &spawnCount);
		{
			// Draw faction list
			auto& modEvents = ModCoreEvents::GetInstance();
			std::shared_lock lock(modEvents.m_CachedDataMutex);

			String previewString = "<Unset Faction>";
			
			if (!modEvents.m_CachedAIFactions.contains(customFaction))
				customFaction = nullptr;
			else
				previewString = customFaction->GetMemberRefUnsafe<String>("Name");

			if (ImGui::BeginCombo("##factioncombo", previewString.c_str()))
			{
				std::vector sortedFactions(modEvents.m_CachedAIFactions.begin(), modEvents.m_CachedAIFactions.end());

				std::ranges::sort(
					sortedFactions,
					[](auto A, auto B)
					{
						return A->GetMemberRefUnsafe<String>("Name") < B->GetMemberRefUnsafe<String>("Name");
					});

				if (ImGui::Selectable("<Unset Faction>", customFaction == nullptr))
					customFaction = nullptr;

				for (auto faction : sortedFactions)
				{
					const bool isSelected = customFaction == faction;

					if (ImGui::Selectable(faction->GetMemberRefUnsafe<String>("Name").c_str(), isSelected))
						customFaction = faction;

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
		}
		ImGui::PopItemWidth();
		ImGui::Spacing();
		ImGui::RadioButton("Spawn at player position", &spawnLocationType, 0);
		ImGui::RadioButton("Spawn at crosshair position", &spawnLocationType, 1);
		ImGui::RadioButton("Spawn at custom position", &spawnLocationType, 2);
		ImGui::Spacing();

		if (spawnLocationType == 2)
		{
			ImGui::PushItemWidth(200);
			ImGui::InputDouble("X", &customSpawnPosition.X, 1.0, 20.0, "%.3f");
			ImGui::InputDouble("Y", &customSpawnPosition.Y, 1.0, 20.0, "%.3f");
			ImGui::InputDouble("Z", &customSpawnPosition.Z, 1.0, 20.0, "%.3f");
			ImGui::PopItemWidth();
			ImGui::Spacing();
		}

		// Spawn button
		if (ImGui::Button("Spawn") || (m_DoSpawnOnNextFrame && allowSpawn))
		{
			m_NextSpawnTransform = GetSpawnTransform(spawnLocationType, customSpawnPosition);
			m_NextSpawnSelectedIndex = m_LastSelectedSetupIndex;
			m_NextFaction = customFaction;
			m_OutstandingSpawnCount = spawnCount;
		}

		ImGui::Spacing();
		ImGui::PushTextWrapPos(0.0f);
		ImGui::TextDisabled("Note: Many humanoid and scripted entities will crash the game.");
		ImGui::TextDisabled("Note: Names are missing. They can be manually added in the mod configuration file.");
		ImGui::PopTextWrapPos();
		ImGui::EndDisabled();
		ImGui::End();

		RunSpawnCommands();
		m_DoSpawnOnNextFrame = false;
	}

	bool EntitySpawnerWindow::Close()
	{
		return !m_WindowOpen;
	}

	std::string EntitySpawnerWindow::GetId() const
	{
		return "Entity Spawner";
	}

	void EntitySpawnerWindow::RunSpawnCommands()
	{
		// Faction setup logic
		if (!m_FactionSetsPending.empty())
		{
			JobHeaderCPU::SubmitCallable(
				[this]
				{
					std::scoped_lock lock(m_FactionSetupMutex);

					std::erase_if(
						m_FactionSetsPending,
						[&](const auto& Pair)
						{
							const auto getSpawnpointEntity = Offsets::Signature(
																 "40 55 48 8D 6C 24 A9 48 81 EC A0 00 00 00 48 83 ? ? ? ? ? ? 48 8B C1 75 0B 33 C0")
																 .ToPointer<Entity *(RTTIRefObject *)>();

							if (auto entity = getSpawnpointEntity(Pair.first))
							{
								entity->SetFaction(reinterpret_cast<HRZ2::AIFaction *>(Pair.second));
								return true;
							}

							return false;
						});
				});
		}

		// Streaming and spawnpoint logic
		if (m_OutstandingSpawnCount <= 0)
			return;

		const auto rootUUID = ModConfiguration.CachedSpawnSetups[m_NextSpawnSelectedIndex].RootUUID;
		const auto spawnSetupUUID = ModConfiguration.CachedSpawnSetups[m_NextSpawnSelectedIndex].UUID;

		const auto targetSpawnSetup = [&]() -> Ref<RTTIRefObject>
		{
			auto& modEvents = ModCoreEvents::GetInstance();
			std::shared_lock lock(modEvents.m_CachedDataMutex);

			auto itr = std::ranges::find_if(
				modEvents.m_CachedSpawnSetups,
				[&](const auto& Setup)
				{
					return Setup->m_UUID == spawnSetupUUID;
				});

			if (itr != modEvents.m_CachedSpawnSetups.end())
				return *itr;

			return nullptr;
		}();

		// If the setup isn't already loaded we'll have to stream the whole object group in
		if (!targetSpawnSetup && !m_StreamerRequestPending)
		{
			auto streamingManager = StreamingManager::GetInstance();

			g_TargetRef.Clear();
			streamingManager->Register2(g_TargetRef, {}, rootUUID);
			streamingManager->RegisterCallback(g_TargetRef, EStreamingRefCallbackMode::OnLoad, &m_LoaderCallback, this);
			streamingManager->Resolve(g_TargetRef, EStreamingRefPriority::Normal);

			m_StreamerRequestPending = true;
		}
		else if (targetSpawnSetup)
		{
			spdlog::debug(
				"Spawning {} entities with UUID {}",
				m_OutstandingSpawnCount,
				ModConfiguration.CachedSpawnSetups[m_NextSpawnSelectedIndex].UUID);

			JobHeaderCPU::SubmitCallable(
				[this,
				 spawnCount = m_OutstandingSpawnCount,
				 spawnSetup = targetSpawnSetup,
				 transform = m_NextSpawnTransform,
				 faction = m_NextFaction]()
				{
					const auto spawnpointRTTI = RTTI::FindTypeByName("Spawnpoint")->AsCompound();

					for (uint32_t i = 0; i < spawnCount; i++)
					{
						Ref spawnpoint = static_cast<RTTIRefObject *>(spawnpointRTTI->CreateInstance()); // TODO: MsgInit?

						spawnpointRTTI->SetMemberValue<GGUUID>(spawnpoint, "ObjectUUID", GGUUID::Generate());
						spawnpointRTTI->SetMemberValue<WorldTransform>(spawnpoint, "Orientation", transform);
						spawnpointRTTI->SetMemberValue<bool>(spawnpoint, "FactsArePersistent", false);
						spawnpointRTTI->SetMemberValue<bool>(spawnpoint, "AutoSpawn", false);
						spawnpointRTTI->SetMemberValue<Ref<RTTIRefObject>>(spawnpoint, "SpawnSetup", spawnSetup);

						const auto spawnpointSpawn = Offsets::Signature("48 85 C9 74 43 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 84 C0")
														 .ToPointer<void(RTTIRefObject *)>();
						spawnpointSpawn(spawnpoint);

						if (faction)
						{
							std::scoped_lock lock(m_FactionSetupMutex);
							m_FactionSetsPending.emplace_back(std::move(spawnpoint), faction);
						}
					}
				});

			m_OutstandingSpawnCount = 0;
			m_NextFaction = nullptr;
			m_StreamerRequestPending = false;
		}
	}

	WorldTransform EntitySpawnerWindow::GetSpawnTransform(uint32_t Type, const WorldPosition& CustomPosition)
	{
		auto player = Player::GetLocalPlayer();
		auto currentTransform = player->m_Entity->GetWorldTransform();

		if (Type == 0)
		{
			// Player position
			currentTransform.Position = player->m_Entity->m_WorldTransform.Position;
		}
		else if (Type == 1)
		{
			// Crosshair position - project forwards
			const auto cameraMatrix = player->GetLastActivatedCamera()->GetWorldTransform();
			const auto moveDirection = cameraMatrix.Orientation.Forward() * 200.0f;

			currentTransform.Position += moveDirection;

			// Raycast
			WorldPosition rayHitPosition;
			float unknownFloat;
			Entity *unknownEntity;
			void *unknownVoid;
			Vec3 normal;
			uint32_t uint1;
			uint32_t uint2;

			const auto intersectLine = Offsets::Signature("4C 8B DC 49 89 5B 10 49 89 73 18 55 57 41 54 41 55 41 57 48 8D 6C 24 90")
										   .ToPointer<bool(
											   const WorldPosition&, // a1
											   const WorldPosition&, // a2
											   int,					 // a3 EPhysicsCollisionLayerGame
											   const Entity *,		 // a4
											   bool,				 // a5
											   uint8_t,				 // a6
											   int,					 // a7
											   WorldPosition *,		 // a8
											   Vec3 *,				 // a9
											   float *,				 // a10
											   Entity **,			 // a11
											   void **,				 // a12
											   uint32_t&,			 // a13
											   uint32_t&)>();		 // a14

			intersectLine(
				cameraMatrix.Position,
				currentTransform.Position,
				47,
				nullptr,
				false,
				0,
				0,
				&rayHitPosition,
				&normal,
				&unknownFloat,
				&unknownEntity,
				&unknownVoid,
				uint1,
				uint2);

			currentTransform.Position = rayHitPosition;
		}
		else if (Type == 2)
		{
			// Custom position
			currentTransform.Position = CustomPosition;
		}

		return currentTransform;
	}

	void EntitySpawnerWindow::ForceSpawnEntityClick()
	{
		m_DoSpawnOnNextFrame = true;
	}
}
