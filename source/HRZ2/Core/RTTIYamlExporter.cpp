#include <filesystem>
#include <algorithm>
#include "RTTI.h"
#include "RTTIYamlExporter.h"

namespace HRZ2
{
	RTTIYamlExporter::RTTIYamlExporter(const std::unordered_set<const RTTI *>& Types) : m_Types(Types.begin(), Types.end())
	{
		// Always sort by name during export
		std::ranges::sort(
			m_Types,
			[](const auto *A, const auto *B)
			{
				return A->GetSymbolName() < B->GetSymbolName();
			});
	}

	void RTTIYamlExporter::ExportRTTITypes(const std::string_view& Directory)
	{
		std::filesystem::create_directories(Directory);

		auto outputPath = std::format("{0:}\\Decima.HFW.RTTI.yaml", Directory);

		if (fopen_s(&m_FileHandle, outputPath.c_str(), "w") == 0)
		{
			Print("---");
			ExportMetaHeader();
			ExportGGRTTI();
			Print("...");

			fclose(m_FileHandle);
		}
	}

	void RTTIYamlExporter::ExportMetaHeader()
	{
		Print("meta:");
		IncreaseIndent();

		Print("game: \"Horizon Forbidden West\"");
		Print("codename: \"HRZ2\"");
		Print("platform: \"PC\"");
		Print("rtti_binary_version: 1010");
		Print("type_name_version: 1");

		Print("class_member_flags:");
		IncreaseIndent();
		Print("- {{name: \"savestate\", flag: 2, comment: \"Determines if a class member is serialized in the save file\"}}");
		Print("- {{name: \"attribute_mask\", flag: 65003, comment: \"Valid flag mask when generating attribute lists\"}}");
		DecreaseIndent();

		DecreaseIndent();
	}

	void RTTIYamlExporter::ExportGGRTTI()
	{
		Print("atom:");
		IncreaseIndent();
		EmitAtomTypes();
		DecreaseIndent();

		Print("compound:");
		IncreaseIndent();
		EmitCompoundTypes();
		DecreaseIndent();

		Print("enum:");
		IncreaseIndent();
		EmitEnumTypes();
		DecreaseIndent();

		Print("bitset:");
		IncreaseIndent();
		EmitBitsetTypes();
		DecreaseIndent();
	}

	void RTTIYamlExporter::EmitAtomTypes()
	{
		for (auto type : m_Types)
		{
			if (auto asAtom = type->AsAtom())
			{
				Print("{0:}:", asAtom->GetSymbolName());

				IncreaseIndent();
				Print("typeid: {0:}", asAtom->GetCoreBinaryTypeId());
				Print("size: {0:}", asAtom->m_Size);
				if (asAtom->m_BaseType != type)
					Print("base: \"{0:}\"", asAtom->m_BaseType->GetSymbolName());
				DecreaseIndent();
			}
		}
	}

	void RTTIYamlExporter::EmitCompoundTypes()
	{
		for (auto type : m_Types)
		{
			if (auto asCompound = type->AsCompound())
			{
				Print("{0:}:", asCompound->GetSymbolName());

				IncreaseIndent();
				Print("typeid: {0:}", asCompound->GetCoreBinaryTypeId());
				Print("size: {0:}", asCompound->m_Size);
				Print("alignment: {0:}", asCompound->m_Alignment);
				Print("flags: {0:}", asCompound->m_SerializeFlags);
				Print("version: {0:}", asCompound->m_Version);

				if (auto messages = asCompound->MessageHandlers(); !messages.empty())
				{
					Print("messages:");
					IncreaseIndent();
					for (auto& message : messages)
						Print("- \"{0:}\"", message.m_Message->GetSymbolName());
					DecreaseIndent();
				}

				if (auto bases = asCompound->Bases(); !bases.empty())
				{
					Print("bases:");
					IncreaseIndent();
					for (auto& base : bases)
						Print("- {{name: \"{0:}\", offset: {1:}}}", base.m_Type->GetSymbolName(), base.m_Offset);
					DecreaseIndent();
				}

				if (auto members = asCompound->GetCategorizedClassMembers(); !members.empty())
				{
					Print("members:");
					IncreaseIndent();
					for (auto& [member, category, order] : members)
					{
						Print(
							"- {{name: \"{0:}\", type: \"{1:}\", category: \"{2:}\", offset: {3:}, flags: {4:}, is_property: {5:s}, order: "
							"{6:}}}",
							member->m_Name,
							member->m_Type->GetSymbolName(),
							category,
							member->m_Offset,
							std::to_underlying(member->m_Flags),
							member->IsProperty(),
							order);
					}
					DecreaseIndent();
				}

				DecreaseIndent();
			}
		}
	}

	void RTTIYamlExporter::EmitEnumTypes()
	{
		for (auto type : m_Types)
		{
			if (auto asEnum = type->AsEnum())
			{
				Print("{0:}:", asEnum->GetSymbolName());

				IncreaseIndent();
				Print("typeid: {0:}", asEnum->GetCoreBinaryTypeId());
				Print("size: {0:}", asEnum->m_Size);
				Print("is_flags: {0:s}", asEnum->m_Type == HRZ2::ERTTIType::Bitfield);

				if (auto values = asEnum->Values(); !values.empty())
				{
					Print("values:");
					IncreaseIndent();
					for (auto& entry : values)
					{
						uint64_t value = 0;
						memcpy(&value, &entry.m_Value, std::min<size_t>(sizeof(entry.m_Value), asEnum->m_Size));

						for (auto subName : entry.m_Names)
						{
							if (subName)
								Print("- {{name: \"{0:}\", value: {1:}}}", EscapeString(subName), value);
						}
					}
					DecreaseIndent();
				}

				DecreaseIndent();
			}
		}
	}

	void RTTIYamlExporter::EmitBitsetTypes()
	{
		for (auto type : m_Types)
		{
			if (auto asBitset = type->AsBitset())
			{
				Print("{0:}:", asBitset->GetSymbolName());

				IncreaseIndent();
				Print("typeid: {0:}", asBitset->GetCoreBinaryTypeId());
				Print("base: \"{0:}\"", asBitset->m_Type->GetSymbolName());
				DecreaseIndent();
			}
		}
	}

	void RTTIYamlExporter::IncreaseIndent()
	{
		m_IndentSpaces += 2;
	}

	void RTTIYamlExporter::DecreaseIndent()
	{
		m_IndentSpaces -= 2;
	}

	std::string RTTIYamlExporter::EscapeString(std::string Value)
	{
		using namespace std::literals::string_view_literals;

		auto replaceAll = [&](const std::string_view From, const std::string_view To)
		{
			for (size_t startPos = 0; (startPos = Value.find(From, startPos)) != std::string::npos;)
			{
				Value.replace(startPos, From.length(), To);
				startPos += To.length();
			}
		};

		replaceAll("\\"sv, "\\\\"sv);
		replaceAll("\""sv, "\\\""sv);
		return Value;
	}
}
