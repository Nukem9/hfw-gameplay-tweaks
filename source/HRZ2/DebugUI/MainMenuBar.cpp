#include <algorithm>
#include <imgui.h>
#include <Windows.h>
#include "../../ModConfiguration.h"
#include "../../ModCoreEvents.h"
#include "../Core/DebugSettings.h"
#include "../Core/Destructibility.h"
#include "../Core/GameModule.h"
#include "../Core/GameWorldTimeState.h"
#include "../Core/JobHeaderCPU.h"
#include "../Core/Mover.h"
#include "../Core/PlayerGame.h"
#include "../Core/RTTIRefObject.h"
#include "../Core/RTTIYamlExporter.h"
#include "../RTTIScanner.h"
#include "DebugUI.h"
#include "DemoWindow.h"
#include "EntitySpawnerWindow.h"
#include "LogWindow.h"
#include "MainMenuBar.h"
#include "PlayerInventoryWindow.h"
#include "WeatherSetupWindow.h"

namespace HRZ2::DebugUI
{
	MainMenuBar::MainMenuBar()
	{
		m_LODRangeModifier = ModCoreEvents::GetInstance().m_CachedLODRangeModifierHack;

		m_EnableGodMode = ModConfiguration.EnableGodMode;
		m_EnableInfiniteClipAmmo = ModConfiguration.EnableInfiniteClipAmmo;
		m_EnableAutoNeutralFaction = ModConfiguration.EnableAutoNeutralFaction;
	}

	void MainMenuBar::Render()
	{
		if (!m_IsVisible || !ImGui::BeginMainMenuBar())
			return;

		// Empty space for MSI afterburner display
		ImGui::BeginMenu("                        ", false);

		// "Gameplay" menu
		if (ImGui::BeginMenu("Gameplay"))
		{
			DrawGameplayMenu();
			ImGui::EndMenu();
		}

		// "Cheats" menu
		if (ImGui::BeginMenu("Cheats", Player::GetLocalPlayer() != nullptr))
		{
			DrawCheatsMenu();
			ImGui::EndMenu();
		}

		// "Miscellaneous" menu
		if (ImGui::BeginMenu("Miscellaneous"))
		{
			DrawMiscellaneousMenu();
			ImGui::EndMenu();
		}

		// Credits
		XorStr creditsBuf("Game keyboard input blocked | HFW Gameplay Tweaks & Cheat Menu by Nukem\0");
		auto credits = creditsBuf.Decrypt();

		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(credits.data()).x);
		ImGui::BeginMenu(credits.data(), false);

		ImGui::EndMainMenuBar();
	}

	bool MainMenuBar::Close()
	{
		return false;
	}

	std::string MainMenuBar::GetId() const
	{
		return "Main Menu Bar";
	}

	void MainMenuBar::DrawGameplayMenu()
	{
		ImGui::MenuItem("Pause Game Logic", nullptr, &m_PauseGame);
		if (ImGui::MenuItem("Pause AI Processing", nullptr, m_PauseAIProcessing))
			TogglePauseAIProcessing();
		ImGui::MenuItem("##sep1", nullptr, nullptr, false);

		if (ImGui::MenuItem("Force Quick Save"))
			ToggleQuickSave();

		if (ImGui::MenuItem("Force Load Previous Save"))
			ToggleQuickLoad();

		// Day/night cycle
		if (auto gameModule = GameModule::GetInstance())
		{
			if (auto worldTimeState = gameModule->m_WorldTimeState)
			{
				ImGui::MenuItem("##sep2", nullptr, nullptr, false);

				if (ImGui::MenuItem("Pause Time of Day", nullptr, worldTimeState->m_IsPaused))
					TogglePauseTimeOfDay();

				if (ImGui::MenuItem("Pause Day/Night Cycle", nullptr, !worldTimeState->m_EnableDayNightCycle))
					worldTimeState->m_EnableDayNightCycle = !worldTimeState->m_EnableDayNightCycle;

				ImGui::MenuItem("Time of Day", nullptr, nullptr, false);
				float timeOfDay = worldTimeState->m_TimeOfDay;

				if (ImGui::SliderFloat("##timeofdaybar", &timeOfDay, 0.0f, 23.9999f))
					worldTimeState->SetTimeOfDay(timeOfDay, 0.0f);
			}
		}

		// Timescale
		ImGui::MenuItem("##sep3", nullptr, nullptr, false);
		ImGui::MenuItem("Enable Timescale Override in Menus", nullptr, &m_TimescaleOverrideInMenus);
		if (ImGui::MenuItem("Enable Timescale Override", nullptr, m_TimescaleOverride))
			ToggleTimescaleOverride();
		ImGui::MenuItem("Timescale", nullptr, nullptr, false);

		auto modifyTimescale = [](float Scale, bool SameLine = true)
		{
			char temp[64];
			sprintf_s(temp, "%g##setTs%g", Scale, Scale);

			if (ImGui::Button(temp))
				AdjustTimescale(Scale - m_Timescale);

			if (SameLine)
				ImGui::SameLine();
		};

		if (float f = m_Timescale; ImGui::SliderFloat("##TimescaleDragFloat", &f, 0.001f, 10.0f))
			AdjustTimescale(f - m_Timescale);

		modifyTimescale(0.01f);
		modifyTimescale(0.25f);
		modifyTimescale(0.5f);
		modifyTimescale(1.0f);
		modifyTimescale(2.0f);
		modifyTimescale(5.0f);
		modifyTimescale(10.0f, false);
	}

	void MainMenuBar::DrawCheatsMenu()
	{
		auto debugSettings = DebugSettings::GetInstance();
		auto player = Player::GetLocalPlayer();

		if (!player || !player->m_Entity)
			return;

		if (ImGui::MenuItem("Enable Noclip", nullptr, m_FreeCamMode == FreeCamMode::Noclip))
			ToggleNoclip();

		if (ImGui::MenuItem("Enable Free Camera", nullptr, m_FreeCamMode == FreeCamMode::Free))
			ToggleFreeflyCamera();

		if (auto destructibility = player->m_Entity->m_Destructibility)
		{
			if (ImGui::MenuItem("Enable God Mode", nullptr, &m_EnableGodMode))
			{
				m_EnableDemigodMode = false;
				destructibility->m_Invulnerable = m_EnableGodMode;
				destructibility->m_DieAtZeroHealth = true;
			}

			if (ImGui::MenuItem("Enable Demigod Mode", nullptr, &m_EnableDemigodMode))
			{
				m_EnableGodMode = false;
				destructibility->m_Invulnerable = false;
				destructibility->m_DieAtZeroHealth = !m_EnableDemigodMode;
			}
		}

		if (ImGui::MenuItem("Enable Infinite Ammo", nullptr, &debugSettings->m_InfiniteAmmo))
		{
			debugSettings->m_InfiniteSizeClip = false;
			m_EnableInfiniteClipAmmo = false;
		}

		if (ImGui::MenuItem("Enable Infinite Ammo (Clip)", nullptr, &m_EnableInfiniteClipAmmo))
		{
			debugSettings->m_InfiniteAmmo = false;
			debugSettings->m_InfiniteSizeClip = m_EnableInfiniteClipAmmo;
		}

		ImGui::MenuItem("Enable Infinite Stamina", nullptr, &debugSettings->m_Inexhaustible);
		ImGui::MenuItem("##sep1", nullptr, nullptr, false);

		if (ImGui::BeginMenu("Teleport To..."))
		{
			auto doTeleport = [&](const char *Name, WorldPosition Position)
			{
				if (!ImGui::MenuItem(std::format("{} ({:.1f}, {:.1f}, {:.1f})", Name, Position.X, Position.Y, Position.Z).c_str()))
					return;

				// Fixup so Aloy doesn't fall through the ground
				Position.Z += 0.5;

				if (m_FreeCamMode == FreeCamMode::Noclip)
				{
					m_FreeCamPosition.Position = Position;
				}
				else
				{
					JobHeaderCPU::SubmitCallable([Position]()
					{
						if (auto player = Player::GetLocalPlayer())
						{
							auto worldTransform = player->m_Entity->GetWorldTransform();
							worldTransform.Position = Position;

							player->m_Entity->m_Mover->OverrideMovement(worldTransform, 0.0001f, false);
						}
					});
				}
			};

			doTeleport("Free Camera Position", m_FreeCamPosition.Position);
			ImGui::MenuItem("##septeleport1", nullptr, nullptr, false);
			doTeleport("HZD - Meridian Entrance", { 3918.612, 5465.897, 830.652 });
			doTeleport("HZD - The Spire", { 4162.499, 4757.230, 774.453 });
			doTeleport("HZD - The Estate", { 4082.374, 4614.900, 710.106 });
			doTeleport("HZD - Gatelands", { 4695.546, 5482.825, 787.018 });
			doTeleport("HZD - Lone Light", { 4772.899, 5777.062 , 780.938 });
			doTeleport("HZD - Blazon Arch", { 3086.388, 5931.355, 813.258 });
			doTeleport("HZD - Cauldron ZETA", { 4050.798, 6724.117, 840.551 });
			doTeleport("HZD - Sunfall Arena", { 3009.000, 6565.725, 868.025 });
			doTeleport("HZD - Maker's End", { 3435.481, 7210.677, 825.362 });
			doTeleport("HZD - The Daunt", { 1662.439, 5811.224, 794.055 });
			doTeleport("HZD - The Glarebreak", { 5260.463, 6411.703, 820.775 });
			ImGui::MenuItem("##septeleport2", nullptr, nullptr, false);
			doTeleport("HFW - Intro Cutscene 1", { 5737.400, -2394.600, 432.400 });
			doTeleport("HFW - Intro Cutscene 2", { 6315.800, -1698.400, 320.100 });
			doTeleport("HFW - Intro Cutscene 3", { 6942.200, -1713.900, 321.000 });
			doTeleport("HFW - Intro Cutscene 4 (Tree Dream)", { 2691.584, -3752.680, 477.490 });
			doTeleport("HFW - Intro Tutorial Area", { 5645.362, -2886.453, 403.551 });
			doTeleport("HFW - Intro Far Zenith Facility", { 5993.999, -2915.253, 334.978 });
			doTeleport("HFW - Intro Far Zenith Space Shuttle", { 6465.844, -3147.694, 331.482 });
			doTeleport("HFW - Barren Light Mountain Fort", { 2690.644, 441.522, 677.341 });
			doTeleport("HFW - Burning Shores Boss Battle Area", { -140.415, -4405.686, 291.182 });
			doTeleport("HFW - Burning Shores Unfinished Area", { 2107.752, -6777.761, 302.759 });
			doTeleport("HFW - Apex Hunt: The Westshallows", { -4531.792, -460.949, 185.629 });
			doTeleport("HFW - Apex Hunt: The Graypeak", { -1452.500, -614.100, 520.100 });
			doTeleport("HFW - Far Zenith Base", { -1417.900, -3088.700, 283.900 });

			ImGui::EndMenu();
		}

		// Faction
		if (ImGui::BeginMenu("Player Faction..."))
		{
			auto& modEvents = ModCoreEvents::GetInstance();
			std::shared_lock lock(modEvents.m_CachedDataMutex);
			std::vector sortedFactions(modEvents.m_CachedAIFactions.begin(), modEvents.m_CachedAIFactions.end());

			std::ranges::sort(
				sortedFactions,
				[](auto A, auto B)
				{
					return A->GetMemberRefUnsafe<String>("Name") < B->GetMemberRefUnsafe<String>("Name");
				});

			for (auto faction : sortedFactions)
			{
				if (ImGui::MenuItem(
					faction->GetMemberRefUnsafe<String>("Name").c_str(),
					nullptr,
					reinterpret_cast<RTTIRefObject *>(player->m_Entity->m_Faction) == faction))
				{
					m_EnableAutoNeutralFaction = false;

					JobHeaderCPU::SubmitCallable([factionRef = Ref(faction)]
					{
						if (auto player = Player::GetLocalPlayer(); player && player->m_Entity)
							player->m_Entity->SetFaction(reinterpret_cast<AIFaction *>(factionRef.GetPtr()));
					});
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Player Inventory..."))
			AddWindow(std::make_shared<PlayerInventoryWindow>());

		if (ImGui::MenuItem("Entity Spawner..."))
			AddWindow(std::make_shared<EntitySpawnerWindow>());

		if (ImGui::MenuItem("Weather Setup..."))
			AddWindow(std::make_shared<WeatherSetupWindow>());

		ImGui::MenuItem("##sep2", nullptr, nullptr, false);
		ImGui::MenuItem("Simulate Game Completed", nullptr, &debugSettings->m_SPAllUnlocked);
		ImGui::MenuItem("Apply Photomode Settings Ingame", nullptr, &debugSettings->m_ApplyPhotoModeSettingsIngame);
	}

	void MainMenuBar::DrawMiscellaneousMenu()
	{
		if (ImGui::MenuItem("Show Log Window"))
			AddWindow(std::make_shared<LogWindow>());

		if (ImGui::MenuItem("Show ImGui Demo Window"))
			AddWindow(std::make_shared<DemoWindow>());

		if (ImGui::MenuItem("Dump RTTI Structures", nullptr, false, !RTTIScanner::GetAllTypes().empty()))
		{
			RTTIYamlExporter exporter(RTTIScanner::GetAllTypes());
			exporter.ExportRTTITypes(".");
		}

#if 0
		if (ImGui::MenuItem("Dump Fullgame Typeinfo", nullptr, false, false))
		{
		}
#endif

		if (ImGui::MenuItem("Dump Player Components"))
		{
			if (auto player = Player::GetLocalPlayer())
			{
				if (auto entity = player->m_Entity)
				{
					spdlog::info("Player RTTI: '{}' UUID: '{}'", entity->GetRTTI()->GetSymbolName(), entity->m_UUID);

					if (auto resource = entity->m_EntityResource.GetUntypedPtr())
						spdlog::info("\tResource RTTI: '{}' UUID: '{}'", resource->GetRTTI()->GetSymbolName(), resource->m_UUID);

					spdlog::info("");

					for (size_t i = 0; i < entity->m_Components.m_Components.size(); i++)
					{
						const auto& component = entity->m_Components.m_Components[i];

						spdlog::info(
							"Component {}: RTTI: '{}' UUID: '{}' (0x{:X})",
							i,
							component->GetRTTI()->GetSymbolName(),
							component->m_UUID,
							reinterpret_cast<uintptr_t>(component));

						if (auto resource = reinterpret_cast<HRZ2::RTTIRefObject *>(component->m_Resource.GetPtr()))
							spdlog::info(
								"\tResource RTTI: '{}' UUID: '{}' (0x{:X})",
								resource->GetRTTI()->GetSymbolName(),
								resource->m_UUID,
								reinterpret_cast<uintptr_t>(resource));
					}
				}
			}
		}

#if 0
		{
			static bool instrumentationActive = false;
			static std::unordered_set<void *> resourcesPresentAtStart;
			static size_t currentRootIndex = std::numeric_limits<size_t>::max();
			static size_t targetRootIndex = std::numeric_limits<size_t>::max();
			static size_t loadingRootIndex = std::numeric_limits<size_t>::max();
			static StreamingRefBase streamingRefRoot;

			if (ImGui::MenuItem("Begin Instrumentation", nullptr, false, false) || instrumentationActive)
			{
				auto& modEvents = ModCoreEvents::GetInstance();
				auto& resourceList = modEvents.m_CachedInventoryItems;

				// Gather the initial resources available in the main menu
				if (!instrumentationActive)
				{
					streamingRefRoot.Clear();
					resourcesPresentAtStart.clear();

					std::shared_lock lock(modEvents.m_CachedDataMutex);
					resourcesPresentAtStart.insert(resourceList.begin(), resourceList.end());

					instrumentationActive = true;
					targetRootIndex = 0;
				}

				// Perform a diff with a new group finishes loading
				if (currentRootIndex != targetRootIndex && streamingRefRoot.GetUntypedPtr())
				{
					std::shared_lock lock(modEvents.m_CachedDataMutex);
					for (const auto& resource : resourceList)
					{
						if (!resourcesPresentAtStart.contains(resource))
						{
							auto& nameRef = resource->GetMemberRefUnsafe<Ref<RTTIRefObject>>("ItemName");

							if (nameRef)
							{
								const auto func = Offsets::Signature("48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 30 48 8B 05")
													  .ToPointer<String *(RTTIRefObject *, String *)>();

								String str; // ctor is wrong
								func(nameRef, &str);

								spdlog::info(
									"InventoryItem {{\"UUID\": \"{}\", \"RootUUID\": \"{}\", \"Name\": \"{}\"}}",
									resource->m_UUID,
									streamingRefRoot.GetUUID(),
									str);
							}
						}
					}

					currentRootIndex = targetRootIndex;
					targetRootIndex += 1;
				}

				// Queue the next load
				if (loadingRootIndex != targetRootIndex)
				{
					spdlog::info("Loading root index {}", targetRootIndex);

					auto streamingManager = StreamingManager::GetInstance();
					streamingRefRoot.Clear();
					streamingManager->Register2(streamingRefRoot, {}, ModConfiguration.CachedRoots[targetRootIndex].RootUUID);
					streamingManager->Resolve(streamingRefRoot, EStreamingRefPriority::AboveNormal);

					loadingRootIndex = targetRootIndex;
				}
			}
		}
#endif

		ImGui::MenuItem("##blankseparator0", nullptr, nullptr, false);

		// LOD bias
		if (ImGui::MenuItem("Enable LOD Bias Override", nullptr, m_LODRangeModifier != std::numeric_limits<float>::max()))
			m_LODRangeModifier = m_LODRangeModifier == std::numeric_limits<float>::max() ? 1.0f : std::numeric_limits<float>::max();
		ImGui::MenuItem("Bias", nullptr, nullptr, false);
		if (float t = m_LODRangeModifier == std::numeric_limits<float>::max() ? 1.0f : m_LODRangeModifier;
			ImGui::SliderFloat("##LODDragFloat", &t, 0.0f, 1.0f))
			m_LODRangeModifier = t;

		ImGui::MenuItem("##blankseparator1", nullptr, nullptr, false);
		ImGui::MenuItem("##blankseparator2", nullptr, nullptr, false);

		if (ImGui::MenuItem("Terminate Process"))
			TerminateProcess(GetCurrentProcess(), 0);
	}

	void MainMenuBar::ToggleVisibility()
	{
		m_IsVisible = !m_IsVisible;
	}

	void MainMenuBar::TogglePauseGameLogic()
	{
		m_PauseGame = !m_PauseGame;
	}

	void MainMenuBar::TogglePauseAIProcessing()
	{
		m_PauseAIProcessing = !m_PauseAIProcessing;
	}

	void MainMenuBar::TogglePauseTimeOfDay()
	{
		JobHeaderCPU::SubmitCallback([]()
		{
			if (auto gameModule = GameModule::GetInstance())
			{
				if (auto worldTimeState = gameModule->m_WorldTimeState)
					worldTimeState->m_IsPaused = !worldTimeState->m_IsPaused;
			}
		});
	}

	void MainMenuBar::ToggleQuickSave()
	{
		JobHeaderCPU::SubmitCallback([]()
		{
			auto player = static_cast<PlayerGame *>(Player::GetLocalPlayer());

			if (!player)
				return;

			const auto func = Offsets::Signature("40 57 48 83 EC 50 4C 8B 15 ? ? ? ? 4D 8B D9 41 0F B6 F8 4D 85 D2")
								  .ToPointer<void(uint8_t, bool, bool, const GGUUID&)>();

			player->m_RestartOnSpawned = true;
			func(2, false, false, {}); // ESaveGameType::Quick
		});
	}

	void MainMenuBar::ToggleQuickLoad()
	{
		JobHeaderCPU::SubmitCallback([]()
		{
			if (!Player::GetLocalPlayer())
				return;

			const auto func = Offsets::Signature("40 55 57 48 8D 6C 24 B1 48 81 EC 88 00 00 00 48 8B 05")
				.ToPointer<void(float, uint8_t)>();

			func(0.0f, 1); // ESaveGameRestoreReason::Manual
		});
	}

	void MainMenuBar::ToggleTimescaleOverride()
	{
		m_TimescaleOverride = !m_TimescaleOverride;
	}

	void MainMenuBar::AdjustTimescale(float Adjustment)
	{
		m_Timescale = std::max(m_Timescale + Adjustment, 0.001f);
		m_TimescaleOverride = true;
	}

	void MainMenuBar::ToggleFreeflyCamera()
	{
		auto player = Player::GetLocalPlayer();
		auto camera = player ? player->GetLastActivatedCamera() : nullptr;

		if (!camera)
			return;

		m_FreeCamMode = (m_FreeCamMode == FreeCamMode::Free) ? FreeCamMode::Off : FreeCamMode::Free;

		if (m_FreeCamMode == FreeCamMode::Free)
			m_FreeCamPosition = camera->GetWorldTransform();
	}

	void MainMenuBar::ToggleNoclip()
	{
		auto player = Player::GetLocalPlayer();
		auto entity = player ? player->m_Entity : nullptr;

		if (!entity)
			return;

		m_FreeCamMode = (m_FreeCamMode == FreeCamMode::Noclip) ? FreeCamMode::Off : FreeCamMode::Noclip;
		m_FreeCamPosition = entity->GetWorldTransform();
	}
}
