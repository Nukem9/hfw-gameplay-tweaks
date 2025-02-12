#include <format>
#include <ranges>
#include <shared_mutex>
#include "HRZ2/Core/EntityComponent.h"
#include "HRZ2/Core/IStreamingManager.h"
#include "HRZ2/Core/JobHeaderCPU.h"
#include "HRZ2/Core/ProgramResource.h"
#include "HRZ2/Core/RTTI.h"
#include "HRZ2/Core/RTTIRefObject.h"
#include "ModConfiguration.h"
#include "ModCoreEvents.h"

ModCoreEvents::ValuePatchVisitor::ValuePatchVisitor(const RTTIValuePatch& Patch)
{
	m_ValueToSet = Patch.m_Value;
}

void ModCoreEvents::ValuePatchVisitor::Visit(void *Object, const HRZ2::RTTI *Type)
{
	if (!Type->DeserializeObject(Object, m_ValueToSet))
		m_LastError = std::format("Failed to set value '{}'.", m_ValueToSet);
}

HRZ2::RTTIObjectTweaker::SetValueVisitor::EMode ModCoreEvents::ValuePatchVisitor::GetMode()
{
	return EMode::MODE_WRITE;
}

ModCoreEvents::ModCoreEvents()
{
	RegisterParsedPatches();
	RegisterHardcodedPatches();
}

void ModCoreEvents::RegisterParsedPatches()
{
	auto splitStringByDelimiter = [](const std::string_view& Text, char Delimiter, auto&& Callback)
	{
		for (const auto& part : Text | std::views::split(Delimiter))
			Callback(std::string_view(part));
	};

	// Create all of the patch instances from mod configuration data
	for (const auto& entry : ModConfiguration.AssetOverrides)
	{
		if (!entry.Enabled)
			continue;

		if (entry.ObjectTypes.empty() && entry.ObjectUUIDs.empty())
			spdlog::error("ObjectTypes and ObjectUUIDs are both empty. Asset override will have no effect.");

		// Lookup by type
		splitStringByDelimiter(
			entry.ObjectTypes,
			',',
			[&](const std::string_view& Type)
			{
				auto rtti = HRZ2::RTTI::FindTypeByName(Type);

				if (rtti)
					m_RTTIPatchesByType[rtti].emplace_back(RTTIValuePatch { entry.Path, entry.Value });
				else
					spdlog::error("Failed to resolve override type name '{}'. Skipping.", Type);
			});

		// Lookup by UUID
		splitStringByDelimiter(
			entry.ObjectUUIDs,
			',',
			[&](const std::string_view& UUID)
			{
				auto uuid = HRZ2::GGUUID::TryParse(UUID);

				if (uuid)
					m_RTTIPatchesByUUID[uuid.value()].emplace_back(RTTIValuePatch { entry.Path, entry.Value });
				else
					spdlog::error("Failed to parse override UUID '{}'. Skipping.", UUID);
			});
	}

	// Then also handle character override entries
	for (const auto& entry : ModConfiguration.CharacterOverrides)
	{
		if (entry.RootUUID.empty() || entry.VariantUUID.empty())
		{
			spdlog::error("RootUUID or VariantUUID is empty. Character override will have no effect.");
			continue;
		}

		m_CharacterOverrideRootUUID = entry.RootUUID;
		m_CharacterOverrideVariantUUID = entry.VariantUUID;
	}

	if (m_CharacterOverrideVariantUUID)
		spdlog::info("Using character override variant UUID {}", m_CharacterOverrideVariantUUID.value());
}

void ModCoreEvents::RegisterHardcodedPatches()
{
	using namespace HRZ2;

	auto addCallback = [this]<typename T>(T Value, void (*Callback)(Ref<RTTIRefObject>&))
	{
		if constexpr (std::is_same_v<T, GGUUID>)
			m_RTTIPatchesByUUID[Value].emplace_back(RTTICallbackPatch { Callback });
		else if constexpr (std::is_same_v<T, const RTTI *>)
			m_RTTIPatchesByType[Value].emplace_back(RTTICallbackPatch { Callback });
		else
			static_assert(!sizeof(T), "Invalid type");
	};

	auto addUnloadCallback = [this](const RTTI *Value, void (*Callback)(Ref<RTTIRefObject>&))
	{
		m_RTTIUnloadsByType[Value].emplace_back(Callback);
	};

	//
	// HFW hardcodes ultra hard difficulty in various places. Two common cases are direct pointer comparisons in HFW.exe
	// code and in fullgame graph programs. Menus use some kind of logic I haven't identified yet.
	//
	// UH's DifficultyResource gets loaded first at startup and stays in memory forever. The OnDifficultySetGraphProgram's
	// NodeConstantsResource and AIManager's AIManagerResourceGame can then reference it any time later.
	//
	if (ModConfiguration.ForceCustomUltraHard)
	{
		static Ref<RTTIRefObject> customDifficultyRef;

		addCallback(
			GGUUID::Parse("8B0BFB86-3540-1040-B856-488DA2D9E906"), // DifficultyResource
			[](auto& Object)
			{
				customDifficultyRef = Object;
			});

		addCallback(
			GGUUID::Parse("8B27A783-AB20-1A3B-8C6B-30330C9211BB"), // NodeConstantsResource
			[](auto& Object)
			{
				auto& parameters = Object->GetMemberRefUnsafe<ProgramParameterList>("Parameters");
				parameters.m_DefaultSoftLinkedObjects[0] = customDifficultyRef;
			});

		addCallback(
			GGUUID::Parse("8070E517-98AE-F347-BB49-3E87F3248E11"), // AIManagerResourceGame
			[](auto& Object)
			{
				Object->GetMemberRefUnsafe<Ref<RTTIRefObject>>("UltraHardDifficultyPreset") = customDifficultyRef;
			});
	}

	//
	// Remove the BoolFactConditionResource "is UH" check from HUDEnemyHealthBarLogicResource's DisplayConditions. Asset
	// overrides don't support arrays of references. We have to do it here instead.
	//
	if (ModConfiguration.UnlockUltraHardRestrictions)
	{
		addCallback(
			GGUUID::Parse("E1258F7C-DA72-5D48-8F62-3257B02C6BA6"), // HUDEnemyHealthBarLogicResource
			[](auto& Object)
			{
				auto& displayConditions = Object->GetMemberRefUnsafe<Array<Ref<RTTIRefObject>>>("DisplayConditions");

				if (displayConditions.size() == 5)
					displayConditions.pop_back();
			});
	}

	//
	// Increase inventory item stacks to 1,000,000 or multiply defaults by a user-defined value.
	//
	if (ModConfiguration.IncreaseInventoryStacks || ModConfiguration.InventoryStackMultiplier > 0)
	{
		addCallback(
			RTTI::FindTypeByName("InventoryItemResource"),
			[](auto& Object)
			{
				if (Object->GetMemberRefUnsafe<bool>("IsStackable"))
				{
					auto& stackSizes = Object->GetMemberRefUnsafe<Array<int>>("MaxStackSize");

					for (auto& limit : stackSizes)
					{
						if (ModConfiguration.InventoryStackMultiplier > 0)
							limit *= ModConfiguration.InventoryStackMultiplier;
						else
							limit = std::max(limit, 1000000);
					}
				}
			});

		addCallback(
			RTTI::FindTypeByName("InventoryCapacityComponentResource"),
			[](auto& Object)
			{
				class SharedInventoryItemTagLimitDefinition
				{
				public:
					Ref<RTTIRefObject> m_InventoryItemTag;
					Array<int> m_Limits;
					Ref<RTTIRefObject> m_LimitIndexFact;
				};
				static_assert(sizeof(SharedInventoryItemTagLimitDefinition) == 0x20);

				auto& limitDefinitions = Object->GetMemberRefUnsafe<Array<SharedInventoryItemTagLimitDefinition>>(
					"SharedInventoryItemLimits");

				for (auto& limitDef : limitDefinitions)
				{
					for (auto& limit : limitDef.m_Limits)
					{
						const auto hardCap = std::max(limit, 1000); // Higher than 1000 leads to UI-related crashes

						if (limit > 0)
						{
							if (ModConfiguration.InventoryStackMultiplier > 0)
								limit = std::min(limit * ModConfiguration.InventoryStackMultiplier, hardCap);
							else
								limit = hardCap;
						}
					}
				}
			});
	}

	//
	// Remove magnet-based aim assist and camera anchoring. Most obvious use is when targeting machines with the focus.
	//
	if (ModConfiguration.DisableCameraMagnetism || ModConfiguration.DisableFocusMagnetism)
	{
		auto killMagnetSelector = [](auto& Object)
		{
			class CameraParmResource
			{
			public:
				virtual ~CameraParmResource() = 0;

				bool m_Override;
				Ref<RTTIRefObject> m_Val;
			};
			static_assert(sizeof(CameraParmResource) == 0x18);

			auto& selector = Object->GetMemberRefUnsafe<CameraParmResource>("MagnetModeSelector");
			selector.m_Override = true;
			selector.m_Val = nullptr;
		};

		// CameraModeResource: everything
		if (ModConfiguration.DisableCameraMagnetism)
			addCallback(RTTI::FindTypeByName("CameraModeResource"), killMagnetSelector);

		if (ModConfiguration.DisableFocusMagnetism)
		{
			addCallback(GGUUID::Parse("A7493033-3D69-C94E-9791-641089B1FE46"), killMagnetSelector); // On foot, focus open
			addCallback(GGUUID::Parse("3AF1EE58-0F6C-A74E-A2FB-833376A2CAE7"), killMagnetSelector); // On foot, crouched, focus open
			addCallback(GGUUID::Parse("12C9ED87-A030-1D42-88C7-4ED6CB4356FC"), killMagnetSelector); // Swimming, focus open
			addCallback(GGUUID::Parse("FFDCCC96-898F-2B4A-846C-71F1B211BD23"), killMagnetSelector); // Swimming, underwater ruins, focus open
			addCallback(GGUUID::Parse("ED1E28E1-9CFE-2B4F-AEB1-7E26D976EC91"), killMagnetSelector); // Charger mount, focus open
			addCallback(GGUUID::Parse("CFB94F63-FD50-F54C-8AAF-21FEB059AB4D"), killMagnetSelector); // Clawstrider mount, focus open
			addCallback(GGUUID::Parse("D7B8892F-8ED5-0449-B540-316151575AEF"), killMagnetSelector); // Flying mount, focus open
		}
	}

	//
	// Character overrides. Direct BodyVariant pointer replacement.
	//
	if (m_CharacterOverrideVariantUUID)
	{
		static StreamingRef<RTTIRefObject> targetRootObject; // Permanently loaded
		static Ref<RTTIRefObject> targetBodyVariantObject;

		addCallback(
			GGUUID::Parse("1C710F84-C54D-9643-8353-379EC8927545"),
			[](auto& Object)
			{
				// Queue the load after the inital photo mode resource. Otherwise we'd need a hook.
				JobHeaderCPU::SubmitCallback(
					[]()
					{
						auto manager = StreamingManager::GetInstance();
						manager->Register2(targetRootObject, {}, GetInstance().m_CharacterOverrideRootUUID.value());
						manager->Resolve(targetRootObject, EStreamingRefPriority::Normal);
					});
			});

		addCallback(
			m_CharacterOverrideVariantUUID.value(),
			[](auto& Object)
			{
				std::scoped_lock lock(GetInstance().m_CachedDataMutex);
				targetBodyVariantObject = Object;
			});

		addCallback(
			RTTI::FindTypeByName("PlayerOutfitTheme"),
			[](auto& Object)
			{
				auto& variantRef = Object->GetMemberRefUnsafe<StreamingRef<RTTIRefObject>>("BodyVariant");
				variantRef.Clear();

				std::shared_lock lock(GetInstance().m_CachedDataMutex);
				StreamingManager::GetInstance()->Register4(variantRef, targetBodyVariantObject, 5);
			});
	}

	//
	// Custom damage multiplier overrides.
	//
	if (!ModConfiguration.DamageModifierOverrides.empty())
	{
		using DamageTypeResource = RTTIRefObject;
		using DamageTypeGroup = RTTIRefObject;
		using PlayerDamageModifier = RTTIRefObject;

		static Ref<RTTIRefObject> damageTypeResourceSettings;

		addCallback(
			RTTI::FindTypeByName("DamageTypeResourceSettingsGame"),
			[](auto& Object)
		{
			std::scoped_lock lock(GetInstance().m_CachedDataMutex);
			damageTypeResourceSettings = Object;
		});

		static auto getDamageTypeResourceByUUID = [](const GGUUID& UUID) -> Ref<DamageTypeResource>
		{
			std::shared_lock lock(GetInstance().m_CachedDataMutex);
			const auto& damageTypes = damageTypeResourceSettings->GetMemberRefUnsafe<Array<Ref<DamageTypeResource>>>("DamageTypeResources");

			for (const auto& type : damageTypes)
			{
				if (type->m_UUID == UUID)
					return type;
			}

			return nullptr;
		};

		addCallback(
			RTTI::FindTypeByName("DifficultyManagerResource"),
			[](auto& Object)
		{
			if (auto& globalDamageModifier = Object->GetMemberRefUnsafe<Ref<RTTIRefObject>>("GlobalDamageModifier"))
			{
				class PlayerDamageModifierForDamageType
				{
				public:
					Array<Ref<DamageTypeResource>> m_DamageTypes; // 0x0
					Ref<DamageTypeGroup> m_DamageTypeGroup;		  // 0x10
					alignas(16) float m_UnknownFloats[4] = {};	  // 0x20
					bool m_UnknownFlag = false;					  // 0x30
					char _pad31[0xF];							  // 0x31
					Ref<PlayerDamageModifier> m_DamageModifier;	  // 0x40
				};
				static_assert(sizeof(PlayerDamageModifierForDamageType) == 0x50);

				auto& specificDamageSettings = globalDamageModifier->GetMemberRefUnsafe<Array<PlayerDamageModifierForDamageType>>(
					"SpecificPlayerDamageDealtSettings");

				// We have to create a copy since old modifier values act as a fallback (they're evaluated last)
				const auto oldSpecificDamageSettings = std::move(specificDamageSettings);

				// Then convert mod-side overrides to the game's format
				for (const auto& modOverride : ModConfiguration.DamageModifierOverrides)
				{
					auto damageTypeResource = getDamageTypeResourceByUUID(modOverride.DamageTypeUUID);

					if (!damageTypeResource)
					{
						spdlog::error("Failed to find damage type resource with UUID '{}'. Skipping.", modOverride.DamageTypeUUID);
						continue;
					}

					const auto rtti = RTTI::FindTypeByName("PlayerDamageModifier")->AsCompound();
					Ref rttiInstance = static_cast<PlayerDamageModifier *>(rtti->CreateInstance());

					rtti->SetMemberValue<GGUUID>(rttiInstance, "ObjectUUID", GGUUID::Generate());
					rtti->SetMemberValue<float>(rttiInstance, "DamageModifier", modOverride.Multiplier);

					PlayerDamageModifierForDamageType newDamageSetting;
					newDamageSetting.m_DamageTypes.push_back(damageTypeResource);
					newDamageSetting.m_DamageModifier = rttiInstance;
					specificDamageSettings.push_back(newDamageSetting);
				}

				for (const auto& oldSetting : oldSpecificDamageSettings)
					specificDamageSettings.push_back(oldSetting);
			}
		});
	}

	//
	// Dialogue blacklist. Does nothing more than silence audio for SentenceResources matching child LocalizedTextResource
	// UUIDs.
	//
	if (!ModConfiguration.CachedDialogueBlacklist.empty())
	{
		addCallback(
			RTTI::FindTypeByName("SentenceResource"),
			[](auto& Object)
			{
				if (const auto& textResource = Object->GetMemberRefUnsafe<Ref<RTTIRefObject>>("Text"))
				{
					const bool needsPatching = std::binary_search(
						ModConfiguration.CachedDialogueBlacklist.begin(),
						ModConfiguration.CachedDialogueBlacklist.end(),
						textResource->m_UUID);

					if (needsPatching)
					{
						Object->GetMemberRefUnsafe<bool>("ShowSubtitles") = false;

						if (auto& soundResource = Object->GetMemberRefUnsafe<Ref<RTTIRefObject>>("SimpleSound"))
						{
							soundResource->GetMemberRefUnsafe<float>("DefaultVolume") = 0.0f;
							soundResource->GetMemberRefUnsafe<float>("DefaultLfeVolume") = 0.0f;
						}
					}
				}
			});
	}

	//
	// HACK: Read the fake "InternalLODRangeModifier" asset override value and forward it to the debug menu. GameViewResource
	// isn't actually present in the game files so we can remove it entirely.
	//
	if (auto itr = m_RTTIPatchesByType.find(RTTI::FindTypeByName("GameViewResource")); itr != m_RTTIPatchesByType.end())
	{
		if (!itr->second.empty())
		{
			const auto& value = std::get<RTTIValuePatch>(itr->second.front()).m_Value;

			if (!value.empty())
				std::from_chars(value.data(), value.data() + value.size(), m_CachedLODRangeModifierHack);
		}

		m_RTTIPatchesByType.erase(itr);
	}

	//
	// Global object cache lists
	//
#define RegisterCacheCallback(RTTIName, MemberName)			\
	addCallback(											\
		RTTI::FindTypeByName(RTTIName),				        \
		[](auto& Object)									\
		{													\
			auto& e = GetInstance();						\
			std::scoped_lock lock(e.m_CachedDataMutex);		\
			e.MemberName.emplace(Object);					\
		});													\
	addUnloadCallback(										\
		RTTI::FindTypeByName(RTTIName),				        \
		[](auto& Object)									\
		{													\
			auto& e = GetInstance();						\
			std::scoped_lock lock(e.m_CachedDataMutex);		\
			e.MemberName.erase(Object);						\
		});

	RegisterCacheCallback("AIFaction", m_CachedAIFactions);

	RegisterCacheCallback("SpawnSetup", m_CachedSpawnSetups);
	RegisterCacheCallback("SpawnSetupGroup", m_CachedSpawnSetups);
	RegisterCacheCallback("SpawnSetupPlaceholder", m_CachedSpawnSetups);

	RegisterCacheCallback("WeatherSetup", m_CachedWeatherSetups);
	RegisterCacheCallback("WeatherSetupDLC", m_CachedWeatherSetups);
	RegisterCacheCallback("LocalWeatherSetup", m_CachedWeatherSetups);

	RegisterCacheCallback("InventoryItemResource", m_CachedInventoryItems);

#undef RegisterCacheCallback
}

void ModCoreEvents::OnFinishLoadGroup(const HRZ2::Array<HRZ2::Ref<HRZ2::RTTIRefObject>>& Objects)
{
	if (ModConfiguration.EnableAssetLogging)
		spdlog::info("OnFinishLoadGroup");
}

void ModCoreEvents::OnBeforeUnloadGroup(const HRZ2::Array<HRZ2::Ref<HRZ2::RTTIRefObject>>& Objects)
{
	if (ModConfiguration.EnableAssetLogging)
		spdlog::info("OnBeforeUnloadGroup");

	for (auto& object : Objects)
	{
		const auto rtti = object->GetRTTI();

		if (auto itr = m_RTTIUnloadsByType.find(rtti); itr != m_RTTIUnloadsByType.end())
		{
			for (const auto& entry : itr->second)
				entry.m_Callback(object);
		}
	}
}

void ModCoreEvents::OnLoadAssetGroup(const HRZ2::Array<HRZ2::Ref<HRZ2::RTTIRefObject>>& Objects)
{
	if (ModConfiguration.EnableAssetLogging)
		spdlog::info("OnLoadAssetGroup");

	for (auto& object : Objects)
	{
		const auto rtti = object->GetRTTI();

		if (ModConfiguration.EnableAssetLogging)
			spdlog::debug("Asset {}: {}", object->m_UUID, rtti->GetSymbolName());

		// Apply all patches loaded from user config
		auto visitAll = [&](const auto& Entries)
		{
			auto v = [&object, &rtti](auto&& Patch)
			{
				if constexpr (std::is_same_v<std::decay_t<decltype(Patch)>, RTTIValuePatch>)
				{
					if constexpr (false)
						spdlog::debug("Patching '{}.{}' with value '{}'", rtti->GetSymbolName(), Patch.m_Path, Patch.m_Value);

					if (Patch.m_Path.starts_with('@'))
					{
						auto& src = object->GetMemberRefUnsafe<HRZ2::Ref<HRZ2::RTTIRefObject>>(Patch.m_Path.c_str() + 1);

						if (Patch.m_Value.empty())
							src = nullptr;
						else if (Patch.m_Value.starts_with('@'))
							src = object->GetMemberRefUnsafe<HRZ2::Ref<HRZ2::RTTIRefObject>>(Patch.m_Value.c_str() + 1);
						else
							spdlog::error("Error applying asset override: Invalid reference value '{}'", Patch.m_Value);
					}
					else
					{
						ValuePatchVisitor v(Patch);
						HRZ2::RTTIObjectTweaker::VisitObjectPath(object, rtti, Patch.m_Path, &v);

						if (!v.m_LastError.empty())
							spdlog::error("Error applying asset override: {}", v.m_LastError);
					}
				}
				else if constexpr (std::is_same_v<std::decay_t<decltype(Patch)>, RTTICallbackPatch>)
					Patch.m_Callback(object);
				else
					static_assert(!sizeof(decltype(Patch)), "Invalid type");
			};

			for (auto& entry : Entries)
				std::visit(v, entry);
		};

		if (auto itr = m_RTTIPatchesByType.find(rtti); itr != m_RTTIPatchesByType.end())
			visitAll(itr->second);

		if (auto itr = m_RTTIPatchesByUUID.find(object->m_UUID); itr != m_RTTIPatchesByUUID.end())
			visitAll(itr->second);
	}
}

ModCoreEvents& ModCoreEvents::GetInstance()
{
	// Yes, I'm intentionally leaking memory. There's no virtual destructor present and this
	// never gets unregistered properly.
	static auto handler = new ModCoreEvents();
	return *handler;
}
