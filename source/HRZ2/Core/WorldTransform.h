#pragma once

#include "WorldPosition.h"
#include "RotMatrix.h"

namespace HRZ2
{
	class WorldTransform final
	{
	public:
		WorldPosition Position;
		RotMatrix Orientation;
	};
	static_assert(sizeof(WorldTransform) == 0x40);
}
