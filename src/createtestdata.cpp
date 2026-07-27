#include "DTAFile.hpp"
#include "utility.hpp"

#include <array>
#include <bit>
#include <charconv>
#include <cstdint>
#include <format>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

// Add test data for the chosen variable type
template <typename T>
void AddTestData(dta::DTAFile& dtafile, std::string_view type, std::uint8_t version) {
	std::string varname = std::string(type) + "_";
	dtafile.AddVariable(varname, type);
	dtafile.SetVariableLabel(varname, std::format("{:s} variable", type));

	if constexpr (std::is_integral_v<T>) {
		dtafile.SetValue(varname, 1, T(0));
		if (version < 112) {
			dtafile.SetValue(varname, 2, dta::stata_limits_v1<T>::lowest());
			dtafile.SetValue(varname, 3, dta::stata_limits_v1<T>::max());
			dtafile.SetValue(varname, 4, dta::stata_limits_v1<T>::missing());
		}
		else {
			dtafile.SetValue(varname, 2, dta::stata_limits_v2<T>::lowest());
			dtafile.SetValue(varname, 3, dta::stata_limits_v2<T>::max());
			dtafile.SetValue(varname, 4, dta::stata_limits_v2<T>::missing());
			dtafile.SetValue(varname, 5, dta::stata_limits_v2<T>::missingA());
			dtafile.SetValue(varname, 6, dta::stata_limits_v2<T>::missingB());
			dtafile.SetValue(varname, 7, dta::stata_limits_v2<T>::missingC());
			dtafile.SetValue(varname, 8, dta::stata_limits_v2<T>::missingD());
			dtafile.SetValue(varname, 9, dta::stata_limits_v2<T>::missingE());
			dtafile.SetValue(varname, 10, dta::stata_limits_v2<T>::missingF());
			dtafile.SetValue(varname, 11, dta::stata_limits_v2<T>::missingG());
			dtafile.SetValue(varname, 12, dta::stata_limits_v2<T>::missingH());
			dtafile.SetValue(varname, 13, dta::stata_limits_v2<T>::missingI());
			dtafile.SetValue(varname, 14, dta::stata_limits_v2<T>::missingJ());
			dtafile.SetValue(varname, 15, dta::stata_limits_v2<T>::missingK());
			dtafile.SetValue(varname, 16, dta::stata_limits_v2<T>::missingL());
			dtafile.SetValue(varname, 17, dta::stata_limits_v2<T>::missingM());
			dtafile.SetValue(varname, 18, dta::stata_limits_v2<T>::missingN());
			dtafile.SetValue(varname, 19, dta::stata_limits_v2<T>::missingO());
			dtafile.SetValue(varname, 20, dta::stata_limits_v2<T>::missingP());
			dtafile.SetValue(varname, 21, dta::stata_limits_v2<T>::missingQ());
			dtafile.SetValue(varname, 22, dta::stata_limits_v2<T>::missingR());
			dtafile.SetValue(varname, 23, dta::stata_limits_v2<T>::missingS());
			dtafile.SetValue(varname, 24, dta::stata_limits_v2<T>::missingT());
			dtafile.SetValue(varname, 25, dta::stata_limits_v2<T>::missingU());
			dtafile.SetValue(varname, 26, dta::stata_limits_v2<T>::missingV());
			dtafile.SetValue(varname, 27, dta::stata_limits_v2<T>::missingW());
			dtafile.SetValue(varname, 28, dta::stata_limits_v2<T>::missingX());
			dtafile.SetValue(varname, 29, dta::stata_limits_v2<T>::missingY());
			dtafile.SetValue(varname, 30, dta::stata_limits_v2<T>::missingZ());
		}
	}

	if constexpr (std::is_floating_point_v<T>) {
		dtafile.SetValue(varname, 1, T(0));
		if (version < 108) {
			dtafile.SetValue(varname, 2, dta::stata_limits_v1<T>::lowest());
			dtafile.SetValue(varname, 3, dta::stata_limits_v1<T>::max());
			dtafile.SetValue(varname, 4, dta::stata_limits_v1<T>::missing());
		}
		else {
			dtafile.SetValue(varname, 2, dta::stata_limits_v2<T>::lowest());
			dtafile.SetValue(varname, 3, dta::stata_limits_v2<T>::max());
			dtafile.SetValue(varname, 4, dta::stata_limits_v2<T>::missing());
			dtafile.SetValue(varname, 5, dta::stata_limits_v2<T>::missingA());
			dtafile.SetValue(varname, 6, dta::stata_limits_v2<T>::missingB());
			dtafile.SetValue(varname, 7, dta::stata_limits_v2<T>::missingC());
			dtafile.SetValue(varname, 8, dta::stata_limits_v2<T>::missingD());
			dtafile.SetValue(varname, 9, dta::stata_limits_v2<T>::missingE());
			dtafile.SetValue(varname, 10, dta::stata_limits_v2<T>::missingF());
			dtafile.SetValue(varname, 11, dta::stata_limits_v2<T>::missingG());
			dtafile.SetValue(varname, 12, dta::stata_limits_v2<T>::missingH());
			dtafile.SetValue(varname, 13, dta::stata_limits_v2<T>::missingI());
			dtafile.SetValue(varname, 14, dta::stata_limits_v2<T>::missingJ());
			dtafile.SetValue(varname, 15, dta::stata_limits_v2<T>::missingK());
			dtafile.SetValue(varname, 16, dta::stata_limits_v2<T>::missingL());
			dtafile.SetValue(varname, 17, dta::stata_limits_v2<T>::missingM());
			dtafile.SetValue(varname, 18, dta::stata_limits_v2<T>::missingN());
			dtafile.SetValue(varname, 19, dta::stata_limits_v2<T>::missingO());
			dtafile.SetValue(varname, 20, dta::stata_limits_v2<T>::missingP());
			dtafile.SetValue(varname, 21, dta::stata_limits_v2<T>::missingQ());
			dtafile.SetValue(varname, 22, dta::stata_limits_v2<T>::missingR());
			dtafile.SetValue(varname, 23, dta::stata_limits_v2<T>::missingS());
			dtafile.SetValue(varname, 24, dta::stata_limits_v2<T>::missingT());
			dtafile.SetValue(varname, 25, dta::stata_limits_v2<T>::missingU());
			dtafile.SetValue(varname, 26, dta::stata_limits_v2<T>::missingV());
			dtafile.SetValue(varname, 27, dta::stata_limits_v2<T>::missingW());
			dtafile.SetValue(varname, 28, dta::stata_limits_v2<T>::missingX());
			dtafile.SetValue(varname, 29, dta::stata_limits_v2<T>::missingY());
			dtafile.SetValue(varname, 30, dta::stata_limits_v2<T>::missingZ());
		}
	}

	if constexpr (std::is_same_v<T, std::string>) {
		std::uint16_t slen{};
		auto [ptr, ec] = std::from_chars(type.data() + 3, type.data() + type.size(), slen);
		if (ptr == type.data() + type.size() && ec == std::errc{}) {
			dtafile.SetValue(varname, 1, std::string(""));
			dtafile.SetValue(varname, 2, std::string(slen, 'X'));
		}
		else {
			std::println(std::cerr, "Unrecognised string type");
		}
	}

	if constexpr (std::is_same_v<T, dta::strL>) {
		dtafile.SetValue(varname, 1, dta::strL(""));
		dtafile.SetValue(varname, 2, dta::strL("abcdefghijklmnopqrstuvwxyz"));
		dtafile.SetValue(varname, 3, dta::strL(std::vector<char>({ 'a','b', 'c', 'd', 'e' })));
	}
}

// Create test datasets for each supported file version
int main(int argc, char* argv[]) {
	for (const auto& ver : { 102, 103, 104, 105 }) {
		dta::DTAFile stata_floatlimit;
		stata_floatlimit.AddVariable("float_", "f");
		stata_floatlimit.AddVariable("double_", "d");
		stata_floatlimit.SetObservations(2);
		stata_floatlimit.SetValue("float_", 1, dta::stata_limits_v1<float>::lowest());
		stata_floatlimit.SetValue("float_", 2, dta::stata_limits_v1<float>::max());
		stata_floatlimit.SetValue("double_", 1, dta::stata_limits_v1<double>::lowest());
		stata_floatlimit.SetValue("double_", 2, dta::stata_limits_v1<double>::max());
		stata_floatlimit.Save(std::format("stata_float_validranges_{:d}_le.dta", ver), ver, std::endian::little);
		stata_floatlimit.Save(std::format("stata_float_validranges_{:d}_be.dta", ver), ver, std::endian::big);
	}
	for (const auto& ver : { 108, 110, 111, 113, 114, 119 }) {
		dta::DTAFile stata_floatlimit;
		stata_floatlimit.AddVariable("float_", "f");
		stata_floatlimit.AddVariable("double_", "d");
		stata_floatlimit.SetObservations(2);
		stata_floatlimit.SetValue("float_", 1, dta::stata_limits_v2<float>::lowest());
		stata_floatlimit.SetValue("float_", 2, dta::stata_limits_v2<float>::max());
		stata_floatlimit.SetValue("double_", 1, dta::stata_limits_v2<double>::lowest());
		stata_floatlimit.SetValue("double_", 2, dta::stata_limits_v2<double>::max());
		stata_floatlimit.Save(std::format("stata_float_validranges_{:d}_le.dta", ver), ver, std::endian::little);
		stata_floatlimit.Save(std::format("stata_float_validranges_{:d}_be.dta", ver), ver, std::endian::big);
	}
	for (const auto& ver : { 102, 103, 104, 105, 108, 110, 111 }) {
		dta::DTAFile intlimits_v1(ver);
		intlimits_v1.SetDataLabel(std::format("Integer limits ({:d} format)", ver));
		intlimits_v1.SetObservations(2);
		if (ver >= 104) {
			intlimits_v1.AddVariable("byte_", "byte");
		}
		else {
			intlimits_v1.AddVariable("byte_", "int");
		}
		intlimits_v1.SetValue("byte_", 1, dta::stata_limits_v1<std::int8_t>::lowest());
		intlimits_v1.SetValue("byte_", 2, dta::stata_limits_v1<std::int8_t>::max());
		intlimits_v1.AddVariable("int_", "int");
		intlimits_v1.SetValue("int_", 1, dta::stata_limits_v1<std::int16_t>::lowest());
		intlimits_v1.SetValue("int_", 2, dta::stata_limits_v1<std::int16_t>::max());
		intlimits_v1.AddVariable("long_", "long");
		intlimits_v1.SetValue("long_", 1, dta::stata_limits_v1<std::int32_t>::lowest());
		intlimits_v1.SetValue("long_", 2, dta::stata_limits_v1<std::int32_t>::max());
		intlimits_v1.Save(std::format("stata_int_validranges{:d}_le.dta", ver), ver, std::endian::little);
		intlimits_v1.Save(std::format("stata_int_validranges{:d}_be.dta", ver), ver, std::endian::big);
	}
	for (const auto& ver : { 113, 114, 115, 117, 118, 119 }) {
		dta::DTAFile intlimits_v2(ver);
		intlimits_v2.SetDataLabel(std::format("Integer limits ({:d} format)", ver));
		intlimits_v2.SetObservations(2);
		intlimits_v2.AddVariable("byte_", "byte");
		intlimits_v2.SetValue("byte_", 1, dta::stata_limits_v2<std::int8_t>::lowest());
		intlimits_v2.SetValue("byte_", 2, dta::stata_limits_v2<std::int8_t>::max());
		intlimits_v2.AddVariable("int_", "int");
		intlimits_v2.SetValue("int_", 1, dta::stata_limits_v2<std::int16_t>::lowest());
		intlimits_v2.SetValue("int_", 2, dta::stata_limits_v2<std::int16_t>::max());
		intlimits_v2.AddVariable("long_", "long");
		intlimits_v2.SetValue("long_", 1, dta::stata_limits_v2<std::int32_t>::lowest());
		intlimits_v2.SetValue("long_", 2, dta::stata_limits_v2<std::int32_t>::max());
		intlimits_v2.Save(std::format("stata_int_validranges{:d}_le.dta", ver), ver, std::endian::little);
		intlimits_v2.Save(std::format("stata_int_validranges{:d}_be.dta", ver), ver, std::endian::big);
	}

	std::array<std::uint8_t, 16> versions{ 102, 103, 104, 105, 108, 110, 111, 112, 113, 114, 115, 117, 118, 119, 120, 121 };

	for (const auto& version : versions) {
		dta::DTAFile newfile(version);
		newfile.SetDataLabel(std::format("Test file for Stata {:d} format", version));
		newfile.SetObservations(30);
		if (version > 103) {
			AddTestData<std::int8_t>(newfile, "byte", version);
		}
		AddTestData<std::int16_t>(newfile, "int", version);
		AddTestData<std::int32_t>(newfile, "long", version);
		AddTestData<float>(newfile, "float", version);
		AddTestData<double>(newfile, "double", version);

		if (version > 102) {
			AddTestData<std::string>(newfile, "str1", version);
			AddTestData<std::string>(newfile, "str80", version);
			if (version > 110 && version != 112) {
				AddTestData<std::string>(newfile, "str244", version);
				if (version > 115) {
					AddTestData<std::string>(newfile, "str2025", version);
				}
			}
		}
		if (version > 115) {
			AddTestData<dta::strL>(newfile, "strL", version);
		}
		if (version == 120 || version == 121) {
			AddTestData<std::monostate>(newfile, "alias", version);
		}
		newfile.AddVariable("lbltest", "int");
		newfile.SetVariableLabel("lbltest", "Value label test");
		newfile.SetValue("lbltest", 1, 1);
		newfile.SetValue("lbltest", 2, 2);
		newfile.SetValue("lbltest", 3, 3);
		newfile.SetValue("lbltest", 4, 4);

		newfile.AddValueLabel("testlbl", 1, "one");
		newfile.AddValueLabel("testlbl", 2, "two");
		newfile.AddValueLabel("testlbl", 3, "three");
		newfile.AddValueLabel("testlbl", 4, "four");

		newfile.AssignValueLabel("lbltest", "testlbl");

		newfile.AddCharacteristic("_dta", "test", "Test Characteristic");
		newfile.StampTime();
		if (newfile.Save(std::format("statatest_{:d}_le.dta", version), version, std::endian::little) == true) {
			dta::DTAFile newfiletest(std::format("statatest_{:d}_le.dta", version));
			Summary(newfiletest);
			Description(newfiletest);
		}
		if (newfile.Save(std::format("statatest_{:d}_be.dta", version), version, std::endian::big) == true) {
			dta::DTAFile newfiletest(std::format("statatest_{:d}_be.dta", version));
			Summary(newfiletest);
			Description(newfiletest);
		}

		newfile.RenameVariable("long_", "longint");
		newfile.RemoveVariable("int_");
		Description(newfile);
		newfile.RemoveValueLabel("testlbl", 1);
		newfile.RemoveValueLabel("testlbl", 2);
		newfile.RemoveValueLabel("testlbl", 3);
		newfile.RemoveValueLabel("testlbl", 4);
		newfile.RemoveCharacteristic("_dta", "test");
		Print(newfile);

		auto val1 = newfile.GetValue<std::int32_t>("longint", 2);
		newfile.SortData({ "longint", "float_" });
		Print(newfile);
	}

	return 0;
}