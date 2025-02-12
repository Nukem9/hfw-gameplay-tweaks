#pragma once

#include <unordered_set>
#include <format>
#include <stdio.h>

namespace HRZ2
{
	class RTTI;

	class RTTIYamlExporter
	{
	private:
		FILE *m_FileHandle = nullptr;
		std::vector<const RTTI *> m_Types;
		uint32_t m_IndentSpaces = 0;

	public:
		RTTIYamlExporter() = delete;
		RTTIYamlExporter(const RTTIYamlExporter&) = delete;
		RTTIYamlExporter(const std::unordered_set<const RTTI *>& Types);
		RTTIYamlExporter& operator=(const RTTIYamlExporter&) = delete;

		void ExportRTTITypes(const std::string_view& Directory);

	private:
		void ExportMetaHeader();
		void ExportGGRTTI();
		void EmitAtomTypes();
		void EmitCompoundTypes();
		void EmitEnumTypes();
		void EmitBitsetTypes();
		void IncreaseIndent();
		void DecreaseIndent();

		template<typename... TArgs>
		void Print(const std::string_view& Format, TArgs&&...Args)
		{
			char indentation[128] = {};
			auto formatted = std::vformat(Format, std::make_format_args(Args...));

			for (uint32_t i = 0; i < m_IndentSpaces; i++)
				indentation[i] = ' ';

			fprintf(m_FileHandle, "%s%s\n", indentation, formatted.c_str());
		}

		static std::string EscapeString(std::string Value);
	};
}
