	#pragma once

#include "../PCore/Common.h"
#include "PhysicsCollisionListener.h"
#include "RTTIRefObject.h"
#include "SpatialBins2DObject.h"
#include "WeakPtr.h"
#include "WorldTransform.h"
#include "EntityComponent.h"
#include "IStreamingManager.h"

namespace HRZ2
{
	class AIFaction;
	class Destructibility;
	class EntityResource;
	class Model;
	class Mover;
	class Player;

	class EntityResource : public RTTIRefObject
	{
	public:
	};

	class Entity : public RTTIRefObject, public WeakPtrRTTITarget, public PhysicsCollisionListener, public SpatialBins2DObject
	{
	public:
		enum EFlags : int
		{
			FLAG_NONE = 0,
			FLAG_IS_CHANGED = 0x1,
			FLAG_IS_VISIBLE = 0x2,
			FLAG_IS_SLEEPING = 0x200,
		};

		char _pad0[0x30];
		StreamingRef<EntityResource> m_EntityResource; // 0x68
		char _pad1[0x10];
		Entity *m_Parent;							   // 0x80
		Entity *m_LeftMostChild;					   // 0x88
		Entity *m_RightSibling;						   // 0x90
		uint32_t m_Flags;							   // 0x98 atomic
		EntityComponentContainer m_Components;		   // 0xA0
		Mover *m_Mover;								   // 0xC0
		Model *m_Model;								   // 0xC8
		Destructibility *m_Destructibility;			   // 0xD0
		WorldTransform m_WorldTransform;			   // 0xD8 lock
		char _pad3[0x70];
		AIFaction *m_Faction;						   // 0x188
		char _pad4[0x108];							   // Instigator data @ 0x190
		// Update frequency @ 0x218
		mutable RecursiveMutex m_EntityAccessMutex; // 0x298
		char _pad5[0x40];

		virtual const RTTI *GetRTTI() const override;									 // 0
		virtual ~Entity() override;														 // 1
		virtual WorldTransform GetSafePlacementPosition(const WorldTransform& Position); // 4
		virtual void SetEntityResource(EntityResource *Resource);						 // 5
		virtual void UnknownEntity06();													 // 6
		virtual void UnknownEntity07();													 // 7
		virtual const Player *GetPlayer() const;										 // 8
		virtual Player *GetPlayer();													 // 9
		virtual void EnableCollision(bool, bool);										 // 10
		virtual void UnknownEntity11();													 // 11
		virtual void UnknownEntity12();													 // 12
		virtual void UnknownEntity13();													 // 13

		virtual bool OnPhysicsContactValidate() override;								 // 1
		virtual void OnPhysicsContactAdded() override;									 // 2
		virtual void OnPhysicsContactProcess() override;								 // 3
		virtual void OnPhysicsContactRemoved() override;								 // 4

		WorldTransform GetWorldTransform() const
		{
			std::scoped_lock lock(m_EntityAccessMutex);
			return m_WorldTransform;
		}

		void SetWorldTransform(const WorldTransform& Transform)
		{
			std::scoped_lock lock(m_EntityAccessMutex);

			m_WorldTransform = Transform;
			m_Flags |= FLAG_IS_CHANGED;
		}

		void PlaceOn(const WorldTransform& Transform, bool UseSafePlacement)
		{
			auto func = Offsets::Signature("48 89 5C 24 08 48 89 7C 24 10 55 48 8D 6C 24 A9 48 81 EC C0 00 00 00 48 8B F9 45 84 C0 74 1C")
							.ToPointer<void(Entity *, const WorldTransform&, bool)>();

			func(this, Transform, UseSafePlacement);
		}

		void SetFaction(AIFaction *Faction)
		{
			auto func = Offsets::Signature("48 85 C0 74 1A 80 78 28 00 74 14 48 8B 07 48 8B CF FF 50 48")
							.AsAdjusted(-57)
							.ToPointer<void(Entity *, AIFaction *)>();

			func(this, Faction);
		}
	};
	assert_offset(Entity, m_EntityResource, 0x68);
	assert_offset(Entity, m_Parent, 0x80);
	assert_offset(Entity, m_Mover, 0xC0);
	assert_offset(Entity, m_Faction, 0x188);
	assert_offset(Entity, m_EntityAccessMutex, 0x298);
	static_assert(sizeof(Entity) == 0x300);
}
