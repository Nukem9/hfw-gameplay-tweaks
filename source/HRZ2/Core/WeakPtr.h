#pragma once

namespace HRZ2
{
	class RTTI;
	class WeakPtrBase;
	class WeakPtrRTTITarget;
	class WeakPtrTarget;

	class WeakPtrBase
	{
	protected:
		WeakPtrTarget *m_Ptr = nullptr; // 0x0
		WeakPtrBase *m_Prev = nullptr;  // 0x8
		WeakPtrBase *m_Next = nullptr;  // 0x10

	public:
		WeakPtrBase() = delete;
		~WeakPtrBase() = delete;
	};
	static_assert(sizeof(WeakPtrBase) == 0x18);

	class WeakPtrTarget
	{
	public:
		WeakPtrBase *m_Head = nullptr; // 0x8

		virtual ~WeakPtrTarget();	   // 0
	};
	static_assert(sizeof(WeakPtrTarget) == 0x10);

	class WeakPtrRTTITarget : public WeakPtrTarget
	{
	public:
		virtual ~WeakPtrRTTITarget() override; // 0
		virtual const RTTI *GetRTTI() const;   // 1
	};

	template<typename T>
	class WeakPtr : protected WeakPtrBase
	{
	public:
		WeakPtr() = delete;
		~WeakPtr() = delete;

		T *GetPtr() const
		{
			return static_cast<T *>(m_Ptr);
		}

		T *operator->() const
		{
			return GetPtr();
		}

		operator T *() const
		{
			return GetPtr();
		}

		explicit operator bool() const
		{
			return m_Ptr != nullptr;
		}
	};
}
