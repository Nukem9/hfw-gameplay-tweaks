#pragma once

#include "../PCore/Common.h"
#include "RTTIObject.h"

namespace HRZ2
{
	class RTTIRefObject : public RTTIObject
	{
	public:
		GGUUID m_UUID;								  // 0x8
		uint32_t m_FlagsAndRefCount = 0;			  // 0x18

		virtual const RTTI *GetRTTI() const override; // 0
		virtual ~RTTIRefObject() override;			  // 1
		virtual void GetReferencedObjects1();		  // 2
		virtual void GetReferencedObjects2();		  // 3

		void AddRef()
		{
			_InterlockedIncrement(reinterpret_cast<volatile long *>(&m_FlagsAndRefCount));
		}

		void Release()
		{
			const auto func = Offsets::Signature("40 53 48 83 EC 20 48 8B D9 B8 FF FF FF FF F0 0F C1 41 18 25 FF FF 3F 00")
				.ToPointer<void(RTTIRefObject *)>();

			func(this);
		}
	};
	static_assert(sizeof(RTTIRefObject) == 0x20);
}
