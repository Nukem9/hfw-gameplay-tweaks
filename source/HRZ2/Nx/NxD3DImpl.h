#pragma once

struct ID3D12CommandQueue;
struct ID3D12Device;
struct ID3D12Resource;

namespace HRZ2
{
	class NxD3DImpl
	{
	public:
		static NxD3DImpl *GetSingleton()
		{
			const auto ptr = Offsets::Signature("48 8B 0D ? ? ? ? 8B D3 4C 8B 01 41 FF 90 ? ? 00 00 48 81 C4 ? 01 00 00 5B C3")
								 .AsRipRelative(7)
								 .ToPointer<NxD3DImpl *>();

			return *ptr;
		}

		ID3D12Device *GetD3D12Device()
		{
			return (*(ID3D12Device *(__fastcall **)(NxD3DImpl *))(*(uint64_t *)this + 224))(this);
		}

		ID3D12Resource *GetD3D12GameBackBuffer(uint32_t Node)
		{
			return (*(ID3D12Resource *(__fastcall **)(NxD3DImpl *, uint32_t))(*(uint64_t *)this + 240))(this, Node);
		}

		ID3D12CommandQueue *GetD3D12CommandQueue(uint32_t Node)
		{
			return (*(ID3D12CommandQueue *(__fastcall **)(NxD3DImpl *, uint32_t))(*(uint64_t *)this + 264))(this, Node);
		}
	};
}
