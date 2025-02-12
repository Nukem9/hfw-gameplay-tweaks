#pragma once

namespace HRZ2
{
	class PhysicsConstraintListener
	{
	public:
		virtual ~PhysicsConstraintListener();  // 0
		virtual void OnConstraintBroken() = 0; // 1
	};
}
