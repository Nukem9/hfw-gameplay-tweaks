#pragma once

#include <string>
#include "../PCore/Common.h"

namespace HRZ2
{
	class String;

	class RTTI;
	class RTTIAtom;
	class RTTIBitset;
	class RTTIContainer;
	class RTTIEnum;
	class RTTICompound;
	class RTTIPOD;

	enum class ERTTIType : uint8_t
	{
		Atom = 0,
		Pointer = 1,
		Container = 2,
		Enum = 3,
		Compound = 4,
		Bitfield = 5,
		POD = 6,
		Bitset = 7,
	};

	// RTTIDataBase
#pragma pack(push, 1)
	class RTTI
	{
	public:
		using ConstructorFunc = void (*)(const RTTI *, void *);
		using DestructorFunc = void (*)(const RTTI *, void *);
		using FromStringFunc = bool (*)(void *, const String& In);
		using ToStringFunc = bool (*)(const void *In, String& Out);

		enum TypeId : int16_t
		{
			InvalidTypeId = -1,
		};

		TypeId m_TypeIndex;		  // +0x0
		TypeId m_NumDerivedTypes; // +0x2
		ERTTIType m_Type;		  // +0x4
		uint8_t m_UnknownFlags;	  // +0x5

		bool IsExactKindOf(const RTTI *Other) const;
		bool IsKindOf(const RTTI *Other) const;

		const RTTIAtom *AsAtom() const;
		const RTTIContainer *AsContainer() const;
		const RTTIEnum *AsEnum() const;
		const RTTICompound *AsCompound() const;
		const RTTIPOD *AsPOD() const;
		const RTTIBitset *AsBitset() const;

		const RTTI *GetContainedType() const;
		std::string GetSymbolName() const;
		uint64_t GetCoreBinaryTypeId() const;

		void *CreateInstance() const;

		std::optional<std::string> SerializeObject(const void *Object) const;
		bool DeserializeObject(void *Object, const String& InText) const;

		static const RTTI *FindTypeByName(const std::string_view& TypeName);
		static const RTTI *FindTypeByName(const char *TypeName);
	};
	static_assert(sizeof(RTTI) == 0x6);
#pragma pack(pop)

	class RTTIStringSpan
	{
	private:
		const void *m_Data = nullptr;
		uint32_t m_Length = 0;

	public:
		RTTIStringSpan() = default;

		RTTIStringSpan(const String& InString)
		{
			m_Data = InString.c_str(); // TODO: FIX REF COUNT
			m_Length = static_cast<uint32_t>(InString.size());
		}
	};

	class RTTIIter
	{
	private:
		RTTI *m_ContainerType = nullptr;
		void *m_Container = nullptr;
		uint32_t m_UserData = 0;
	};
	static_assert(sizeof(RTTIIter) == 0x18);

	// RTTIDataAtom (float, int, bool, String)
	class RTTIAtom : public RTTI
	{
	public:
		using CopyFunc = void (*)(void *Out, const void *In);
		using IsEqualFunc = bool (*)(const void *, const void *);
		using SerializeFunc = bool (*)(void *In, void *Out, bool SwapEndian);
		using DeserializeFunc = bool (*)(void *In, void *Out); // bool SwapEndian?
		using GetSerializeSizeFunc = int (*)(const void *);
		using RangeCheckFunc = bool (*)(const void *, const char *, const char *);

		uint16_t m_Size;												 // +0x6
		uint8_t m_Alignment;											 // +0x8
		bool m_IsSimple;												 // +0x9
		const char *m_TypeName;											 // +0x10
		RTTI *m_BaseType;												 // +0x18
		bool (*m_FromStringSpan)(const RTTIStringSpan& Span, void *Out); // +0x20
		ToStringFunc m_ToString;										 // +0x28
		void *m_UnknownFunction1;										 // +0x30
		CopyFunc m_CopyFunc;											 // +0x38
		IsEqualFunc m_IsEqualFunc;										 // +0x40
		ConstructorFunc m_Constructor;									 // +0x48
		DestructorFunc m_Destructor;									 // +0x50
		SerializeFunc m_Serialize;										 // +0x58
		DeserializeFunc m_Deserialize;									 // +0x60
		GetSerializeSizeFunc m_GetSerializeSize;						 // +0x68
		RangeCheckFunc m_RangeCheck;									 // +0x70

		std::optional<std::string> SerializeObject(const void *Object) const;
		bool DeserializeObject(void *Object, const String& InText) const;
	};
	assert_offset(RTTIAtom, m_Size, 0x6);
	assert_offset(RTTIAtom, m_TypeName, 0x10);
	assert_offset(RTTIAtom, m_Serialize, 0x58);

	// RTTIDataContainer (Ref<>, UUIDRef<>, StreamingRef<>, WeakPtr<>) and (Array<>, HashMap<>, HashSet<>)
	class RTTIContainer : public RTTI
	{
	public:
		struct Data
		{
			const char *m_TypeName; // +0x0
		};

		struct DataPointer : Data
		{
			uint32_t m_Size;									  // +0x8
			uint32_t m_Alignment;								  // +0xC
			void (*m_Constructor)(const RTTI *, void *);		  // +0x10
			void (*m_Destructor)(const RTTI *, void *);			  // +0x18
			void *(*m_Getter)(const RTTI *, const void *);		  // +0x20
			bool (*m_Setter)(const RTTI *, void **Out, void *In); // +0x28
			void (*m_Copier)(void **Out, void **In);			  // +0x30
		};

		struct DataContainer : Data
		{
			uint16_t m_Size;															  // +0x8
			uint8_t m_Alignment;														  // +0xA
			bool m_IsSimple;															  // +0xB
			bool m_IsAssociative;														  // +0xC
			void (*m_Constructor)(const RTTI *, void *);								  // +0x10
			void (*m_Destructor)(const RTTI *, void *);									  // +0x18
			bool (*m_Resize)(const RTTI *, void *, int NewSize, bool);					  // +0x20
			char _pad0[0x8];
			bool (*m_Remove)(const RTTI *, void *, int Index);							  // +0x30
			int (*m_GetNumItems)(const RTTI *, const void *);							  // +0x38
			void *(*m_GetItem)(const RTTI *, const void *, int);						  // +0x40
			RTTIIter (*m_GetBeginIterator)(const RTTI *, const void *);					  // +0x48
			RTTIIter (*m_GetEndIterator)(const RTTI *, const void *);					  // +0x50
			void (*m_AdvanceIterator)(RTTIIter&);										  // +0x58
			void *(*m_DereferenceIterator)(const RTTIIter&);							  // +0x60
			bool (*m_IsIteratorValid)(const RTTIIter&);									  // +0x68
			char _pad1[0x10];
			RTTIIter (*m_AddItem)(const RTTI *, void *, void *);						  // +0x80
			RTTIIter (*m_AddEmpty)(const RTTI *, void *);								  // +0x88
			bool (*m_Clear)(const RTTI *, void *);										  // +0x90
			bool (*m_ToString)(const void *, const RTTI *, String& Out);				  // +0x98
			bool (*m_FromString)(const RTTIStringSpan& Span, const RTTI *, const void *); // +0xA0
		};
		assert_offset(DataContainer, m_GetNumItems, 0x38);
		assert_offset(DataContainer, m_Clear, 0x90);

		bool m_HasPointers;			// +0x6
		RTTI *m_ItemType;			// +0x8
		Data *m_ContainerType;		// +0x10
		const char *m_FullTypeName; // +0x18

		std::optional<std::string> SerializeObject(const void *Object) const;
		bool DeserializeObject(void *Object, const String& InText) const;
	};
	assert_offset(RTTIContainer, m_HasPointers, 0x6);
	assert_offset(RTTIContainer, m_ContainerType, 0x10);

	// RTTIDataEnum
	class RTTIEnum : public RTTI
	{
	public:
		class Value
		{
		public:
			int m_Value;			// +0x0
			const char *m_Names[4]; // +0x8
			char _pad28[0x8];		// +0x28
		};
		static_assert(sizeof(Value) == 0x30);

		uint8_t m_Size;			  // +0x6
		uint16_t m_NumValues;	  // +0x8
		uint8_t m_Alignment;	  // +0xA
		const char *m_TypeName;	  // +0x10
		Value *m_Values;		  // +0x18
		RTTI *m_PODOptimizedType; // +0x20

		auto Values() const
		{
			return std::span<const Value> { m_Values, m_NumValues };
		}

		std::optional<std::string> SerializeObject(const void *Object) const;
		bool DeserializeObject(void *Object, const String& InText) const;
	};
	assert_offset(RTTIEnum, m_Size, 0x6);
	assert_offset(RTTIEnum, m_TypeName, 0x10);

	// RTTIDataCompound
	class RTTICompound : public RTTI
	{
	public:
		using GetFunc = void (*)(const void *, void *);
		using SetFunc = void (*)(void *, const void *);
		using MessageHandlerFunc = void (*)(void *, void *);
		using GetExporterSymbolsFunc = const RTTI *(*)();

		class Base
		{
		public:
			RTTI *m_Type;
			uint32_t m_Offset;
		};
		static_assert(sizeof(Base) == 0x10);

		class Attr
		{
		public:
			enum Flags : uint16_t
			{
				ATTR_DONT_SERIALIZE_BINARY = 2,
				ATTR_VALID_FLAG_MASK = 3563,
			};

			RTTI *m_Type;
			uint16_t m_Offset;
			Flags m_Flags;
			const char *m_Name;
			GetFunc m_GetFunc;
			SetFunc m_SetFunc;
			const char *m_RangeMin;
			const char *m_RangeMax;

			bool IsGroup() const
			{
				return m_Type == nullptr;
			}

			bool IsSaveStateOnly() const
			{
				return (m_Flags & ATTR_DONT_SERIALIZE_BINARY) == ATTR_DONT_SERIALIZE_BINARY;
			}

			bool IsProperty() const
			{
				return m_GetFunc || m_SetFunc;
			}
		};
		static_assert(sizeof(Attr) == 0x38);

		class MessageHandler
		{
		public:
			RTTI *m_Message;			  // MsgReadBinary/MsgInit/MsgXXX
			MessageHandlerFunc m_Handler; // Callback
		};
		static_assert(sizeof(MessageHandler) == 0x10);

		class MessageOrderEntry
		{
		public:
			bool m_Before;
			RTTI *m_Message;
			RTTI *m_Compound;
		};
		static_assert(sizeof(MessageOrderEntry) == 0x18);

		uint8_t m_BaseCount;							 // +0x6 Determines number of entries for m_Bases
		uint8_t m_AttributeCount;						 // +0x7 Determines number of entries for m_Attributes
		uint8_t m_MessageHandlerCount;					 // +0x9 Determines number of entries for m_MessageHandlers
		uint8_t m_MessageOrderEntryCount;				 // +0xA Determines number of entries for m_MessageOrderEntries
		char _pad2[0x3];								 // +0xB
		uint16_t m_Version;								 // +0xE
		uint32_t m_Size;								 // +0x14
		uint16_t m_Alignment;							 // +0x18
		uint16_t m_SerializeFlags;						 // +0x1A
		ConstructorFunc m_Constructor;					 // +0x18
		DestructorFunc m_Destructor;					 // +0x20
		FromStringFunc m_FromString;					 // +0x28
		ToStringFunc m_ToString;						 // +0x30
		char _pad3[0x8];								 // +0x38
		const char *m_TypeName;							 // +0x40
		uint32_t m_CachedTypeNameHash;					 // +0x48 fullgame.dll resolution?
		char _pad4[0xC];								 // +0x4C
		Base *m_Bases;									 // +0x60
		Attr *m_Attributes;								 // +0x68
		MessageHandler *m_MessageHandlers;				 // +0x78
		MessageOrderEntry *m_MessageOrderEntries;		 // +0x80
		GetExporterSymbolsFunc m_GetSymbolGroupFunc;	 // +0x88

		auto Bases() const
		{
			return std::span<const Base> { m_Bases, m_BaseCount };
		}

		auto Members() const
		{
			return std::span<const Attr> { m_Attributes, m_AttributeCount };
		}

		auto MessageHandlers() const
		{
			return std::span<const MessageHandler> { m_MessageHandlers, m_MessageHandlerCount };
		}

		auto MessageOrderEntries() const
		{
			return std::span<const MessageOrderEntry> { m_MessageOrderEntries, m_MessageOrderEntryCount };
		}

		template<typename T>
		bool SetMemberValue(void *Object, const char *Name, const T& Value) const
		{
			return VisitClassMembersByInheritance(
				Object,
				[&](const Attr& Member, void *MemberObject)
				{
					if (strcmp(Name, Member.m_Name) != 0)
						return false;

					if (Member.IsProperty())
						Member.m_SetFunc(Object, &Value);
					else
						*reinterpret_cast<T *>(MemberObject) = Value;

					return true;
				});
		}

		template<typename T>
		bool GetMemberValue(const void *Object, const char *Name, T *OutValue) const
		{
			return VisitClassMembersByInheritance(
				const_cast<void *>(Object),
				[&](const Attr& Member, void *MemberObject)
				{
					if (strcmp(Name, Member.m_Name) != 0)
						return false;

					if (OutValue)
					{
						if (Member.IsProperty())
							Member.m_GetFunc(Object, OutValue);
						else
							*OutValue = *reinterpret_cast<std::add_const_t<T> *>(MemberObject);
					}

					return true;
				});
		}

		template<typename T>
		T& GetMemberRefUnsafe(void *Object, const char *Name) const
		{
			void *memberObjectPointer = nullptr;

			VisitClassMembersByInheritance(
				Object,
				[&](const Attr& Member, void *MemberObject)
			{
				if (strcmp(Name, Member.m_Name) != 0)
					return false;

				if (Member.IsProperty())
					throw std::runtime_error("Cannot obtain a reference to a property function");

				memberObjectPointer = MemberObject;
				return true;
			});

			if (!memberObjectPointer)
				throw std::runtime_error("Couldn't resolve member name");

			return *reinterpret_cast<T *>(memberObjectPointer);
		}

		template<std::predicate<const Attr& /*Member*/, void */*MemberObject*/> Func>
		bool VisitClassMembersByInheritance(void *Object, Func&& Callback) const
		{
			return VisitAttributesByInheritance([&](const Attr& Member, const char *, uint32_t BaseOffset, bool)
			{
				if (Member.IsGroup())
					return false;

				auto rawObject = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(Object) + BaseOffset + Member.m_Offset);
				return Callback(Member, rawObject);
			});
		}

		template<std::predicate<const Attr& /*Member*/, const char */*Category*/, uint32_t /*BaseOffset*/, bool /*TopLevel*/> Func>
		bool VisitAttributesByInheritance(Func&& Callback, uint32_t BaseOffset = 0, bool TopLevel = true) const
		{
			for (auto& base : Bases())
			{
				if (static_cast<const RTTICompound *>(base.m_Type)->VisitAttributesByInheritance(
					Callback,
					BaseOffset + base.m_Offset,
					false))
					return true;
			}

			for (const char *activeCategory = ""; auto& member : Members())
			{
				if (member.IsGroup())
					activeCategory = member.m_Name;

				if (Callback(member, activeCategory, BaseOffset, TopLevel))
					return true;
			}

			return false;
		}

		std::vector<std::tuple<const Attr *, const char *, size_t>> GetCategorizedClassMembers() const;
		std::optional<std::string> SerializeObject(const void *Object, size_t MaximumRecursionDepth = 0) const;
		bool DeserializeObject(void *Object, const String& InText) const;
	};
	assert_offset(RTTICompound, m_BaseCount, 0x6);
	assert_offset(RTTICompound, m_Size, 0x10);
	assert_offset(RTTICompound, m_Constructor, 0x18);
	assert_offset(RTTICompound, m_TypeName, 0x40);
	assert_offset(RTTICompound, m_Bases, 0x58);
	assert_offset(RTTICompound, m_Attributes, 0x60);
	assert_offset(RTTICompound, m_MessageOrderEntries, 0x70);

	class RTTIPOD : public RTTI
	{
	public:
		// Type 6 = Plain old data. Doesn't exist in static RTTI. They're generated at runtime by
		// combining fields for optimization purposes.
		uint32_t m_Size; // +0x8
	};
	assert_offset(RTTIPOD, m_Size, 0x8);

	class RTTIBitset : public RTTI
	{
	public:
		// Type 7 = Enumeration bit set. I don't understand the purpose of this compared to standard enum flags.
		RTTI *m_Type;			// +0x8
		const char *m_TypeName; // +0x10
	};
	assert_offset(RTTIBitset, m_TypeName, 0x10);
}
