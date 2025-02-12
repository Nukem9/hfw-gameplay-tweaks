#pragma once

#include "../PCore/Common.h"

namespace HRZ2
{
	class RTTI;

	class RTTIObjectTweaker
	{
	public:
		class RTTIVisitor
		{
		public:
			enum EMode : int
			{
				MODE_READ = 0,
				MODE_WRITE = 1,
			};

			String m_LastError;										   // 0x8

			virtual ~RTTIVisitor() = default;						   // 0
			virtual void Visit(void *Object, const RTTI *Type) = 0;	   // 1
			virtual EMode GetMode() = 0;							   // 2
		};

		class SetValueVisitor : public RTTIVisitor
		{
		protected:
			String m_ValueToSet; // 0x10

		public:
			virtual ~SetValueVisitor() override = default;					 // 0
			virtual void Visit(void *Object, const RTTI *Type) override = 0; // 1
			virtual EMode GetMode() override = 0;							 // 2
		};
		static_assert(sizeof(SetValueVisitor) == 0x18);

		class SetValueVisitorFunctor : public SetValueVisitor
		{
		public:
			using functor_type = std::function<void(void *Object, const RTTI *Type)>;

			SetValueVisitorFunctor(functor_type Functor) : m_Functor(Functor) {}
			virtual ~SetValueVisitorFunctor() override = default;

			virtual void Visit(void *Object, const RTTI *Type) override
			{
				m_Functor(Object, Type);
			}

			virtual EMode GetMode() override
			{
				return EMode::MODE_WRITE;
			}

		private:
			functor_type m_Functor;
		};

		static void VisitObjectPath(void *Object, const RTTI *Type, const String& Path, SetValueVisitor *Visitor)
		{
			const auto func = Offsets::Signature("4C 8B DC 49 89 4B 08 55 57 41 54 41 55 49 8D 6B B8 48 81 EC 28 01 00 00 41 0F B6 40 04")
								  .ToPointer<void(RTTIObjectTweaker *, void *, const RTTI *, const String&, int, SetValueVisitor&)>();

			return func(nullptr, Object, Type, Path, 0, *Visitor);
		}

		static void VisitObjectPath(void *Object, const RTTI *Type, const String& Path, SetValueVisitorFunctor::functor_type Functor)
		{
			SetValueVisitorFunctor f(Functor);
			VisitObjectPath(Object, Type, Path, &f);
		}
	};
}
