#pragma once

#include "../PCore/Common.h"

namespace HRZ2
{
	class JobHeaderCPU
	{
	private:
		virtual void UnknownJobHeaderCPU00(); // 0
		virtual ~JobHeaderCPU();			  // 1
		virtual void Destruct();			  // 2

		char _pad0[0x30]; // 0x00
		short m_RefCount; // 0x38

	public:
		void AddRef()
		{
			_InterlockedIncrement16(&m_RefCount);
		}

		void Release()
		{
			if (_InterlockedDecrement16(&m_RefCount) == 0)
				Destruct();
		}

		static Ref<JobHeaderCPU> CreateCallableJob(void (*Callback)(void *UserData), void *UserData)
		{
			const auto func = Offsets::Signature("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 BA 10 00 00 00 48 8B F1")
								  .ToPointer<Ref<JobHeaderCPU>(void *, void *, void *, void (*)(void *))>();

			return func(nullptr, nullptr, UserData, Callback);
		}

		static Ref<JobHeaderCPU> CreateCallableJob(std::function<void()> Callback)
		{
			auto userData = new std::function<void()>(std::move(Callback));
			auto wrapper = +[](void *Userdata)
			{
				auto callback = static_cast<decltype(userData)>(Userdata);
				(*callback)();

				delete callback;
			};

			return CreateCallableJob(wrapper, userData);
		}

		static void SubmitCallback(void (*Callback)())
		{
			auto job = CreateCallableJob(reinterpret_cast<void (*)(void *)>(Callback), nullptr);
			SubmitJob(job);
		}

		static void SubmitCallable(std::function<void()> Callback)
		{
			auto job = CreateCallableJob(std::move(Callback));
			SubmitJob(job);
		}

		static void SubmitJob(Ref<JobHeaderCPU> Job)
		{
			const auto mainJobQueue = Offsets::Signature("48 8D 0D ? ? ? ? C6 40 3B 02 48 8B 13 E8 ? ? ? ? 49 FF C6")
										  .AsRipRelative(7)
										  .ToPointer<void>();

			const auto func = Offsets::Signature("40 57 41 57 48 83 EC 28 48 8B FA 4C 8B F9 0F B6 52 3A")
								  .ToPointer<void(void *, JobHeaderCPU *)>();

			func(mainJobQueue, Job);
		}
	};
}
