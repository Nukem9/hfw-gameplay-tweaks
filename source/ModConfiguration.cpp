#include <spdlog/sinks/basic_file_sink.h>
#include <toml++/toml.h>
#include "HRZ2/DebugUI/LogWindow.h"
#include "ModConfiguration.h"

namespace InternalModConfig
{
	InternalModConfig::GlobalSettings& GetModifiableSettings()
	{
		static GlobalSettings configuration;
		return configuration;
	}

	std::filesystem::path& GetModifiableRootDirectory()
	{
		static std::filesystem::path rootDirectory;
		return rootDirectory;
	}
}

const InternalModConfig::GlobalSettings& ModConfiguration = InternalModConfig::GetModifiableSettings();

namespace InternalModConfig
{
	void ParseSettings(GlobalSettings& s, const toml::table& Table);
	void PostProcessSettings(GlobalSettings& s);

	void Initialize(const std::filesystem::path& RootDirectory)
	{
		GetModifiableRootDirectory() = RootDirectory;

		if (!InitializeConfigurationFile())
			throw std::runtime_error("Failed to load configuration file.");

		// Main logger (debug UI and log sink)
		if (auto logLevel = spdlog::level::debug; logLevel != spdlog::level::off)
		{
			std::vector<spdlog::sink_ptr> sinks;

			auto debugUISink = std::make_shared<HRZ2::DebugUI::LogWindow::LogSink>();
			sinks.emplace_back(std::move(debugUISink));

			if constexpr (false)
			{
				auto logFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(GetModRelativePath("mod_log.txt").string(), true);
				sinks.emplace_back(std::move(logFileSink));
			}

			auto logger = std::make_shared<spdlog::logger>("main_logger", sinks.begin(), sinks.end());
			logger->set_level(logLevel);
			logger->flush_on(spdlog::level::debug);
			logger->set_pattern("[%H:%M:%S] [%l] %v");

			spdlog::set_default_logger(std::move(logger));
		}

		if (!Offsets::Initialize())
			throw std::runtime_error("Failed to initialize offsets. The game likely updated and is no longer supported.");

		if (!Hooks::Initialize())
			throw std::runtime_error("Failed to initialize hooks.");
	}

	bool InitializeConfigurationFile()
	{
		auto tryParseTomlFile = [](GlobalSettings& Settings, const std::filesystem::path& Path)
		{
			toml::table table;

			try
			{
				table = toml::parse_file(Path.string());
			}
			catch (const toml::parse_error&)
			{
				return false;
			}

			ParseSettings(Settings, table);
			return true;
		};

		// mod_config.ini takes precedence
		if (!tryParseTomlFile(GetModifiableSettings(), GetModRelativePath("mod_config.ini")))
			return false;

		// Then check for mod_*.ini files in the game directory
		for (const auto& entry : std::filesystem::directory_iterator(GetModRelativePath("")))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".ini")
				continue;

			if (!entry.path().filename().string().starts_with("mod_") || entry.path().filename().string() == "mod_config.ini")
				continue;

			if (!tryParseTomlFile(GetModifiableSettings(), entry.path()))
				return false;
		}

		PostProcessSettings(GetModifiableSettings());
		return true;
	}

	std::filesystem::path GetModRelativePath(const std::string_view& PartialPath)
	{
		auto path = GetModifiableRootDirectory();
		path.append(PartialPath);

		return path;
	}

	void ParseSettings(GlobalSettings& o, const toml::table& Table)
	{
#define PARSE_TOML_MEMBER(obj, x) o.x = (*obj)[#x].value_or(decltype(o.x) {})
#define PARSE_TOML_HOTKEY(obj, x) o.Hotkeys.x = (*obj)[#x].value_or(-1)

		// [General]
		if (auto general = Table["General"].as_table())
		{
			PARSE_TOML_MEMBER(general, EnableDebugMenu);
			PARSE_TOML_MEMBER(general, EnableAssetLogging);
			PARSE_TOML_MEMBER(general, EnableAssetOverrides);
			PARSE_TOML_MEMBER(general, DebugMenuFontScale);
		}

		if (o.DebugMenuFontScale <= 0.0f)
			o.DebugMenuFontScale = 1.0f;

		// [Gameplay]
		if (auto gameplay = Table["Gameplay"].as_table())
		{
			PARSE_TOML_MEMBER(gameplay, UnlockMapBoundaries);
			PARSE_TOML_MEMBER(gameplay, UnlockWorldMapMenu);
			PARSE_TOML_MEMBER(gameplay, UnlockMountRestrictions);
			PARSE_TOML_MEMBER(gameplay, UnlockEntitlementExtras);
			PARSE_TOML_MEMBER(gameplay, UnlockPhotomodeRestrictions);
			PARSE_TOML_MEMBER(gameplay, UnlockAIvsAIDamage);
			PARSE_TOML_MEMBER(gameplay, UnlockUltraHardRestrictions);
			PARSE_TOML_MEMBER(gameplay, UnlockTrapAndTripwireLimits);

			PARSE_TOML_MEMBER(gameplay, EnableFreePerks);
			PARSE_TOML_MEMBER(gameplay, EnableFreeCrafting);
			PARSE_TOML_MEMBER(gameplay, EnableGodMode);
			PARSE_TOML_MEMBER(gameplay, EnableInfiniteClipAmmo);
			PARSE_TOML_MEMBER(gameplay, EnableInfiniteOverrideTimer);
			PARSE_TOML_MEMBER(gameplay, EnableAutoNeutralFaction);

			PARSE_TOML_MEMBER(gameplay, IncreaseInventoryStacks);
			PARSE_TOML_MEMBER(gameplay, InventoryStackMultiplier);

			PARSE_TOML_MEMBER(gameplay, DisableCameraMagnetism);
			PARSE_TOML_MEMBER(gameplay, DisableFocusMagnetism);

			PARSE_TOML_MEMBER(gameplay, ForceCustomUltraHard);
			PARSE_TOML_MEMBER(gameplay, ForceNoShaderLoadingScreen);
		}

		// [Hotkeys]
		if (auto hotkeys = Table["Hotkeys"].as_table())
		{
			PARSE_TOML_HOTKEY(hotkeys, ToggleDebugUI);
			PARSE_TOML_HOTKEY(hotkeys, TogglePauseGameLogic);
			PARSE_TOML_HOTKEY(hotkeys, ToggleFreeCamera);
			PARSE_TOML_HOTKEY(hotkeys, ToggleNoclip);
			PARSE_TOML_HOTKEY(hotkeys, IncreaseTimescale);
			PARSE_TOML_HOTKEY(hotkeys, DecreaseTimescale);
			PARSE_TOML_HOTKEY(hotkeys, ToggleTimescale);
			PARSE_TOML_HOTKEY(hotkeys, SpawnEntity);
			PARSE_TOML_HOTKEY(hotkeys, QuickSave);
			PARSE_TOML_HOTKEY(hotkeys, QuickLoad);
			PARSE_TOML_HOTKEY(hotkeys, ToggleAIProcessing);
			PARSE_TOML_HOTKEY(hotkeys, TogglePauseTimeOfDay);
		}

		// [AssetOverride]
		if (auto overrideArray = Table["AssetOverride"].as_array())
		{
			auto parseAssetOverride = [](const toml::table *Table, bool Enable)
			{
				AssetOverride o;

				o.Enabled = Enable;
				PARSE_TOML_MEMBER(Table, ObjectUUIDs);
				PARSE_TOML_MEMBER(Table, ObjectTypes);
				PARSE_TOML_MEMBER(Table, Path);
				PARSE_TOML_MEMBER(Table, Value);

				return o;
			};

			overrideArray->for_each(
				[&](const toml::table& t)
				{
					const bool enabled = t["Enable"].value_or(false);

					if (auto overridePatchArray = t["Patch"].as_array())
					{
						for (const auto& patchEntry : *overridePatchArray)
							o.AssetOverrides.emplace_back(parseAssetOverride(patchEntry.as_table(), enabled));
					}
				});
		}

		// [CharacterOverride]
		if (auto overrideArray = Table["CharacterOverride"].as_array())
		{
			overrideArray->for_each(
				[&](const toml::table& t)
				{
					o.CharacterOverrides.emplace_back(
						HRZ2::GGUUID::Parse(t["RootUUID"].value_or("")),
						HRZ2::GGUUID::Parse(t["VariantUUID"].value_or("")));
				});
		}

		// [DamageDealtModifiers]
		if (auto overrideArray = Table["DamageDealtModifiers"]["Overrides"].as_array())
		{
			o.DamageModifierOverrides.clear();
			o.DamageModifierOverrides.reserve(overrideArray->size());

			overrideArray->for_each([&](const toml::array& a)
			{
				o.DamageModifierOverrides.emplace_back(HRZ2::GGUUID::Parse(a[0].value_or("")), a[1].value_or(1.0f));
			});
		}

		// [CoreObjectCache]
		auto parseCoreObjectCacheTable = [&Table](const std::string_view& Name, auto& TargetVector)
		{
			if (auto cachedObjectArray = Table["CoreObjectCache"][Name].as_array())
			{
				TargetVector.clear();
				TargetVector.reserve(cachedObjectArray->size());

				cachedObjectArray->for_each(
					[&](const toml::array& a)
					{
						TargetVector.emplace_back(
							a.size() > 0 ? HRZ2::GGUUID::Parse(a[0].value_or("")) : HRZ2::GGUUID {},
							a.size() > 1 ? HRZ2::GGUUID::Parse(a[1].value_or("")) : HRZ2::GGUUID {},
							a.size() > 2 ? a[2].value_or("") : "");
					});

				std::stable_sort(TargetVector.begin(), TargetVector.end());
			}
		};

		parseCoreObjectCacheTable("CachedSpawnSetups", o.CachedSpawnSetups);
		parseCoreObjectCacheTable("CachedWeatherSetups", o.CachedWeatherSetups);
		parseCoreObjectCacheTable("CachedInventoryItems", o.CachedInventoryItems);
		parseCoreObjectCacheTable("CachedDialogueBlacklist", o.CachedDialogueBlacklist);
		parseCoreObjectCacheTable("CachedRoots", o.CachedRoots);

#undef PARSE_TOML_HOTKEY
#undef PARSE_TOML_MEMBER
	}

	void PostProcessSettings(GlobalSettings& s)
	{
		// clang-format off
		if (!s.EnableAssetOverrides)
			s.AssetOverrides.clear();

		// UnlockMapBoundaries
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockMapBoundaries,
			.ObjectUUIDs = "BA5AB7E8-E4CC-0D41-88CD-A9681E5DF4A3", // GraphProgramResource
			.Path = "EntryPoints[2].EntryPoint",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockMapBoundaries,
			.ObjectTypes = "OutOfBoundsQueryComponentResource",
			.Path = "OutOfBoundsAreaTags",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockMapBoundaries,
			.ObjectUUIDs = "E8E912B6-17E3-8344-959D-A867A9F467A1", // RiddenMountInAirAvoidanceResource
			.Path = "PhysicsCollisionLayer", // 2DE5F274-1BF7-6E40-8732-2FFF2AC61DA5 - map barriers and settlements
			.Value = "No Collision",
		});

		// UnlockWorldMapMenu
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockWorldMapMenu,
			.ObjectTypes = "DiscoverableArea",
			.Path = "InitialState",
			.Value = "Completed",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockWorldMapMenu,
			.ObjectTypes = "DiscoverableAreaResource",
			.Path = "FogOfWarRevealType",
			.Value = "Full",
		});

		// UnlockMountRestrictions
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockMountRestrictions,
			.ObjectTypes = "HorseControllerComponentResource,HorseCallComponentResource,GroundMountCallComponentResource,AirMountCallComponentResource",
			.Path = "HorseNotAllowedNavMeshAreaTags",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockMountRestrictions,
			.ObjectTypes = "RiddenMountControllerComponentResource",
			.Path = "DisallowedNavMeshAreaTags",
			.Value = "",
		});

		// UnlockPhotomodeRestrictions
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockPhotomodeRestrictions,
			.ObjectTypes = "PhotoModeResource",
			.Path = "MaxCameraRadius",
			.Value = "1000000",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockPhotomodeRestrictions,
			.ObjectUUIDs = "9D995414-CEF7-6830-AAFA-87CCF64E0448", // GraphProgramResource
			.Path = "EntryPoints[0].EntryPoint",
			.Value = "EntryPoint_EQ12_01_ArmadilloFight_ConditionGraph_019fa88caf88265ff2c20624746b9a87_0_Evaluate",
		});

		// UnlockAIvsAIDamage
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockAIvsAIDamage,
			.ObjectTypes = "AIManagerResourceGame",
			.Path = "@IgnoredAttackEventTagFilter",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockAIvsAIDamage,
			.ObjectTypes = "AIManagerResourceGame",
			.Path = "@IgnoredAttacksOnAIFilter",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockAIvsAIDamage,
			.ObjectTypes = "ReduceIncomingAIDamageComponentResource",
			.Path = "@NotDamagedRecentlyDamageResource", // NotDamagedRecentlyDamageResource = DefaultDamageResource
			.Value = "@DefaultDamageResource",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockAIvsAIDamage,
			.ObjectTypes = "ReduceIncomingAIDamageResource",
			.Path = "DenyKillingBlow",
			.Value = "false",
		});

		// UnlockUltraHardRestrictions
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockUltraHardRestrictions,
			.ObjectTypes = "DifficultyManagerResource",
			.Path = "LockedDifficulties",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockUltraHardRestrictions,
			.ObjectTypes = "DataSourceSettingsResource",
			.Path = "@UltraHardDifficulty",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockUltraHardRestrictions,
			.ObjectTypes = "DifficultyPresetResource",
			.Path = "IsConcentrationModifierAllowed",
			.Value = "true",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockUltraHardRestrictions,
			.ObjectTypes = "DifficultyPresetResource",
			.Path = "IsAutoConcentrationAllowed",
			.Value = "true",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockUltraHardRestrictions,
			.ObjectTypes = "DifficultyPresetResource",
			.Path = "IsAutoHealAllowed",
			.Value = "true",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockUltraHardRestrictions,
			.ObjectTypes = "DifficultyPresetResource",
			.Path = "IsTrajectoryAssistAllowed",
			.Value = "true",
		});

		// UnlockTrapAndTripwireLimits
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockTrapAndTripwireLimits,
			.ObjectUUIDs = "2AC1A900-3547-9F47-877E-F46601C3675F", // PlaceableItemLimiterComponentResource
			.Path = "PlaceableItemLimits",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.UnlockTrapAndTripwireLimits,
			.ObjectUUIDs = "38EE0EBE-FA3E-734F-A09D-DD211FA175B6", // HUDVitalStatusComponentResource
			.Path = "@TrapAndTripwireLimitReachedMessage",
			.Value = "@LowAmmoMessage",
		});

		// EnableFreePerks
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.EnableFreePerks,
			.ObjectTypes = "PerkLevel",
			.Path = "Cost",
			.Value = "0",
		});

		// EnableFreeCrafting
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.EnableFreeCrafting,
			.ObjectTypes = "CraftableItemComponentResource,CraftableItemSettings",
			.Path = "Ingredients.Requirements",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.EnableFreeCrafting,
			.ObjectTypes = "CraftableItemComponentResource,CraftableItemSettings",
			.Path = "DefaultIngredients.Requirements",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.EnableFreeCrafting,
			.ObjectTypes = "CraftableItemComponentResource,CraftableItemSettings",
			.Path = "ConditionalIngredients",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.EnableFreeCrafting,
			.ObjectTypes = "TransactionRequirementsContainer",
			.Path = "Requirements",
			.Value = "",
		});

		// EnableInfiniteOverrideTimer
		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = s.EnableInfiniteOverrideTimer,
			.ObjectTypes = "HackableComponentResource",
			.Path = "BaseHackDuration",
			.Value = "10000000",
		});

		const bool characterOverrideEnabled = !s.CharacterOverrides.empty();

		s.AssetOverrides.emplace_back(AssetOverride // Delete armor model parts
		{
			.Enabled = characterOverrideEnabled,
			.ObjectTypes = "OutfitStreamingData",
			.Path = "ModelParts",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride // Delete Aloy's hair
		{
			.Enabled = characterOverrideEnabled,
			.ObjectUUIDs = "1040F13B-33B7-163B-AD58-3F4F03F1489E",
			.Path = "Poses",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride // Delete skin/face tints and masks (vanilla uuidref override)
		{
			.Enabled = characterOverrideEnabled,
			.ObjectUUIDs = "20F405C6-E25F-7B41-B988-58EDC2756C8A",
			.Path = "ShaderOverrides.ShaderTextureOverrides",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = characterOverrideEnabled,
			.ObjectUUIDs = "20F405C6-E25F-7B41-B988-58EDC2756C8A",
			.Path = "ShaderOverrides.ShaderVariableOverrides",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride // Delete skin/face tints and masks (armor override)
		{
			.Enabled = characterOverrideEnabled,
			.ObjectTypes = "PlayerOutfitTheme",
			.Path = "ShaderEffects",
			.Value = "",
		});

		s.AssetOverrides.emplace_back(AssetOverride
		{
			.Enabled = characterOverrideEnabled,
			.ObjectTypes = "PlayerOutfitTheme",
			.Path = "DefaultShaderEffect.ShaderOverrides",
			.Value = "",
		});
		// clang-format on
	}
}
