#pragma once

#include "EntityComponent.h"

namespace HRZ2
{
	enum class EInventoryItemRemoveType : uint8_t
	{
		Destroy = 0,
		Transfer = 1,
		Keep = 2,
	};

	class InventoryItem : public RTTIRefObject, public IStreamingRefCallback, public WeakPtrRTTITarget
	{
	public:
		char _pad0[0x10];
		StreamingRef<EntityResource> m_EntityResource; // 0x48
		uint32_t m_Amount;							   // 0x50
		Entity *m_Owner;							   // 0x58
		char _pad1[0x28];
		Entity *m_ItemEntity;						   // 0x88

		String GetDisplayName() const
		{
			const auto func = Offsets::Signature("40 56 48 83 EC 30 48 89 6C 24 28 48 8D 2D ? ? ? ? 48 89 7C 24 20")
								  .ToPointer<decltype(&InventoryItem::GetDisplayName)>();

			return (this->*func)();
		}
	};
	assert_offset(InventoryItem, m_EntityResource, 0x48);
	assert_offset(InventoryItem, m_ItemEntity, 0x88);

	class Inventory : public EntityComponent /*, public ConditionListener, public FactChangeListener */
	{
	public:
		char _pad0[0x10];
		HashMap<GGUUID, InventoryItem *> m_Items;		   // 0x60

		virtual const RTTI *GetRTTI() const override;	   // 0
		virtual ~Inventory() override;					   // 1
		virtual const RTTI *GetRepresentationType() const; // 4

		uint32_t RemoveItem(Ref<InventoryItem> Item, uint32_t Amount, EInventoryItemRemoveType RemoveType, bool ShowMessage)
		{
			const auto func = Offsets::Signature("44 88 4C 24 20 48 89 4C 24 08 55 53 56 57 41 54 41 57 48 8D 6C 24 D9")
								  .ToPointer<uint32_t(Inventory *, Ref<InventoryItem>, uint32_t, EInventoryItemRemoveType, bool)>();

			return func(this, Item, Amount, RemoveType, ShowMessage);
		}

		Entity *AddItem(EntityResource *Resource, uint32_t Amount, bool ShowMessage)
		{
			const auto func = Offsets::Signature("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 20 55 41 54 41 55 41 56 41 57 48 8D 6C 24 C9")
								  .ToPointer<Entity *(Inventory *, EntityResource *, uint32_t, bool)>();

			return func(this, Resource, Amount, ShowMessage);
		}
	};
	assert_offset(Inventory, m_Items, 0x60);
}
