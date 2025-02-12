#include <format>
#include <algorithm>
#include "RTTI.h"
#include "RTTIObject.h"

namespace HRZ2
{
	bool RTTI::IsExactKindOf(const RTTI *Other) const
	{
		return this == Other;
	}

	bool RTTI::IsKindOf(const RTTI *Other) const
	{
		if (this == Other)
			return true;

		if (((m_UnknownFlags | Other->m_UnknownFlags) & 1) == 0)
			return static_cast<uint16_t>(m_TypeIndex - Other->m_TypeIndex) <= Other->m_NumDerivedTypes;

		if (m_Type != ERTTIType::Compound || Other->m_Type != ERTTIType::Compound)
			return this == Other;

		for (auto t = static_cast<const RTTICompound *>(this); t->m_BaseCount;)
		{
			t = static_cast<const RTTICompound *>(t->m_Bases[0].m_Type);

			if (t == Other)
				return true;
		}

		return false;
	}

	const RTTIAtom *RTTI::AsAtom() const
	{
		if (m_Type != ERTTIType::Atom)
			return nullptr;

		return static_cast<const RTTIAtom *>(this);
	}

	const RTTIContainer *RTTI::AsContainer() const
	{
		if (m_Type != ERTTIType::Pointer && m_Type != ERTTIType::Container)
			return nullptr;

		return static_cast<const RTTIContainer *>(this);
	}

	const RTTIEnum *RTTI::AsEnum() const
	{
		if (m_Type != ERTTIType::Enum && m_Type != ERTTIType::Bitfield)
			return nullptr;

		return static_cast<const RTTIEnum *>(this);
	}

	const RTTICompound *RTTI::AsCompound() const
	{
		if (m_Type != ERTTIType::Compound)
			return nullptr;

		return static_cast<const RTTICompound *>(this);
	}

	const RTTIPOD *RTTI::AsPOD() const
	{
		if (m_Type != ERTTIType::POD)
			return nullptr;

		return static_cast<const RTTIPOD *>(this);
	}

	const RTTIBitset *RTTI::AsBitset() const
	{
		if (m_Type != ERTTIType::Bitset)
			return nullptr;

		return static_cast<const RTTIBitset *>(this);
	}

	const RTTI *RTTI::GetContainedType() const
	{
		switch (m_Type)
		{
		case ERTTIType::Pointer:
		case ERTTIType::Container:
			return static_cast<const RTTIContainer *>(this)->m_ItemType;
		}

		return this;
	}

	std::string RTTI::GetSymbolName() const
	{
		switch (m_Type)
		{
		case ERTTIType::Atom:
			return static_cast<const RTTIAtom *>(this)->m_TypeName;

		case ERTTIType::Pointer:
		case ERTTIType::Container:
		{
			auto container = static_cast<const RTTIContainer *>(this);

			if (!strcmp(container->m_ContainerType->m_TypeName, "cptr"))
				return std::format("CPtr<{0:}>", container->m_ItemType->GetSymbolName());

			return std::format("{0:}<{1:}>", container->m_ContainerType->m_TypeName, container->m_ItemType->GetSymbolName());
		}

		case ERTTIType::Enum:
		case ERTTIType::Bitfield:
			return static_cast<const RTTIEnum *>(this)->m_TypeName;

		case ERTTIType::Compound:
			return static_cast<const RTTICompound *>(this)->m_TypeName;

		case ERTTIType::POD:
			return std::format("POD{0:}", static_cast<const RTTIPOD *>(this)->m_Size);

		case ERTTIType::Bitset:
			return static_cast<const RTTIBitset *>(this)->m_TypeName;
		}

		return "";
	}

	uint64_t RTTI::GetCoreBinaryTypeId() const
	{
		const auto func = Offsets::Signature("40 57 48 81 EC 30 08 00 00 0F B6 41 04 4C 8D")
							  .ToPointer<uint64_t(const RTTI *)>();

		return func(this);
	}

	void *RTTI::CreateInstance() const
	{
		const auto func = Offsets::Signature("48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B F9 E8 ? ? ? ? 80 79 04 04")
							  .ToPointer<void *(const RTTI *)>();

		return func(this);
	}

	std::optional<std::string> RTTI::SerializeObject(const void *Object) const
	{
		switch (m_Type)
		{
		case ERTTIType::Atom:
			return static_cast<const RTTIAtom *>(this)->SerializeObject(Object);

		case ERTTIType::Pointer:
		case ERTTIType::Container:
			return static_cast<const RTTIContainer *>(this)->SerializeObject(Object);

		case ERTTIType::Enum:
		case ERTTIType::Bitfield:
			return static_cast<const RTTIEnum *>(this)->SerializeObject(Object);

		case ERTTIType::Compound:
			return static_cast<const RTTICompound *>(this)->SerializeObject(Object);

		case ERTTIType::POD:
		case ERTTIType::Bitset:
			return std::nullopt;
		}

		return std::nullopt;
	}

	bool RTTI::DeserializeObject(void *Object, const String& InText) const
	{
		switch (m_Type)
		{
		case ERTTIType::Atom:
			return static_cast<const RTTIAtom *>(this)->DeserializeObject(Object, InText);

		case ERTTIType::Pointer:
		case ERTTIType::Container:
			return static_cast<const RTTIContainer *>(this)->DeserializeObject(Object, InText);

		case ERTTIType::Enum:
		case ERTTIType::Bitfield:
			return static_cast<const RTTIEnum *>(this)->DeserializeObject(Object, InText);

		case ERTTIType::Compound:
			return static_cast<const RTTICompound *>(this)->DeserializeObject(Object, InText);

		case ERTTIType::POD:
		case ERTTIType::Bitset:
			return false;
		}

		return false;
	}

	const RTTI *RTTI::FindTypeByName(const std::string_view& TypeName)
	{
		// 128 characters should be enough for any type
		char buffer[128] = {};
		memcpy(buffer, TypeName.data(), std::min(TypeName.size(), sizeof(buffer) - 1));

		return FindTypeByName(buffer);
	}

	const RTTI *RTTI::FindTypeByName(const char *TypeName)
	{
		const auto func = Offsets::Signature("48 83 EC 38 48 85 C9 74 37 48 89 4C 24 20 48 C7 C0 FF FF FF FF")
							  .ToPointer<const RTTI *(const char *)>();

		return func(TypeName);
	}

	std::optional<std::string> RTTIAtom::SerializeObject(const void *Object) const
	{
		if (String str; m_ToString && m_ToString(Object, str))
			return std::format("\"{}\"", str);

		return std::nullopt;
	}

	bool RTTIAtom::DeserializeObject(void *Object, const String& InText) const
	{
		return m_FromStringSpan && m_FromStringSpan(InText, Object);
	}

	std::optional<std::string> RTTIContainer::SerializeObject(const void *Object) const
	{
		if (m_Type == ERTTIType::Pointer)
		{
			auto containerInfo = static_cast<const DataPointer *>(m_ContainerType);

			if (auto ptr = containerInfo->m_Getter(this, Object))
				return m_ItemType->SerializeObject(ptr);

			return "null";
		}
		else if (m_Type == ERTTIType::Container)
		{
			auto containerInfo = static_cast<const DataContainer *>(m_ContainerType);

#if 0
			// The native serializer can handle select types. This is the fast path.
			if (containerInfo->m_ToString)
			{
				if (String str; containerInfo->m_ToString(Object, this, str))
					return str.c_str();
			}
#endif

			// ToString failed; manually iterate over the array/hashmap
			std::string output = "[";

			for (auto itr = containerInfo->m_GetBeginIterator(this, Object); containerInfo->m_IsIteratorValid(itr);)
			{
				auto ptr = containerInfo->m_DereferenceIterator(itr);
				output += m_ItemType->SerializeObject(ptr).value_or("<WTF>");

				if (containerInfo->m_AdvanceIterator(itr); containerInfo->m_IsIteratorValid(itr))
					output += ",";
				else
					break;
			}

			output += "]";
			return output;
		}

		return std::nullopt;
	}

	bool RTTIContainer::DeserializeObject(void *Object, const String& InText) const
	{
		if (m_Type == ERTTIType::Pointer)
		{
			// auto containerType = static_cast<DataPointer *>(m_ContainerType);

			throw std::runtime_error("Deserializing references is not implemented");
		}
		else if (m_Type == ERTTIType::Container)
		{
			auto containerType = static_cast<const DataContainer *>(m_ContainerType);

			// Empty strings erase all elements stored in the array
			if (InText.empty())
			{
				containerType->m_Clear(this, Object);
				return true;
			}

			// Try to parse it
			if (containerType->m_FromString)
				return containerType->m_FromString(InText, this, Object);
		}

		return false;
	}

	std::optional<std::string> RTTIEnum::SerializeObject(const void *Object) const
	{
		//if (m_Type == ERTTIType::Bitfield)
		//	throw std::runtime_error("Serializing enum flags is unimplemented");

		for (auto& member : Values())
		{
			if (memcmp(&member.m_Value, Object, m_Size) == 0)
				return std::format("\"{}\"", member.m_Names[0]);
		}

		return std::nullopt;
	}

	bool RTTIEnum::DeserializeObject(void *Object, const String& InText) const
	{
		if (m_Type == ERTTIType::Bitfield)
		{
			if (!InText.empty())
				throw std::runtime_error("Deserializing enum flags is unimplemented");

			memset(Object, 0, m_Size);
			return true;
		}
		else
		{
			for (auto& member : Values())
			{
				// Try to match any name
				for (auto& name : member.m_Names)
				{
					if (name && name == InText)
					{
						memcpy(Object, &member.m_Value, m_Size);
						return true;
					}
				}
			}
		}

		return false;
	}

	std::vector<std::tuple<const RTTICompound::Attr *, const char *, size_t>> RTTICompound::GetCategorizedClassMembers() const
	{
		// Build a list of all fields from this class and its parent classes
		struct SorterEntry
		{
			size_t m_DeclOrder;
			const Attr *m_Type;
			const char *m_Category;
			uint32_t m_Offset;
			bool m_TopLevel;
		};

		std::vector<SorterEntry> sortedEntries;

		VisitAttributesByInheritance([&](const Attr& Member, const char *Category, uint32_t BaseOffset, bool TopLevel)
		{
			sortedEntries.emplace_back(SorterEntry {
				.m_DeclOrder = sortedEntries.size(),
				.m_Type = &Member,
				.m_Category = Category,
				.m_Offset = BaseOffset + Member.m_Offset,
				.m_TopLevel = TopLevel,
			});

			return false;
		});

		std::ranges::sort(
			sortedEntries,
			[](const auto& A, const auto& B)
		{
			return A.m_Offset < B.m_Offset;
		});

		// Declaration order must be preserved - return the member index as defined in the static RTTI data
		std::vector<std::tuple<const Attr *, const char *, size_t>> out;

		for (auto& entry : sortedEntries)
		{
			// We only care about the top-level fields
			if (!entry.m_TopLevel || entry.m_Type->IsGroup())
				continue;

			out.emplace_back(entry.m_Type, entry.m_Category, entry.m_DeclOrder);
		}

		return out;
	}

	std::optional<std::string> RTTICompound::SerializeObject(const void *Object, size_t MaximumRecursionDepth) const
	{
		thread_local size_t recursionDepthCounter = 0;
		thread_local size_t recursionDepthLimit = 0;

		if (MaximumRecursionDepth != 0)
			recursionDepthLimit = MaximumRecursionDepth;

		// We want the final object type even if this was upcasted
		const static auto rttiObjectType = FindTypeByName("RTTIObject");

		if (IsKindOf(rttiObjectType))
		{
			const auto downcastedType = static_cast<const RTTIObject *>(Object)->GetRTTI();

			if (downcastedType != this)
				return downcastedType->SerializeObject(Object);
		}

		// Check for dedicated decoding functions
		const static auto uuidType = FindTypeByName("GGUUID");

		if (m_ToString)
		{
			if (String str; m_ToString(Object, str))
				return str.c_str();

			return std::nullopt;
		}
		else if (this == uuidType)
		{
			auto& uuid = *static_cast<const GGUUID *>(Object);
			return std::format("\"{}\"", uuid);
		}

		// Manually visit each member in the hierarchy
		auto output = std::format("{{\n\"@type\": \"{}\",", m_TypeName);

		if (recursionDepthLimit == 0 || recursionDepthCounter < recursionDepthLimit)
		{
			recursionDepthCounter++;

			VisitClassMembersByInheritance(
				const_cast<void *>(Object),
				[&](const Attr& Member, void *MemberObject)
			{
				if (!Member.IsProperty())
				{
					auto memberValue = Member.m_Type->SerializeObject(MemberObject).value_or("\"<FAILED>\"");
					output = std::format("{}\n\"{}\": {},", output, Member.m_Name, memberValue);
				}

				return false;
			});

			recursionDepthCounter--;
		}

		if (MaximumRecursionDepth != 0)
			recursionDepthLimit = 0;

		output += "\n}";
		return output;
	}

	bool RTTICompound::DeserializeObject(void *Object, const String& InText) const
	{
		if (m_FromString)
			return m_FromString(Object, InText);

		throw std::runtime_error("Attempting to deserialize a class without a dedicated handler. This is unsupported.");
	}
}
