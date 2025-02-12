#pragma once

namespace HRZ2
{
	class PhysicsCollisionListener
	{
	public:
		virtual ~PhysicsCollisionListener();	 // 0
		virtual bool OnPhysicsContactValidate(); // 1
		virtual void OnPhysicsContactAdded();	 // 2
		virtual void OnPhysicsContactProcess();	 // 3
		virtual void OnPhysicsContactRemoved();	 // 4
	};
}
