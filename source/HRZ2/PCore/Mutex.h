#pragma once

namespace HRZ2
{
	class SharedMutex final
	{
	private:
		uint8_t alignas(alignof(void *)) m_WinLockData[0x8];

	public:
		SharedMutex();
		SharedMutex(const SharedMutex&) = delete;
		~SharedMutex() = default;
		SharedMutex& operator=(const SharedMutex&) = delete;

		void lock();
		bool try_lock();
		void unlock();

		void lock_shared();
		bool try_lock_shared();
		void unlock_shared();
	};

	class RecursiveMutex final
	{
	private:
		uint8_t alignas(alignof(void *)) m_WinLockData[0x28];

	public:
		RecursiveMutex();
		RecursiveMutex(const RecursiveMutex&) = delete;
		~RecursiveMutex();
		RecursiveMutex& operator=(const RecursiveMutex&) = delete;

		void lock();
		bool try_lock();
		void unlock();
	};
}
