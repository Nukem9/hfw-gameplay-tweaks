#pragma once

struct IDXGIFactory1;
struct IDXGISwapChain3;

namespace HRZ2
{
	class NxDXGIImpl
	{
	public:
		char _pad0[0x28];
		IDXGIFactory1 *m_DXGIFactory;
		IDXGISwapChain3 *m_DXGISwapChain;
		uint32_t m_NumBuffers;
	};
	assert_offset(NxDXGIImpl, m_DXGIFactory, 0x28);
	assert_offset(NxDXGIImpl, m_NumBuffers, 0x38);
}
