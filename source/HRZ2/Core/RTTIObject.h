#pragma once

#include "RTTI.h"

namespace HRZ2
{
	class RTTIObject
	{
	public:
		virtual const RTTI *GetRTTI() const; // 0
		virtual ~RTTIObject();				 // 1

		template<typename T>
		T& GetMemberRefUnsafe(const char *Name)
		{
			return static_cast<const RTTICompound *>(GetRTTI())->GetMemberRefUnsafe<T>(this, Name);
		}
	};
	static_assert(sizeof(RTTIObject) == 0x8);
}
