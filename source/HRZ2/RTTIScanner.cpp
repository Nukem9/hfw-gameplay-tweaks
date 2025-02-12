#include "Core/RTTI.h"
#include "RTTIScanner.h"

namespace RTTIScanner
{
	std::unordered_set<const HRZ2::RTTI *> ScannedRTTITypes;
	std::unordered_set<const HRZ2::RTTI *> RegisteredRTTITypes;

	const std::unordered_set<const HRZ2::RTTI *>& GetAllTypes()
	{
		// If no types are present, try to gather them now
		if (!ScannedRTTITypes.empty() && RegisteredRTTITypes.empty())
			RegisterRTTIStructures();

		return RegisteredRTTITypes;
	}

	void ScanForRTTIStructures()
	{
		const size_t dataBase = Offsets::Relative(0x1C8D000);
		const size_t dataEnd = Offsets::Relative(0x8B77FFF);

		const size_t rdataBase = Offsets::Relative(0x173E000);
		const size_t rdataEnd = Offsets::Relative(0x1C8CA66);

		auto isDataSegment = [&]<typename T>(const T *Pointer)
		{
			return reinterpret_cast<uintptr_t>(Pointer) >= dataBase && reinterpret_cast<uintptr_t>(Pointer) < dataEnd ||
				   reinterpret_cast<uintptr_t>(Pointer) >= rdataBase && reinterpret_cast<uintptr_t>(Pointer) < rdataEnd;
		};

		for (uintptr_t i = dataBase; i < dataEnd; i += 2)
		{
			if (*reinterpret_cast<uint32_t *>(i) != 0xFFFFFFFF)
				continue;

			auto rtti = reinterpret_cast<const HRZ2::RTTI *>(i);

			if (rtti->m_Type < HRZ2::ERTTIType::Atom || rtti->m_Type > HRZ2::ERTTIType::Bitset)
				continue;

			// Validate pointers before blindly adding them to the collection. RTTI entries typically have
			// multiple fields for accessing the .data section, so check those.
			if (auto asContainer = rtti->AsContainer())
			{
				if (!isDataSegment(asContainer->m_ItemType) || !isDataSegment(asContainer->m_ContainerType))
					continue;
			}
			else if (auto asEnum = rtti->AsEnum())
			{
				if (!isDataSegment(asEnum->m_TypeName) || !isDataSegment(asEnum->m_Values))
					continue;
			}
			else if (auto asCompound = rtti->AsCompound())
			{
				if (!isDataSegment(asCompound->m_TypeName) || asCompound->m_Alignment <= 0)
					continue;
			}
			else if (auto asBitset = rtti->AsBitset())
			{
				if (!isDataSegment(asBitset->m_TypeName) || !isDataSegment(asBitset->m_Type))
					continue;
			}
			else
			{
				// Discard the rest
				continue;
			}

			ScannedRTTITypes.emplace(rtti);
		}
	}

	void RegisterTypeInfoRecursively(const HRZ2::RTTI *Info)
	{
		const auto [_, inserted] = RegisteredRTTITypes.emplace(Info);

		if (!inserted)
			return;

		if (auto asCompound = Info->AsCompound())
		{
			// Register base classes
			for (auto& base : asCompound->Bases())
				RegisterTypeInfoRecursively(base.m_Type);

			// Then field types
			for (auto& member : asCompound->Members())
			{
				if (!member.IsGroup())
					RegisterTypeInfoRecursively(member.m_Type);
			}

			// Then message types
			for (auto& message : asCompound->MessageHandlers())
			{
				if (message.m_Message)
					RegisterTypeInfoRecursively(message.m_Message);
			}

			// Then exported script symbols
			if (asCompound->m_GetSymbolGroupFunc)
				RegisterTypeInfoRecursively(asCompound->m_GetSymbolGroupFunc());
		}
		else if (auto asContainer = Info->AsContainer())
		{
			RegisterTypeInfoRecursively(asContainer->GetContainedType());
		}
		else if (auto asBitset = Info->AsBitset())
		{
			RegisterTypeInfoRecursively(asBitset->m_Type);
		}
	}

	void RegisterRTTIStructures()
	{
		RegisteredRTTITypes.clear();

		for (auto rtti : ScannedRTTITypes)
			RegisterTypeInfoRecursively(rtti);

		ScannedRTTITypes.clear();
	}

	DECLARE_HOOK_TRANSACTION(RTTIScanner)
	{
		if constexpr (false)
			ScanForRTTIStructures();
	};
}
