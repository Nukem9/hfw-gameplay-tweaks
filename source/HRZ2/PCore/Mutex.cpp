#include <Windows.h>
#include "Mutex.h"

#define GET_SRW_LOCK()		   std::launder(reinterpret_cast<SRWLOCK *>(&m_WinLockData))
#define GET_CRITICAL_SECTION() std::launder(reinterpret_cast<CRITICAL_SECTION *>(&m_WinLockData))

namespace HRZ2
{
	static_assert(sizeof(SharedMutex) == sizeof(SRWLOCK));
	static_assert(alignof(SharedMutex) == alignof(SRWLOCK));

	static_assert(sizeof(RecursiveMutex) == sizeof(CRITICAL_SECTION));
	static_assert(alignof(RecursiveMutex) == alignof(CRITICAL_SECTION));

	SharedMutex::SharedMutex()
	{
		InitializeSRWLock(GET_SRW_LOCK());
	}

	void SharedMutex::lock()
	{
		AcquireSRWLockExclusive(GET_SRW_LOCK());
	}

	bool SharedMutex::try_lock()
	{
		return TryAcquireSRWLockExclusive(GET_SRW_LOCK());
	}

	void SharedMutex::unlock()
	{
		ReleaseSRWLockExclusive(GET_SRW_LOCK());
	}

	void SharedMutex::lock_shared()
	{
		AcquireSRWLockShared(GET_SRW_LOCK());
	}

	bool SharedMutex::try_lock_shared()
	{
		return TryAcquireSRWLockShared(GET_SRW_LOCK());
	}

	void SharedMutex::unlock_shared()
	{
		ReleaseSRWLockShared(GET_SRW_LOCK());
	}

	RecursiveMutex::RecursiveMutex()
	{
		InitializeCriticalSection(GET_CRITICAL_SECTION());
	}

	RecursiveMutex::~RecursiveMutex()
	{
		DeleteCriticalSection(GET_CRITICAL_SECTION());
	}

	void RecursiveMutex::lock()
	{
		EnterCriticalSection(GET_CRITICAL_SECTION());
	}

	bool RecursiveMutex::try_lock()
	{
		return TryEnterCriticalSection(GET_CRITICAL_SECTION());
	}

	void RecursiveMutex::unlock()
	{
		LeaveCriticalSection(GET_CRITICAL_SECTION());
	}
}
