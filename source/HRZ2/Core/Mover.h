#pragma once

#include "EntityComponent.h"
#include "WorldTransform.h"

namespace HRZ2
{
	class Mover : public EntityComponent
	{
	public:
		virtual const RTTI *GetRTTI() const override;											  // 0
		virtual ~Mover() override;																  // 1
		virtual bool IsActive();																  // 12
		virtual void SetActive(bool Active);													  // 13
		virtual void OverrideMovement(const WorldTransform& Transform, float MoveDuration, bool); // 14
		virtual void UpdateOverrideMovementTarget(const WorldTransform& Transform);				  // 15
		virtual bool IsMovementOverridden();													  // 16
		virtual void StopOverrideMovement();													  // 17
		virtual float GetOverrideMovementDuration();											  // 18
		virtual float GetOverrideMovementTime();												  // 19
		virtual float GetOverrideMovementSpeed();												  // 20
		// ...

		char _pad50[0x8]; // 0x50
	};
	static_assert(sizeof(Mover) == 0x58);
}
