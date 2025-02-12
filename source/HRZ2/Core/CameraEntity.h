#pragma once

#include "Entity.h"

namespace HRZ2
{
	class CameraEntity : public Entity
	{
	public:
	};

	class UnknownCameraEntityRef : public WeakPtr<CameraEntity>
	{
	public:
		~UnknownCameraEntityRef() = delete;

		char _pad0[0x68];
	};
	static_assert(sizeof(UnknownCameraEntityRef) == 0x80);
}
