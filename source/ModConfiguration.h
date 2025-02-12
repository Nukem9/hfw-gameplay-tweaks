#pragma once

#include <filesystem>
#include "HRZ2/PCore/UUID.h"

namespace InternalModConfig
{
	struct AssetOverride
	{
		bool Enabled = false;
		std::string ObjectUUIDs;
		std::string ObjectTypes;
		std::string Path;
		std::string Value;
	};

	struct GlobalSettings
	{
		GlobalSettings() = default;
		GlobalSettings(const GlobalSettings&) = delete;

		// [General]
		bool EnableDebugMenu;
		bool EnableAssetLogging;
		bool EnableAssetOverrides;
		float DebugMenuFontScale;

		// [Gameplay]
		bool UnlockMapBoundaries;
		bool UnlockWorldMapMenu;
		bool UnlockMountRestrictions;
		bool UnlockEntitlementExtras;
		bool UnlockPhotomodeRestrictions;
		bool UnlockAIvsAIDamage;
		bool UnlockUltraHardRestrictions;
		bool UnlockTrapAndTripwireLimits;

		bool EnableFreePerks;
		bool EnableFreeCrafting;
		bool EnableGodMode;
		bool EnableInfiniteClipAmmo;
		bool EnableInfiniteOverrideTimer;
		bool EnableAutoNeutralFaction;

		bool IncreaseInventoryStacks;
		int InventoryStackMultiplier;

		bool DisableCameraMagnetism;
		bool DisableFocusMagnetism;

		bool ForceCustomUltraHard;
		bool ForceNoShaderLoadingScreen;

		// [Hotkeys]
		struct
		{
			int ToggleDebugUI;
			int TogglePauseGameLogic;
			int ToggleFreeCamera;
			int ToggleNoclip;
			int IncreaseTimescale;
			int DecreaseTimescale;
			int ToggleTimescale;
			int SpawnEntity;
			int QuickSave;
			int QuickLoad;
			int ToggleAIProcessing;
			int TogglePauseTimeOfDay;
		} Hotkeys;

		// [AssetOverride]
		std::vector<AssetOverride> AssetOverrides;

		// [CharacterOverride]
		struct CharacterOverride
		{
			HRZ2::GGUUID RootUUID;
			HRZ2::GGUUID VariantUUID;
		};

		std::vector<CharacterOverride> CharacterOverrides;

		// [DamageDealtModifiers]
		struct DamageModifierEntry
		{
			HRZ2::GGUUID DamageTypeUUID;
			float Multiplier;
		};

		std::vector<DamageModifierEntry> DamageModifierOverrides;

		// [CoreObjectCache]
		struct ObjectCacheEntry
		{
			HRZ2::GGUUID UUID;
			HRZ2::GGUUID RootUUID;
			std::string Name;

			friend auto operator<=>(const ObjectCacheEntry& Lhs, const ObjectCacheEntry& Rhs)
			{
				return Lhs.UUID <=> Rhs.UUID;
			}

			friend auto operator<=>(const ObjectCacheEntry& Lhs, const HRZ2::GGUUID& Rhs)
			{
				return Lhs.UUID <=> Rhs;
			}
		};

		std::vector<ObjectCacheEntry> CachedSpawnSetups;
		std::vector<ObjectCacheEntry> CachedWeatherSetups;
		std::vector<ObjectCacheEntry> CachedInventoryItems;
		std::vector<ObjectCacheEntry> CachedDialogueBlacklist;
		std::vector<ObjectCacheEntry> CachedRoots;
	};

	void Initialize(const std::filesystem::path& RootDirectory);
	bool InitializeConfigurationFile();
	std::filesystem::path GetModRelativePath(const std::string_view& PartialPath);
}

extern const InternalModConfig::GlobalSettings& ModConfiguration;
