#pragma once

#include "../PCore/Common.h"
#include "RTTIRefObject.h"
#include "WeakPtr.h"

namespace HRZ2
{
	class Entity;
	class EntityComponentRep;
	class EntityComponentResource;
	class NetEntityComponentState;

	template<typename T>
	class EntityMessageProcessing
	{
	public:
	};

	class EntityComponentResource : public RTTIRefObject
	{
	public:
	};

	class EntityComponent : public RTTIRefObject, public WeakPtrRTTITarget, public EntityMessageProcessing<EntityComponent>
	{
	public:
		Ref<EntityComponentResource> m_Resource;					 // 0x30
		bool m_IsInitialized;										 // 0x38
		EntityComponentRep *m_Representation;						 // 0x40
		Entity *m_Entity;											 // 0x48

		virtual const RTTI *GetRTTI() const override;				 // 0
		virtual ~EntityComponent();									 // 1
		virtual const RTTI *GetRepresentationType() const;			 // 4
		virtual void SetEntity(Entity *Entity);						 // 5
		virtual void SetResource(EntityComponentResource *Resource); // 6
		virtual void UnknownEntityComponent07();					 // 7
		virtual void UnknownEntityComponent08();					 // 8
		virtual void UnknownEntityComponent09();					 // 9
		virtual void UnknownEntityComponent10();					 // 10
		virtual NetEntityComponentState *CreateNetState();			 // 11
	};
	assert_offset(EntityComponent, m_Resource, 0x30);
	static_assert(sizeof(EntityComponent) == 0x50);

	class EntityComponentContainer
	{
	public:
		Array<EntityComponent *> m_Components; // 0x0
		Array<uint16_t> m_ComponentTypes;	   // 0x10

		template<typename T = EntityComponent>
		T *FindComponentByRTTI(const RTTI *RTTI) const
		{
			for (auto& component : m_Components)
			{
				if (component->GetRTTI() == RTTI)
					return static_cast<T *>(component);
			}

			return nullptr;
		}
	};
	static_assert(sizeof(EntityComponentContainer) == 0x20);
}
