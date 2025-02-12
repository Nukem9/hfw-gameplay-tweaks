#pragma once

#include "../PCore/Common.h"
#include "IStreamingSystem.h"

namespace HRZ2
{
	class RTTI;

	enum class EStreamingRefPriority : uint8_t
	{
		None = 0,
		Lowest = 1,
		Lower = 2,
		Low = 3,
		BelowNormal = 4,
		Normal = 5,
		AboveNormal = 6,
		High = 7,
		Higher = 8,
		Highest = 9,
	};

	enum class EStreamingRefCallbackMode : uint8_t
	{
		NoCallbacks = 0,
		OnLoad = 1,
		OnUnload = 2,
		OnLoadAndUnload = 3,
	};

	class TypedPtrC
	{
	public:
		void *m_Pointer = nullptr;
		const RTTI *m_Type = nullptr;
	};

	class AssetPath
	{
	public:
		String m_Path;					  // 0x0 Unused. Debug only?
		uint32_t m_Unknown1 = 0xFFFFFFFF; // 0x8
	};

	class CoreLink
	{
	public:
		TypedPtrC m_SourcePointer;	// 0x0
		AssetPath m_TargetLocation; // 0x10
		GGUUID m_TargetUUID;		// 0x20
	};
	static_assert(sizeof(CoreLink) == 0x30);

	class StreamingRefProxy
	{
	public:
		struct LocatorData
		{
			AssetPath m_Path;		 // 0x0
			GGUUID m_UUID;			 // 0x10
			RTTIRefObject *m_Object; // 0x20
			uint32_t m_Unknown28;	 // 0x28
			uint32_t m_Unknown2C;	 // 0x2C
		};
		static_assert(sizeof(LocatorData) == 0x30);

		LocatorData *m_LocatorData;			  // 0x0
		uintptr_t m_FlagsAndStreamingManager; // 0x8 Lower 52 bits are a pointer to the StreamingManager instance
	};

	class StreamingRefBase
	{
	private:
		StreamingRefProxy *m_Proxy = nullptr; // 0x0

	public:
		StreamingRefBase() = default;
		StreamingRefBase(const StreamingRefBase&) = delete;
		StreamingRefBase& operator=(const StreamingRefBase&) = delete;

		~StreamingRefBase()
		{
			Clear();
		}

		void Clear()
		{
			const auto func = Offsets::Signature("40 53 48 83 EC 20 48 83 39 00 48 8B D9 74 47 E8")
				.ToPointer<void(StreamingRefBase *)>();

			func(this);
			m_Proxy = nullptr;
		}

		GGUUID GetUUID() const
		{
			return (m_Proxy && m_Proxy->m_LocatorData) ? m_Proxy->m_LocatorData->m_UUID : GGUUID {};
		}

		AssetPath GetLocation() const
		{
			return (m_Proxy && m_Proxy->m_LocatorData) ? m_Proxy->m_LocatorData->m_Path : AssetPath {};
		}

		RTTIRefObject *GetUntypedPtr() const
		{
			if (m_Proxy && m_Proxy->m_LocatorData && ((m_Proxy->m_FlagsAndStreamingManager >> 52) & 0x80) != 0)
				return m_Proxy->m_LocatorData->m_Object;

			return nullptr;
		}
	};

	template<typename T>
	class StreamingRef : public StreamingRefBase
	{
	public:
		StreamingRef() = default;
		StreamingRef(const StreamingRef&) = delete;
		StreamingRef& operator=(const StreamingRef&) = delete;

		T *GetPtr() const
		{
			return static_cast<T *>(GetUntypedPtr());
		}

		T *operator->() const
		{
			return GetPtr();
		}

		T& operator*() const
		{
			return *GetPtr();
		}

		operator T *() const
		{
			return GetPtr();
		}

		explicit operator bool() const
		{
			return GetUntypedPtr() != nullptr;
		}
	};

	class IStreamingRefCallback
	{
	public:
		virtual ~IStreamingRefCallback() = default;							// 0
		virtual void OnLoaded(RTTIRefObject *Object, void *Userdata) = 0;	// 1
		virtual void OnUnloaded(RTTIRefObject *Object, void *Userdata) = 0; // 2
	};

	class IStreamingManager
	{
	public:
		virtual ~IStreamingManager();																											  // 0
		virtual void Register1(CoreLink& Link) = 0;																								  // 1
		virtual void Register2(StreamingRefBase& Ref, const AssetPath& Path, const GGUUID& UUID) = 0;											  // 2
		virtual void Register3(StreamingRefBase& Ref, RTTIRefObject *Object, uint8_t Hint, const AssetPath& Path) = 0;							  // 3
		virtual void Register4(StreamingRefBase& Ref, RTTIRefObject *Object, uint8_t Hint) = 0;													  // 4
		virtual void Unregister(StreamingRefBase& Ref) = 0;																						  // 5
		virtual void RegisterCallback(StreamingRefBase& Ref, EStreamingRefCallbackMode Mode, IStreamingRefCallback *Callback, void *Userdata) = 0;// 6
		virtual void UnregisterCallback(StreamingRefBase& Ref) = 0;																				  // 7
		virtual void Resolve(StreamingRefBase& Ref, EStreamingRefPriority Priority) = 0;														  // 8
	};

	class StreamingManager : public IStreamingManager, public IStreamingSystem::Events
	{
	public:
		char _pad0[0xAA8];
		IStreamingSystem *m_StreamingSystem; // 0xAB8

		virtual ~StreamingManager() override;																										   // 0
		virtual void Register1(CoreLink& Link) override;																							   // 1
		virtual void Register2(StreamingRefBase& Ref, const AssetPath& Path, const GGUUID& UUID) override;											   // 2
		virtual void Register3(StreamingRefBase& Ref, RTTIRefObject *Object, uint8_t Hint, const AssetPath& Path) override;							   // 3
		virtual void Register4(StreamingRefBase& Ref, RTTIRefObject *Object, uint8_t Hint) override;												   // 4
		virtual void Unregister(StreamingRefBase& Ref) override;																					   // 5
		virtual void RegisterCallback(StreamingRefBase& Ref, EStreamingRefCallbackMode Mode, IStreamingRefCallback *Callback, void *Userdata) override;// 6
		virtual void UnregisterCallback(StreamingRefBase& Ref) override;																			   // 7															
		virtual void Resolve(StreamingRefBase& Ref, EStreamingRefPriority Priority) override;														   // 8

		// IStreamingSystem::Events
		virtual void OnFinishLoadGroup(const Array<Ref<RTTIRefObject>>& Objects) override;	 // 0
		virtual void OnBeforeUnloadGroup(const Array<Ref<RTTIRefObject>>& Objects) override; // 1
		virtual void OnLoadAssetGroup(const Array<Ref<RTTIRefObject>>& Objects) override;	 // 2

		static StreamingManager *GetInstance()
		{
			const auto ptr = Offsets::Signature("48 8B ? ? ? ? 07 48 8B 08 48 8B 59 40 48 8D 0D")
				.AsRipRelative(7)
				.ToPointer<StreamingManager *>();

			return *ptr;
		}
	};
	assert_offset(StreamingManager, m_StreamingSystem, 0xAB8);
}
