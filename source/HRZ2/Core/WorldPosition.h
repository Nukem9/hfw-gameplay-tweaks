#pragma once

#include "Vec.h"

namespace HRZ2
{
	class WorldPosition final
	{
	public:
		double X = 0.0;
		double Y = 0.0;
		double Z = 0.0;

		WorldPosition() = default;
		WorldPosition(double aX, double aY, double aZ) : X(aX), Y(aY), Z(aZ) {}

		WorldPosition& operator+=(const Vec3& Other)
		{
			X += Other.X;
			Y += Other.Y;
			Z += Other.Z;

			return *this;
		}

		WorldPosition& operator-=(const Vec3& Other)
		{
			X -= Other.X;
			Y -= Other.Y;
			Z -= Other.Z;

			return *this;
		}

		WorldPosition& operator+=(const Vec3Pack& Other)
		{
			X += Other.X;
			Y += Other.Y;
			Z += Other.Z;

			return *this;
		}

		WorldPosition& operator-=(const Vec3Pack& Other)
		{
			X -= Other.X;
			Y -= Other.Y;
			Z -= Other.Z;

			return *this;
		}
	};
	static_assert(sizeof(WorldPosition) == 0x18);
}
