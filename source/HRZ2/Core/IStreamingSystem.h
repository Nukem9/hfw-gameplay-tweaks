#pragma once

#include "../PCore/Common.h"

namespace HRZ2
{
	class RTTIRefObject;

	class IStreamingSystem
	{
	public:
		class Events
		{
		public:
			virtual void OnFinishLoadGroup(const Array<Ref<RTTIRefObject>>& Objects) = 0;	// 0 After OnLoadAssetGroup
			virtual void OnBeforeUnloadGroup(const Array<Ref<RTTIRefObject>>& Objects) = 0; // 1
			virtual void OnLoadAssetGroup(const Array<Ref<RTTIRefObject>>& Objects) = 0;	// 2 Before resolving pointers/MsgInit
		};

		virtual ~IStreamingSystem();								   // 0
		virtual bool Initialize();									   // 1
		virtual void Shutdown();									   // 2
		virtual void RegisterEventHandler(Events *EventHandler) = 0;   // 3
		virtual void UnregisterEventHandler(Events *EventHandler) = 0; // 4
		virtual bool UnknownIStreamingSystem05();					   // 5
		virtual void UnknownIStreamingSystem06() = 0;				   // 6
		virtual void UnknownIStreamingSystem07() = 0;				   // 7
		virtual void UnknownIStreamingSystem08() = 0;				   // 8
		virtual void UnknownIStreamingSystem09();					   // 9
		virtual bool UnknownIStreamingSystem10();					   // 10
		virtual void UnknownIStreamingSystem11() = 0;				   // 11
		virtual void UnknownIStreamingSystem12() = 0;				   // 12
		virtual void UnknownIStreamingSystem13() = 0;				   // 13
		virtual void UnknownIStreamingSystem14() = 0;				   // 14
		virtual void UnknownIStreamingSystem15();					   // 15
		virtual void UnknownIStreamingSystem16() = 0;				   // 16
	};
}
