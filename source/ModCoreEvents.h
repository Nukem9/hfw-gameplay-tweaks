#pragma once

#include <unordered_set>
#include "HRZ2/Core/IStreamingSystem.h"
#include "HRZ2/Core/RTTIObjectTweaker.h"
#include "HRZ2/PCore/Common.h"

namespace HRZ2
{
	class RTTI;
	class RTTIRefObject;
}

class ModCoreEvents : public HRZ2::IStreamingSystem::Events
{
private:
	struct RTTICallbackPatch
	{
		void (*m_Callback)(HRZ2::Ref<HRZ2::RTTIRefObject>&);
	};

	struct RTTIValuePatch
	{
		HRZ2::String m_Path;
		HRZ2::String m_Value;
	};

	using RTTIPatch = std::variant<RTTICallbackPatch, RTTIValuePatch>;

	class ValuePatchVisitor : public HRZ2::RTTIObjectTweaker::SetValueVisitor
	{
	public:
		ValuePatchVisitor(const RTTIValuePatch& Patch);
		ValuePatchVisitor(const ValuePatchVisitor&) = delete;
		virtual ~ValuePatchVisitor() override = default;

		virtual void Visit(void *Object, const HRZ2::RTTI *Type) override;
		virtual EMode GetMode() override;
	};

	// Patching callbacks
	std::unordered_map<const HRZ2::RTTI *, std::vector<RTTIPatch>> m_RTTIPatchesByType;
	std::unordered_map<HRZ2::GGUUID, std::vector<RTTIPatch>> m_RTTIPatchesByUUID;
	std::unordered_map<const HRZ2::RTTI *, std::vector<RTTICallbackPatch>> m_RTTIUnloadsByType;

	// Character override
	std::optional<HRZ2::GGUUID> m_CharacterOverrideRootUUID;
	std::optional<HRZ2::GGUUID> m_CharacterOverrideVariantUUID;

public:
	HRZ2::SharedMutex m_CachedDataMutex;
	std::unordered_set<HRZ2::RTTIRefObject *> m_CachedAIFactions;
	std::unordered_set<HRZ2::RTTIRefObject *> m_CachedSpawnSetups;
	std::unordered_set<HRZ2::RTTIRefObject *> m_CachedWeatherSetups;
	std::unordered_set<HRZ2::RTTIRefObject *> m_CachedInventoryItems;
	float m_CachedLODRangeModifierHack = std::numeric_limits<float>::max();

	ModCoreEvents();
	ModCoreEvents(const ModCoreEvents&) = delete;
	virtual ~ModCoreEvents() = default;

	void RegisterParsedPatches();
	void RegisterHardcodedPatches();

	virtual void OnFinishLoadGroup(const HRZ2::Array<HRZ2::Ref<HRZ2::RTTIRefObject>>& Objects) override;
	virtual void OnBeforeUnloadGroup(const HRZ2::Array<HRZ2::Ref<HRZ2::RTTIRefObject>>& Objects) override;
	virtual void OnLoadAssetGroup(const HRZ2::Array<HRZ2::Ref<HRZ2::RTTIRefObject>>& Objects) override;

	static ModCoreEvents& GetInstance();
};
