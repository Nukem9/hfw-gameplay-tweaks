#include <objbase.h>
#include "UUID.h"

namespace HRZ2
{
	GGUUID GGUUID::Generate()
	{
		static_assert(sizeof(GUID) == sizeof(GGUUID));

		GUID winGUID = {};
		CoCreateGuid(&winGUID);

		GGUUID guid = {};
		memcpy(&guid, &winGUID, sizeof(GUID));

		return guid;
	}
}

template<size_t Start, size_t Count, bool SwapEndian>
char *FastEmitUnsignedBytes(char *Out, const auto& Values)
{
	constexpr static char hexLUT[] = "0123456789ABCDEF";

	if constexpr (!SwapEndian)
	{
		for (ptrdiff_t i = 0; i < Count; i++)
		{
			*Out++ = hexLUT[Values[Start + i] >> 4];
			*Out++ = hexLUT[Values[Start + i] & 0xF];
		}
	}
	else
	{
		for (ptrdiff_t i = Count; i > 0; i--)
		{
			*Out++ = hexLUT[Values[Start + i - 1] >> 4];
			*Out++ = hexLUT[Values[Start + i - 1] & 0xF];
		}
	}

	return ++Out;
}

std::format_context::iterator std::formatter<HRZ2::GGUUID>::format(const HRZ2::GGUUID& Value, std::format_context& Context) const
{
	// std::format is hilariously slow in debug builds
	char temp[36];
	memset(temp, '-', sizeof(temp));

	auto p = FastEmitUnsignedBytes<0, 4, true>(temp, Value.All);
	p = FastEmitUnsignedBytes<4, 2, true>(p, Value.All);
	p = FastEmitUnsignedBytes<6, 2, true>(p, Value.All);
	p = FastEmitUnsignedBytes<8, 2, false>(p, Value.All);
	p = FastEmitUnsignedBytes<10, 6, false>(p, Value.All);

	return std::copy_n(temp, std::size(temp), Context.out());
}

fmt::format_context::iterator fmt::formatter<HRZ2::GGUUID>::format(const HRZ2::GGUUID& Value, fmt::format_context& Context) const
{
	char temp[36];
	memset(temp, '-', sizeof(temp));

	auto p = FastEmitUnsignedBytes<0, 4, true>(temp, Value.All);
	p = FastEmitUnsignedBytes<4, 2, true>(p, Value.All);
	p = FastEmitUnsignedBytes<6, 2, true>(p, Value.All);
	p = FastEmitUnsignedBytes<8, 2, false>(p, Value.All);
	p = FastEmitUnsignedBytes<10, 6, false>(p, Value.All);

	return std::copy_n(temp, std::size(temp), Context.out());
}
