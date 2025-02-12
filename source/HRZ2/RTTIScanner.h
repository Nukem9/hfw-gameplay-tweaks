#pragma once

#include <unordered_set>

namespace HRZ2
{
	class RTTI;
}

namespace RTTIScanner
{
	const std::unordered_set<const HRZ2::RTTI *>& GetAllTypes();

	void ScanForRTTIStructures();
	void RegisterTypeInfoRecursively(const HRZ2::RTTI *Info);
	void RegisterRTTIStructures();
}
