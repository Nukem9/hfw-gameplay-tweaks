#pragma once

#include <spdlog/spdlog.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Hooking/Hooks.h"
#include "Hooking/Offsets.h"
#include "Hooking/Memory.h"

#define assert_offset(Type, Member, Offset) static_assert(offsetof(Type, Member) == Offset, "Offset mismatch")

template<size_t N>
requires(N > 1)
class XorStr
{
private:
	const uint8_t m_Seed;
	std::array<char, N - 1> m_RawBytes;

public:
	__forceinline consteval XorStr(const char (&Input)[N]) noexcept : m_Seed(static_cast<uint8_t>(Input[0]) | 0x80)
	{
#if !defined(_DEBUG)
		for (size_t i = 0; i < N - 1; i++)
			m_RawBytes[i] = Input[i] ^ m_Seed;
#else
		for (size_t i = 0; i < N - 1; i++)
			m_RawBytes[i] = Input[i];
#endif
	}

	__forceinline std::string_view Decrypt() noexcept
	{
#if !defined(_DEBUG)
		for (auto& c : m_RawBytes)
			*std::launder(&c) ^= m_Seed;
#endif

		return std::string_view(m_RawBytes.data(), m_RawBytes.size());
	}

	__forceinline operator std::string_view() noexcept
	{
		return Decrypt();
	}
};
