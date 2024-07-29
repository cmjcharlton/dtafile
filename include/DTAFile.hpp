// DTAFile.h : Class for reading and writing files in the Stata DTA format
// Does not check that the time stamp is in expected format or is a valid date
// Does not handle character set conversion
// Does not check whether variable format descriptions are valid in Stata

#ifndef DTAFILE_H
#define DTAFILE_H

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <print>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace dta {
	constexpr std::uint8_t default_version{ 119 };

	struct dtainformation {
		std::uint8_t varname_width;
		std::uint8_t format_width;
		std::uint16_t varlab_width;
	};

	static std::vector<std::pair<std::uint8_t, dtainformation> > dta_sizes = {
		{102, {9, 7, 32}}, /* Stata 1 */
		{103, {9, 7, 32}}, /* Stata 2 */
		{104, {9, 7, 32}}, /* Stata 2.1, 3 */
		{105, {9, 12, 32}}, /* Stata 4, 5 */
		{108, {9, 12, 81}}, /* Stata 6 */
		{110, {33, 12, 81}}, /* Stata 7 */
		{111, {33, 12, 81}}, /* Stata 7 SE */
		{112, {33, 12, 81}}, /* Stata 8, 9 (I haven't encountered this version in the wild) */
		{113, {33, 12, 81}}, /* Stata 8, 9 */
		{114, {33, 49, 81}}, /* Stata 10 */
		{115, {33, 49, 81}}, /* Stata 11, 12 */
		{117, {33, 49, 81}}, /* Stata 13 */
		{118, {129, 57, 321}}, /* Stata 14, 15, 16, 17, 18 */
		{119, {129, 57, 321}}, /* Stata 15, 16, 17, 18 */
		{120, {129, 57, 321}}, /* Stata 18 */
		{121, {129, 57, 321}}, /* Stata 18 */
	};

	struct mapdef {
		std::int64_t hdr;
		std::int64_t map;
		std::int64_t types;
		std::int64_t varnames;
		std::int64_t srtlist;
		std::int64_t fmts;
		std::int64_t vlblnames;
		std::int64_t varlabs;
		std::int64_t chars;
		std::int64_t data;
		std::int64_t strls;
		std::int64_t vallabs;
		std::int64_t tlr;
		std::int64_t bot;
	};

	using strL = std::variant<
		std::string,
		std::vector<char>
	>;

	using ColumnData = std::variant <
		std::monostate,
		std::vector<std::int8_t>,
		std::vector<std::int16_t>,
		std::vector<std::int32_t>,
		std::vector<float>,
		std::vector<double>,
		std::vector<std::string>,
		std::vector<strL>
	>;

	using CellData = std::variant <
		std::monostate,
		std::int8_t,
		std::int16_t,
		std::int32_t,
		float,
		double,
		std::string,
		strL
	>;

	template<class... Ts>
	struct overloaded : Ts... { using Ts::operator()...; };

	inline dtainformation getVersionInformation(std::uint8_t version) {
		for (auto& [ver, info] : dta_sizes) {
			if (ver == version) {
				return info;
			}
		}
		return dtainformation{};
	}

	constexpr std::array<std::string_view, 35> reserved_words = { "alias", "_all", "_b", "byte", "_coef", "_cons", "double", "float", "if", "in", "int", "long", "_n", "_N", "_pi", "_pred", "_r_b", "_rc", "_r_ci", "_r_cri", "_r_crlb", "_r_crub", "_r_df", "_r_lb", "_r_p", "_r_se", "_r_ub", "_r_z", "_r_z_abs", "_se", "_skip", "str#", "strL", "using", "with"};

	template <class T>
	class stata_limits_v1 : public std::numeric_limits<T> {
		[[nodiscard]] static constexpr T(missing)() noexcept {
			return T();
		}
	};

	template <>
	class stata_limits_v1<std::int8_t> : public std::numeric_limits<std::int8_t> {
	public:
		[[nodiscard]] static constexpr std::int8_t(min)() noexcept {
			return -128;
		}

		[[nodiscard]] static constexpr std::int8_t(lowest)() noexcept {
			return (min)();
		}

		[[nodiscard]] static constexpr std::int8_t(max)() noexcept {
			return 126;
		}

		[[nodiscard]] static constexpr std::int8_t(missing)() noexcept {
			return 127;
		}
	};

	template <>
	class stata_limits_v1<std::int16_t> : public std::numeric_limits<std::int16_t> {
	public:
		[[nodiscard]] static constexpr std::int16_t(min)() noexcept {
			return -32768;
		}

		[[nodiscard]] static constexpr std::int16_t(lowest)() noexcept {
			return (min)();
		}

		[[nodiscard]] static constexpr std::int16_t(max)() noexcept {
			return 32766;
		}

		[[nodiscard]] static constexpr std::int16_t(missing)() noexcept {
			return 32767;
		}
	};

	template <>
	class stata_limits_v1<std::int32_t> : public std::numeric_limits<std::int32_t> {
	public:
		[[nodiscard]] static constexpr std::int32_t(min)() noexcept {
			return -2147483648;
		}

		[[nodiscard]] static constexpr std::int32_t(lowest)() noexcept {
			return (min)();
		}

		[[nodiscard]] static constexpr std::int32_t(max)() noexcept {
			return 2147483646;
		}

		[[nodiscard]] static constexpr std::int32_t(missing)() noexcept {
			return 2147483647;
		}
	};

	// Values taken from Stata 1 manual
	// I do not have access to the appropriate historic software or later manuals to confirm these values
	// Recent Stata versions open data with values outside these ranges without converting them to missing
	template <>
	class stata_limits_v1<float> : public std::numeric_limits<float> {
	public:
		[[nodiscard]] static constexpr float(lowest)() noexcept {
			return -1.0e37f;
		}

		[[nodiscard]] static constexpr float(max)() noexcept {
			return 1.0e37f;
		}

		[[nodiscard]] static constexpr float(missing)() noexcept {
			return 0x1.000p127f; // Documented value (0x1.0p128f) is outside range of float, so use value taken from a historic worksheets
		}
	};

	// Values taken from Stata 1 manual
	// I do not have access to the appropriate historic software or later manuals to confirm these values
	// Recent Stata versions open data with values outside these ranges without converting them to missing
	template <>
	class stata_limits_v1<double> : public std::numeric_limits<double> {
	public:
		[[nodiscard]] static constexpr double(lowest)() noexcept {
			return -1.0e99;
		}

		[[nodiscard]] static constexpr double(max)() noexcept {
			return 1.0e99;
		}

		[[nodiscard]] static constexpr double(missing)() noexcept {
			return 0x1.0p333;
		}
	};

	template <class T>
	class stata_limits_v2 : public std::numeric_limits<T> {
		[[nodiscard]] static constexpr T(missing)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingA)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingB)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingC)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingD)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingE)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingF)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingG)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingH)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingI)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingJ)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingK)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingL)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingM)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingN)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingO)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingP)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingQ)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingR)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingS)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingT)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingU)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingV)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingW)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingX)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingY)() noexcept {
			return T();
		}

		[[nodiscard]] static constexpr T(missingZ)() noexcept {
			return T();
		}
	};

	template <>
	class stata_limits_v2<std::int8_t> : public std::numeric_limits<std::int8_t> {
	public:
		[[nodiscard]] static constexpr std::int8_t(min)() noexcept {
			return -127;
		}

		[[nodiscard]] static constexpr std::int8_t(lowest)() noexcept {
			return (min)();
		}

		[[nodiscard]] static constexpr std::int8_t(max)() noexcept {
			return 100;
		}

		[[nodiscard]] static constexpr std::int8_t(missing)() noexcept {
			return 101;
		}

		[[nodiscard]] static constexpr std::int8_t(missingA)() noexcept {
			return 102;
		}

		[[nodiscard]] static constexpr std::int8_t(missingB)() noexcept {
			return 103;
		}

		[[nodiscard]] static constexpr std::int8_t(missingC)() noexcept {
			return 104;
		}

		[[nodiscard]] static constexpr std::int8_t(missingD)() noexcept {
			return 105;
		}

		[[nodiscard]] static constexpr std::int8_t(missingE)() noexcept {
			return 106;
		}

		[[nodiscard]] static constexpr std::int8_t(missingF)() noexcept {
			return 107;
		}

		[[nodiscard]] static constexpr std::int8_t(missingG)() noexcept {
			return 108;
		}

		[[nodiscard]] static constexpr std::int8_t(missingH)() noexcept {
			return 109;
		}

		[[nodiscard]] static constexpr std::int8_t(missingI)() noexcept {
			return 110;
		}

		[[nodiscard]] static constexpr std::int8_t(missingJ)() noexcept {
			return 111;
		}

		[[nodiscard]] static constexpr std::int8_t(missingK)() noexcept {
			return 112;
		}

		[[nodiscard]] static constexpr std::int8_t(missingL)() noexcept {
			return 113;
		}

		[[nodiscard]] static constexpr std::int8_t(missingM)() noexcept {
			return 114;
		}

		[[nodiscard]] static constexpr std::int8_t(missingN)() noexcept {
			return 115;
		}

		[[nodiscard]] static constexpr std::int8_t(missingO)() noexcept {
			return 116;
		}

		[[nodiscard]] static constexpr std::int8_t(missingP)() noexcept {
			return 117;
		}

		[[nodiscard]] static constexpr std::int8_t(missingQ)() noexcept {
			return 118;
		}

		[[nodiscard]] static constexpr std::int8_t(missingR)() noexcept {
			return 119;
		}

		[[nodiscard]] static constexpr std::int8_t(missingS)() noexcept {
			return 120;
		}

		[[nodiscard]] static constexpr std::int8_t(missingT)() noexcept {
			return 121;
		}

		[[nodiscard]] static constexpr std::int8_t(missingU)() noexcept {
			return 122;
		}

		[[nodiscard]] static constexpr std::int8_t(missingV)() noexcept {
			return 123;
		}

		[[nodiscard]] static constexpr std::int8_t(missingW)() noexcept {
			return 124;
		}

		[[nodiscard]] static constexpr std::int8_t(missingX)() noexcept {
			return 125;
		}

		[[nodiscard]] static constexpr std::int8_t(missingY)() noexcept {
			return 126;
		}

		[[nodiscard]] static constexpr std::int8_t(missingZ)() noexcept {
			return 127;
		}
	};

	template <>
	class stata_limits_v2<std::int16_t> : public std::numeric_limits<std::int16_t> {
	public:
		[[nodiscard]] static constexpr std::int16_t(min)() noexcept {
			return -32767;
		}

		[[nodiscard]] static constexpr std::int16_t(lowest)() noexcept {
			return (min)();
		}

		[[nodiscard]] static constexpr std::int16_t(max)() noexcept {
			return 32740;
		}

		[[nodiscard]] static constexpr std::int16_t(missing)() noexcept {
			return 32741;
		}

		[[nodiscard]] static constexpr std::int16_t(missingA)() noexcept {
			return 32742;
		}

		[[nodiscard]] static constexpr std::int16_t(missingB)() noexcept {
			return 32743;
		}

		[[nodiscard]] static constexpr std::int16_t(missingC)() noexcept {
			return 32744;
		}

		[[nodiscard]] static constexpr std::int16_t(missingD)() noexcept {
			return 32745;
		}

		[[nodiscard]] static constexpr std::int16_t(missingE)() noexcept {
			return 32746;
		}

		[[nodiscard]] static constexpr std::int16_t(missingF)() noexcept {
			return 32747;
		}

		[[nodiscard]] static constexpr std::int16_t(missingG)() noexcept {
			return 32748;
		}

		[[nodiscard]] static constexpr std::int16_t(missingH)() noexcept {
			return 32749;
		}

		[[nodiscard]] static constexpr std::int16_t(missingI)() noexcept {
			return 32750;
		}

		[[nodiscard]] static constexpr std::int16_t(missingJ)() noexcept {
			return 32751;
		}

		[[nodiscard]] static constexpr std::int16_t(missingK)() noexcept {
			return 32752;
		}

		[[nodiscard]] static constexpr std::int16_t(missingL)() noexcept {
			return 32753;
		}

		[[nodiscard]] static constexpr std::int16_t(missingM)() noexcept {
			return 32754;
		}

		[[nodiscard]] static constexpr std::int16_t(missingN)() noexcept {
			return 32755;
		}

		[[nodiscard]] static constexpr std::int16_t(missingO)() noexcept {
			return 32756;
		}

		[[nodiscard]] static constexpr std::int16_t(missingP)() noexcept {
			return 32757;
		}

		[[nodiscard]] static constexpr std::int16_t(missingQ)() noexcept {
			return 32758;
		}

		[[nodiscard]] static constexpr std::int16_t(missingR)() noexcept {
			return 32759;
		}

		[[nodiscard]] static constexpr std::int16_t(missingS)() noexcept {
			return 32760;
		}

		[[nodiscard]] static constexpr std::int16_t(missingT)() noexcept {
			return 32761;
		}

		[[nodiscard]] static constexpr std::int16_t(missingU)() noexcept {
			return 32762;
		}

		[[nodiscard]] static constexpr std::int16_t(missingV)() noexcept {
			return 32763;
		}

		[[nodiscard]] static constexpr std::int16_t(missingW)() noexcept {
			return 32764;
		}

		[[nodiscard]] static constexpr std::int16_t(missingX)() noexcept {
			return 32765;
		}

		[[nodiscard]] static constexpr std::int16_t(missingY)() noexcept {
			return 32766;
		}

		[[nodiscard]] static constexpr std::int16_t(missingZ)() noexcept {
			return 32767;
		}
	};

	template <>
	class stata_limits_v2<std::int32_t> : public std::numeric_limits<std::int32_t> {
	public:
		[[nodiscard]] static constexpr std::int32_t(min)() noexcept {
			return -2147483647;
		}

		[[nodiscard]] static constexpr std::int32_t(lowest)() noexcept {
			return (min)();
		}

		[[nodiscard]] static constexpr std::int32_t(max)() noexcept {
			return 2147483620;
		}

		[[nodiscard]] static constexpr std::int32_t(missing)() noexcept {
			return 2147483621;
		}

		[[nodiscard]] static constexpr std::int32_t(missingA)() noexcept {
			return 2147483622;
		}

		[[nodiscard]] static constexpr std::int32_t(missingB)() noexcept {
			return 2147483623;
		}

		[[nodiscard]] static constexpr std::int32_t(missingC)() noexcept {
			return 2147483624;
		}

		[[nodiscard]] static constexpr std::int32_t(missingD)() noexcept {
			return 2147483625;
		}

		[[nodiscard]] static constexpr std::int32_t(missingE)() noexcept {
			return 2147483626;
		}

		[[nodiscard]] static constexpr std::int32_t(missingF)() noexcept {
			return 2147483627;
		}

		[[nodiscard]] static constexpr std::int32_t(missingG)() noexcept {
			return 2147483628;
		}

		[[nodiscard]] static constexpr std::int32_t(missingH)() noexcept {
			return 2147483629;
		}

		[[nodiscard]] static constexpr std::int32_t(missingI)() noexcept {
			return 2147483630;
		}

		[[nodiscard]] static constexpr std::int32_t(missingJ)() noexcept {
			return 2147483631;
		}

		[[nodiscard]] static constexpr std::int32_t(missingK)() noexcept {
			return 2147483632;
		}

		[[nodiscard]] static constexpr std::int32_t(missingL)() noexcept {
			return 2147483633;
		}

		[[nodiscard]] static constexpr std::int32_t(missingM)() noexcept {
			return 2147483634;
		}

		[[nodiscard]] static constexpr std::int32_t(missingN)() noexcept {
			return 2147483635;
		}

		[[nodiscard]] static constexpr std::int32_t(missingO)() noexcept {
			return 2147483636;
		}

		[[nodiscard]] static constexpr std::int32_t(missingP)() noexcept {
			return 2147483637;
		}

		[[nodiscard]] static constexpr std::int32_t(missingQ)() noexcept {
			return 2147483638;
		}

		[[nodiscard]] static constexpr std::int32_t(missingR)() noexcept {
			return 2147483639;
		}

		[[nodiscard]] static constexpr std::int32_t(missingS)() noexcept {
			return 2147483640;
		}

		[[nodiscard]] static constexpr std::int32_t(missingT)() noexcept {
			return 2147483641;
		}

		[[nodiscard]] static constexpr std::int32_t(missingU)() noexcept {
			return 2147483642;
		}

		[[nodiscard]] static constexpr std::int32_t(missingV)() noexcept {
			return 2147483643;
		}

		[[nodiscard]] static constexpr std::int32_t(missingW)() noexcept {
			return 2147483644;
		}

		[[nodiscard]] static constexpr std::int32_t(missingX)() noexcept {
			return 2147483645;
		}

		[[nodiscard]] static constexpr std::int32_t(missingY)() noexcept {
			return 2147483646;
		}

		[[nodiscard]] static constexpr std::int32_t(missingZ)() noexcept {
			return 2147483647;
		}
	};

	// Values taken from https://www.stata.com/help.cgi?datatypes
	// These also match https://www.stata.com/help.cgi?dta
	template <>
	class stata_limits_v2<float> : public std::numeric_limits<float> {
	public:
		[[nodiscard]] static constexpr float(lowest)() noexcept {
			return -0x1.fffffp126f;
		}

		[[nodiscard]] static constexpr float(max)() noexcept {
			return 0x1.fffffp126f;
		}

		[[nodiscard]] static constexpr float(missing)() noexcept {
			return 0x1.000p127f;
		}

		[[nodiscard]] static constexpr float(missingA)() noexcept {
			return 0x1.001p127f;
		}

		[[nodiscard]] static constexpr float(missingB)() noexcept {
			return 0x1.002p127f;
		}

		[[nodiscard]] static constexpr float(missingC)() noexcept {
			return 0x1.003p127f;
		}

		[[nodiscard]] static constexpr float(missingD)() noexcept {
			return 0x1.004p127f;
		}

		[[nodiscard]] static constexpr float(missingE)() noexcept {
			return 0x1.005p127f;
		}

		[[nodiscard]] static constexpr float(missingF)() noexcept {
			return 0x1.006p127f;
		}

		[[nodiscard]] static constexpr float(missingG)() noexcept {
			return 0x1.007p127f;
		}

		[[nodiscard]] static constexpr float(missingH)() noexcept {
			return 0x1.008p127f;
		}

		[[nodiscard]] static constexpr float(missingI)() noexcept {
			return 0x1.009p127f;
		}

		[[nodiscard]] static constexpr float(missingJ)() noexcept {
			return 0x1.00ap127f;
		}

		[[nodiscard]] static constexpr float(missingK)() noexcept {
			return 0x1.00bp127f;
		}

		[[nodiscard]] static constexpr float(missingL)() noexcept {
			return 0x1.00cp127f;
		}

		[[nodiscard]] static constexpr float(missingM)() noexcept {
			return 0x1.00dp127f;
		}

		[[nodiscard]] static constexpr float(missingN)() noexcept {
			return 0x1.00ep127f;
		}

		[[nodiscard]] static constexpr float(missingO)() noexcept {
			return 0x1.00fp127f;
		}

		[[nodiscard]] static constexpr float(missingP)() noexcept {
			return 0x1.010p127f;
		}

		[[nodiscard]] static constexpr float(missingQ)() noexcept {
			return 0x1.011p127f;
		}

		[[nodiscard]] static constexpr float(missingR)() noexcept {
			return 0x1.012p127f;
		}

		[[nodiscard]] static constexpr float(missingS)() noexcept {
			return 0x1.013p127f;
		}

		[[nodiscard]] static constexpr float(missingT)() noexcept {
			return 0x1.014p127f;
		}

		[[nodiscard]] static constexpr float(missingU)() noexcept {
			return 0x1.015p127f;
		}

		[[nodiscard]] static constexpr float(missingV)() noexcept {
			return 0x1.016p127f;
		}

		[[nodiscard]] static constexpr float(missingW)() noexcept {
			return 0x1.017p127f;
		}

		[[nodiscard]] static constexpr float(missingX)() noexcept {
			return 0x1.018p127f;
		}

		[[nodiscard]] static constexpr float(missingY)() noexcept {
			return 0x1.019p127f;
		}

		[[nodiscard]] static constexpr float(missingZ)() noexcept {
			return 0x1.01ap127f;
		}
	};

	// Values taken from https://www.stata.com/help.cgi?datatypes
	// Value for lowest minumum in https://www.stata.com/help.cgi?dta has a lower value (-0x1.fffffffffffffp1023)
	template <>
	class stata_limits_v2<double> : public std::numeric_limits<double> {
	public:
		[[nodiscard]] static constexpr double(lowest)() noexcept {
			return -0x1.fffffffffffffp1022;
		}

		[[nodiscard]] static constexpr double(max)() noexcept {
			return 0x1.fffffffffffffp1022;
		}

		[[nodiscard]] static constexpr double(missing)() noexcept {
			return 0x1.000p1023;
		}

		[[nodiscard]] static constexpr double(missingA)() noexcept {
			return 0x1.001p1023;
		}

		[[nodiscard]] static constexpr double(missingB)() noexcept {
			return 0x1.002p1023;
		}

		[[nodiscard]] static constexpr double(missingC)() noexcept {
			return 0x1.003p1023;
		}

		[[nodiscard]] static constexpr double(missingD)() noexcept {
			return 0x1.004p1023;
		}

		[[nodiscard]] static constexpr double(missingE)() noexcept {
			return 0x1.005p1023;
		}

		[[nodiscard]] static constexpr double(missingF)() noexcept {
			return 0x1.006p1023;
		}

		[[nodiscard]] static constexpr double(missingG)() noexcept {
			return 0x1.007p1023;
		}

		[[nodiscard]] static constexpr double(missingH)() noexcept {
			return 0x1.008p1023;
		}

		[[nodiscard]] static constexpr double(missingI)() noexcept {
			return 0x1.009p1023;
		}

		[[nodiscard]] static constexpr double(missingJ)() noexcept {
			return 0x1.00ap1023;
		}

		[[nodiscard]] static constexpr double(missingK)() noexcept {
			return 0x1.00bp1023;
		}

		[[nodiscard]] static constexpr double(missingL)() noexcept {
			return 0x1.00cp1023;
		}

		[[nodiscard]] static constexpr double(missingM)() noexcept {
			return 0x1.00dp1023;
		}

		[[nodiscard]] static constexpr double(missingN)() noexcept {
			return 0x1.00ep1023;
		}

		[[nodiscard]] static constexpr double(missingO)() noexcept {
			return 0x1.00fp1023;
		}

		[[nodiscard]] static constexpr double(missingP)() noexcept {
			return 0x1.010p1023;
		}

		[[nodiscard]] static constexpr double(missingQ)() noexcept {
			return 0x1.011p1023;
		}

		[[nodiscard]] static constexpr double(missingR)() noexcept {
			return 0x1.012p1023;
		}

		[[nodiscard]] static constexpr double(missingS)() noexcept {
			return 0x1.013p1023;
		}

		[[nodiscard]] static constexpr double(missingT)() noexcept {
			return 0x1.014p1023;
		}

		[[nodiscard]] static constexpr double(missingU)() noexcept {
			return 0x1.015p1023;
		}

		[[nodiscard]] static constexpr double(missingV)() noexcept {
			return 0x1.016p1023;
		}

		[[nodiscard]] static constexpr double(missingW)() noexcept {
			return 0x1.017p1023;
		}

		[[nodiscard]] static constexpr double(missingX)() noexcept {
			return 0x1.018p1023;
		}

		[[nodiscard]] static constexpr double(missingY)() noexcept {
			return 0x1.019p1023;
		}

		[[nodiscard]] static constexpr double(missingZ)() noexcept {
			return 0x1.01ap1023;
		}
	};

	template <typename T>
	inline bool isMissing(const T& value, std::uint8_t version = default_version) {
		if constexpr (std::is_integral_v<T>) {
			if (version < 112) {
				return value < stata_limits_v1<T>::lowest() || value > stata_limits_v1<T>::max();
			}
			else {
				return value < stata_limits_v2<T>::lowest() || value > stata_limits_v2<T>::max();
			}
		}
		if constexpr (std::is_floating_point_v<T>) {
			if (version < 108) {
				return value < stata_limits_v1<T>::lowest() || value > stata_limits_v1<T>::max();
			}
			else {
				return value < stata_limits_v2<T>::lowest() || value > stata_limits_v2<T>::max();
			}
		}
		if constexpr (std::is_same_v<T, std::string>) {
			return value.empty();
		}
		if constexpr (std::is_same_v<T, std::vector<char> >) {
			return value.empty();
		}
	}

	template<typename T>
	struct ColumnSummary {
		T min{ std::numeric_limits<T>::max() };
		T max{ std::numeric_limits<T>::lowest() };
		std::size_t nmissing{ 0 };

		ColumnSummary<T>(const std::vector<T>& column, std::uint8_t version = default_version) {
			for (const auto& value : column) {
				if (isMissing(value, version) == false) {
					min = std::min(min, value);
					max = std::max(max, value);
				}
				else {
					++nmissing;
				}
			}
		}
	};

	template<>
	struct ColumnSummary<std::string> {
		std::size_t min{ std::numeric_limits<std::size_t>::max() };
		std::size_t max{ std::numeric_limits<std::size_t>::min() };
		std::size_t nmissing{ 0 };

		ColumnSummary<std::string>(const std::vector<std::string>& column, std::uint8_t version = default_version) {
			for (const auto& value : column) {
				if (isMissing(value, version) == false) {
					min = std::min(min, value.length());
					max = std::max(max, value.length());
				}
				else {
					++nmissing;
				}
			}
		}
	};

	template<>
	struct ColumnSummary<strL> {
		std::size_t min{ std::numeric_limits<std::size_t>::max() };
		std::size_t max{ std::numeric_limits<std::size_t>::min() };
		std::size_t nmissing{ 0 };

		ColumnSummary<strL>(const std::vector<strL>& column, std::uint8_t version = default_version) {
			for (const auto& value : column) {
				std::visit(overloaded{
					[&](const std::string& str) {
						if (isMissing(str, version) == false) {
							min = std::min(min, str.length());
							max = std::max(max, str.length());
						}
						else {
							++nmissing;
						}
					},
					[&](const std::vector<char>& chars) {
						if (isMissing(chars, version) == false) {
							min = std::min(min, chars.size());
							max = std::max(max, chars.size());
						}
						else {
							++nmissing;
						}
					}
				}, value);
			}
		}
	};

	template<typename T>
	inline void readVector(std::istream& f, std::vector<T>& vals, std::endian order = std::endian::native) {
		if (vals.empty()) {
			return;
		}
		f.read((char*)vals.data(), sizeof(T) * vals.size());

		if (order != std::endian::native) {
			for (auto& val : vals) {
				val = std::byteswap(val);
			}
		}
	}

	template<typename T>
	inline T readValue(std::istream& f, std::endian order = std::endian::native) {
		T val = 0;
		f.read((char*)&val, sizeof(T));

		if (order != std::endian::native) {
			val = std::byteswap(val);
		}
		return val;
	}

	template<>
	inline float readValue(std::istream& f, std::endian order) {
		float val = 0;
		f.read((char*)&val, sizeof(float));

		if (order != std::endian::native) {
			val = std::bit_cast<float>(std::byteswap(std::bit_cast<std::uint32_t>(val)));
		}
		return val;
	}

	template<>
	inline double readValue(std::istream& f, std::endian order) {
		double val = 0;
		f.read((char*)&val, sizeof(double));

		if (order != std::endian::native) {
			val = std::bit_cast<double>(std::byteswap(std::bit_cast<std::uint64_t>(val)));
		}
		return val;
	}

	inline std::string readString(std::istream& f, std::size_t length) {
		std::string str(length + 1, '\0'); // Input string may not be null terminated, so add an extra character for this
		if (length > 0) {
			f.read((char*)str.data(), length);
		}
		str.erase(str.find('\0')); // Trim to null terminator
		return str;
	}

	inline std::vector<char> readCharacters(std::istream& f, std::size_t length) {
		std::vector<char> chars(length, '\0');
		if (length > 0) {
			f.read(chars.data(), length);
		}
		return chars;
	}

	inline std::uint32_t readValue_uint24(std::istream& f, std::endian order = std::endian::native) {
		std::uint32_t result{};
		std::byte* parts = reinterpret_cast<std::byte*>(&result);
		if (order == std::endian::little) {
			f.read((char*)&parts[0], 3);
		}
		if (order == std::endian::big) {
			f.read((char*)&parts[1], 3);
		}
		if (order != std::endian::native) {
			result = std::byteswap(result);
		}
		return result;
	}

	inline std::uint64_t readValue_uint40(std::istream& f, std::endian order = std::endian::native) {
		std::uint64_t result{};
		std::byte* parts = reinterpret_cast<std::byte*>(&result);
		if (order == std::endian::little) {
			f.read((char*)&parts[0], 5);
		}
		if (order == std::endian::big) {
			f.read((char*)&parts[3], 5);
		}
		if (order != std::endian::native) {
			result = std::byteswap(result);
		}
		return result;
	}

	inline std::uint64_t readValue_uint48(std::istream& f, std::endian order = std::endian::native) {
		std::uint64_t result{};
		std::byte* parts = reinterpret_cast<std::byte*>(&result);
		if (order == std::endian::little) {
			f.read((char*)&parts[0], 6);
		}
		if (order == std::endian::big) {
			f.read((char*)&parts[2], 6);
		}
		if (order != std::endian::native) {
			result = std::byteswap(result);
		}
		return result;
	}

	inline std::streamoff readtag(std::istream& f, std::string_view tag) {
		size_t taglen = tag.length();
		std::string tagbuff(taglen, '\0');
		auto pos = f.tellg();
		f.read(tagbuff.data(), taglen);
		if (tag != tagbuff) {
			f.seekg(pos, f.beg);
			return -1;
		}
		return f.tellg();
	}

	template<typename T>
	inline void writeValue(std::ostream& f, T value, std::endian order = std::endian::native) {
		if (order != std::endian::native) {
			value = std::byteswap(value);
		}
		f.write((char*)&value, sizeof(T));
	}

	template<>
	inline void writeValue(std::ostream& f, float value, std::endian order) {
		if (order != std::endian::native) {
			value = std::bit_cast<float>(std::byteswap(std::bit_cast<std::uint32_t>(value)));
		}
		f.write((char*)&value, sizeof(float));
	}

	template<>
	inline void writeValue(std::ostream& f, double value, std::endian order) {
		if (order != std::endian::native) {
			value = std::bit_cast<double>(std::byteswap(std::bit_cast<std::uint64_t>(value)));
		}
		f.write((char*)&value, sizeof(double));
	}

	inline void writeString(std::ostream& f, std::string_view s, std::size_t width = std::string::npos) {
		// NOTE: the string written by this function this does not necessarily include a null terminator
		if (width == std::string::npos) {
			f.write(s.data(), s.length());
		}
		else {
			std::string out(width, '\0');
			// Ensure maximum length is not exceeded
			std::format_to(out.data(), "{:.{}s}", s, width);
			f.write(out.data(), out.length());
			if (s.length() > width) {
				std::println(std::cerr, "Warning: String \"{:s}\" was truncated to \"{:s}\" when written", s, out);
			}
		}
	}

	inline void writeStringZ(std::ostream& f, const std::string &s, std::size_t width = std::string::npos) {
		// NOTE: the string written by this function includes a null terminator
		if (width == std::string::npos) {
			f.write(s.c_str(), s.length() + 1);
		}
		else {
			--width; // Make space for trailing null
			std::string out(width, '\0');
			// Ensure maximum length is not exceeded
			std::format_to(out.data(), "{:.{}s}", s, width);
			f.write(out.c_str(), out.length() + 1);
			if (s.length() > width) {
				std::println(std::cerr, "Warning: String \"{:s}\" was truncated to \"{:s}\" when written", s, out);
			}
		}
	}

	inline void writeCharacters(std::ostream& f, const std::vector<char> &chars) {
		f.write(chars.data(), chars.size());
	}

	inline void writeValue_uint24(std::ostream& f, std::uint32_t value, std::endian order = std::endian::native) {
		if (order != std::endian::native) {
			value = std::byteswap(value);
		}
		std::byte* parts = reinterpret_cast<std::byte*>(&value);
		if (order == std::endian::big) {
			f.write((char*)&parts[1], 3);

		}
		if (order == std::endian::little) {
			f.write((char*)&parts[0], 3);
		}
	}

	inline void writeValue_uint40(std::ostream& f, std::uint64_t value, std::endian order = std::endian::native) {
		if (order != std::endian::native) {
			value = std::byteswap(value);
		}
		std::byte* parts = reinterpret_cast<std::byte*>(&value);
		if (order == std::endian::big) {
			f.write((char*)&parts[3], 5);
		}
		if (order == std::endian::little) {
			f.write((char*)&parts[0], 5);
		}
	}

	inline void writeValue_uint48(std::ostream& f, std::uint64_t value, std::endian order = std::endian::native) {
		if (order != std::endian::native) {
			value = std::byteswap(value);
		}
		std::byte* parts = reinterpret_cast<std::byte*>(&value);

		if (order == std::endian::big) {
			f.write((char*)&parts[2], 6);
		}
		if (order == std::endian::little) {
			f.write((char*)&parts[0], 6);
		}
	}

	inline void writetag(std::ostream& f, std::string_view tag) {
		f.write((char*)tag.data(), tag.length());
	}

	inline namespace v1 {

	class DTAFile {
	public:
		DTAFile() = default;
		DTAFile(std::uint8_t version) : fileversion(version) {}
		DTAFile(std::string_view filename);

		bool Save(std::string_view filename) const {
			return Save(filename, fileversion, byteorder);
		}
		bool Save(std::string_view filename, std::uint8_t version = default_version, std::endian byteorder = std::endian::native) const;

		[[nodiscard]] std::uint8_t GetOriginalFileVersion() const;
		[[nodiscard]] std::endian GetOriginalByteOrder() const;
		[[nodiscard]] std::string GetDataLabel() const;
		[[nodiscard]] std::string GetTimeStamp() const;
		[[nodiscard]] std::string GetVariableName(std::uint32_t varnumber) const;
		[[nodiscard]] std::string GetVariableLabel(std::string_view varname) const;
		[[nodiscard]] std::string GetValueLabelName(std::string_view varname) const;
		[[nodiscard]] std::string GetVariableType(std::string_view varname) const;
		[[nodiscard]] std::string GetFormat(std::string_view varname) const;
		[[nodiscard]] std::vector<std::string> GetValueLabelNames() const;
		[[nodiscard]] std::vector<std::int32_t> GetValueLabelCodes(std::string labname) const;
		[[nodiscard]] std::string GetValueLabelString(std::string labname, std::int32_t value) const;
		[[nodiscard]] std::uint64_t GetNumObservations() const;
		[[nodiscard]] std::uint32_t GetNumVariables() const;
		[[nodiscard]] std::vector<std::string> GetSortOrder() const;
		[[nodiscard]] std::vector<std::string> GetCharacteristicNames(std::string_view varname) const;
		[[nodiscard]] std::string GetCharacteristic(std::string_view varname, std::string_view charname) const;
		[[nodiscard]] ColumnData GetVariable(std::string_view varname) const;
		[[nodiscard]] std::size_t GetMaxStringLength(std::string_view varname) const;
		[[nodiscard]] std::vector<CellData> GetObservation(std::uint64_t row) const;
		[[nodiscard]] CellData GetValue(std::string_view varname, std::uint64_t row) const;
		template <typename T>
		[[nodiscard]] T GetValue(std::string_view varname, std::uint64_t row) const;
		[[nodiscard]] std::string GetValueString(std::string_view varname, std::uint64_t row) const;

		bool AddVariable(std::string_view varname, std::string_view type);
		bool AddValueLabel(std::string labname, std::int32_t value, std::string label);
		bool AddCharacteristic(std::string_view varname, std::string_view charname, std::string characteristic);
		void StampTime();
		bool SetDataLabel(std::string_view label);
		bool SetVariableLabel(std::string_view varname, std::string_view label);
		bool SetFormat(std::string_view varname, std::string_view format);
		bool SetObservations(std::uint64_t obs);
		bool SetSortOrder(const std::vector<std::string_view> &variablelist);
		bool AssignValueLabel(std::string_view varname, std::string_view labname);
		template <typename T>
		bool SetValue(std::string_view varname, std::uint64_t row, T value);

		void SortData(const std::vector<std::string_view> &keys);

		bool RenameVariable(std::string_view oldname, std::string_view newname);
		bool RenameValueLabel(std::string oldname, std::string newname);
		bool RemoveVariable(std::string_view varname);
		bool RemoveValueLabel(std::string labname, std::int32_t value);
		bool RemoveCharacteristic(std::string_view varname, std::string_view charname);

	private:
		std::uint8_t fileversion = default_version;
		std::endian byteorder = std::endian::native;

		std::uint32_t nvar{};
		std::uint64_t nobs{};
		std::string data_label{};
		std::string time_stamp{};

		std::vector<std::string> varlist{};
		std::vector<std::string> lbllist{};
		std::vector<std::string> fmtlist{};
		std::vector<std::string> varlabs{};

		std::vector<std::uint32_t> srtlist{ 0 };
		std::vector<std::size_t> strlens{};

		std::vector<ColumnData> datacolumns{};

		std::map<std::string, std::vector<std::pair<std::int32_t, std::string> > > label_table;

		std::vector<std::tuple<std::string, std::string, std::string> > characteristics;

		void readStata(std::istream& f);
		void readStataxml(std::istream& f);
		bool writeStata(std::ostream& f, std::uint8_t version, std::endian byteorder) const;
		bool writeStataxml(std::ostream& f, std::uint8_t version, std::endian byteorder) const;
		bool changeVersion(std::uint8_t version = default_version);
		std::uint32_t variableIndex(std::string_view varname) const;

	};

	inline DTAFile::DTAFile(std::string_view filename) {
		std::ifstream f(std::filesystem::path(filename), std::ifstream::binary);
		if (f.good() == false) {
			std::println(std::cerr, "Invalid file");
			return;
		}
		readStata(f);
	}

	inline void DTAFile::readStataxml(std::istream& f) {
		if (readtag(f, "<stata_dta>") == -1) {
			std::println(std::clog, "Warning: Data not in expected position");
		}
		if (readtag(f, "<header>") == -1) {
			std::println(std::clog, "Warning: Header not in expected position");
		}
		if (readtag(f, "<release>") == -1) {
			std::println(std::clog, "Warning: Version information not in expected position");
		}
		std::string release = readString(f, 3);
		if (readtag(f, "</release>") == -1) {
			std::println(std::clog, "Warning: Version information not expected length");
		}

		fileversion = std::stoi(release);

		dtainformation fileinfo = getVersionInformation(fileversion);
		if (fileinfo.varname_width == 0 || fileinfo.varlab_width == 0 || fileinfo.format_width == 0) {
			std::println(std::cerr, "Unrecognised Stata format");
			return;
		}

		if (readtag(f, "<byteorder>") == -1) {
			std::println(std::clog, "Warning: Byte order information not in expected position");
		}
		std::string order = readString(f, 3);
		if (readtag(f, "</byteorder>") == -1) {
			std::println(std::clog, "Warning: Byte order information not expected length");
		}

		byteorder = std::endian::native;
		if (order == "MSF") {
			byteorder = std::endian::big;
		}
		if (order == "LSF") {
			byteorder = std::endian::little;
		}

		if (readtag(f, "<K>") == -1) {
			std::println(std::clog, "Warning: Variable count information not in expected position");
		}
		nvar = (fileversion != 119 && fileversion != 121) ? readValue<std::uint16_t>(f, byteorder) : readValue<std::uint32_t>(f, byteorder);
		if (readtag(f, "</K>") == -1) {
			std::println(std::clog, "Warning: Variable count information not expected length");
		}

		if (readtag(f, "<N>") == -1) {
			std::println(std::clog, "Warning: Observation count information not in expected position");
		}
		nobs = (fileversion == 117) ? readValue<std::uint32_t>(f, byteorder) : readValue<std::uint64_t>(f, byteorder);
		if (readtag(f, "</N>") == -1) {
			std::println(std::clog, "Warning: Observation count information not expected length");
		}

		if (readtag(f, "<label>") == -1) {
			std::println(std::clog, "Warning: Data label not in expected position");
		}
		std::uint16_t labellen = (fileversion == 117) ? readValue<std::uint8_t>(f, byteorder) : readValue<std::uint16_t>(f, byteorder);

		data_label = readString(f, labellen);
		if (readtag(f, "</label>") == -1) {
			std::println(std::clog, "Warning: Data label not expected length");
		}
		if (readtag(f, "<timestamp>") == -1) {
			std::println(std::clog, "Warning: Timestamp not in expected position");
		}
		std::uint8_t timelen = readValue<std::uint8_t>(f, byteorder);

		time_stamp = readString(f, timelen);

		if (readtag(f, "</timestamp>") == -1) {
			std::println(std::clog, "Warning: Timestamp not expected length");
		}
		if (readtag(f, "</header>") == -1) {
			std::println(std::clog, "Warning: Header not expected length");
		}

		mapdef map{};

		if (readtag(f, "<map>") == -1) {
			std::println(std::clog, "Warning: Map not in expected position");
		};
		map.hdr = readValue<std::int64_t>(f, byteorder);
		map.map = readValue<std::int64_t>(f, byteorder);
		map.types = readValue<std::int64_t>(f, byteorder);
		map.varnames = readValue<std::int64_t>(f, byteorder);
		map.srtlist = readValue<std::int64_t>(f, byteorder);
		map.fmts = readValue<std::int64_t>(f, byteorder);
		map.vlblnames = readValue<std::int64_t>(f, byteorder);
		map.varlabs = readValue<std::int64_t>(f, byteorder);
		map.chars = readValue<std::int64_t>(f, byteorder);
		map.data = readValue<std::int64_t>(f, byteorder);
		map.strls = readValue<std::int64_t>(f, byteorder);
		map.vallabs = readValue<std::int64_t>(f, byteorder);
		map.tlr = readValue<std::int64_t>(f, byteorder);
		map.bot = readValue<std::int64_t>(f, byteorder);

		if (readtag(f, "</map>") == -1) {
			std::println(std::clog, "Warning: Map not expected length");
		}

		std::vector<std::uint16_t> typlist(nvar);
		if (map.types != f.tellg()) {
			std::println(std::clog, "Warning: Type list not at expected offset");
		}
		if (readtag(f, "<variable_types>") == -1) {
			f.seekg(map.types, f.beg);
			readtag(f, "<variable_types>");
		}
		readVector(f, typlist, byteorder);
		if (readtag(f, "</variable_types>") == -1) {
			std::println(std::clog, "Warning: Type list not expected length");
		}

		if (map.varnames != f.tellg()) {
			std::println(std::clog, "Warning: Variable names not at expected offset");
		}
		if (readtag(f, "<varnames>") == -1) {
			f.seekg(map.varnames, f.beg);
			readtag(f, "<varnames>");
		}
		std::vector<std::string>(nvar).swap(varlist);
		for (std::uint32_t i = 0; i < nvar; ++i) {
			varlist[i] = readString(f, fileinfo.varname_width);
		}
		if (readtag(f, "</varnames>") == -1) {
			std::println(std::clog, "Warning: Variable names not expected length");
		}

		if (map.srtlist != f.tellg()) {
			std::println(std::clog, "Warning: Sort list not at expected offset");
		}
		if (readtag(f, "<sortlist>") == -1) {
			f.seekg(map.srtlist, f.beg);
		}
		std::vector<std::uint32_t>(nvar + 1).swap(srtlist);
		if (fileversion != 119 && fileversion != 121) {
			for (std::uint32_t i = 0; i < nvar + 1; ++i) {
				srtlist[i] = readValue<std::uint16_t>(f, byteorder);
			}
		}
		else {
			readVector(f, srtlist, byteorder);
		}

		if (readtag(f, "</sortlist>") == -1) {
			std::println(std::clog, "Warning: Sort list not expected length");
		}

		if (map.fmts != f.tellg()) {
			std::println(std::clog, "Warning: Formats not at expected offset");
		}
		if (readtag(f, "<formats>") == -1) {
			f.seekg(map.fmts, f.beg);
		}
		std::vector<std::string>(nvar).swap(fmtlist);
		for (std::uint32_t i = 0; i < nvar; ++i) {
			fmtlist[i] = readString(f, fileinfo.format_width);
		}
		if (readtag(f, "</formats>") == -1) {
			std::println(std::clog, "Warning: Formats not expected length");
		}

		if (map.vlblnames != f.tellg()) {
			std::println(std::clog, "Warning: Variable label names not at expected offset");
		}

		if (readtag(f, "<value_label_names>") == -1) {
			f.seekg(map.vlblnames, f.beg);
		}
		std::vector<std::string>(nvar).swap(lbllist);
		for (std::uint32_t i = 0; i < nvar; ++i) {
			lbllist[i] = readString(f, fileinfo.varname_width);
		}
		if (readtag(f, "</value_label_names>") == -1) {
			std::println(std::clog, "Warning: Variable label names not expected length");
		}

		if (map.varlabs != f.tellg()) {
			std::println(std::clog, "Warning: Variable labels not at expected offset");
		}
		if (readtag(f, "<variable_labels>") == -1) {
			f.seekg(map.varlabs, f.beg);
		}
		std::vector<std::string>(nvar).swap(varlabs);
		for (std::uint32_t i = 0; i < nvar; ++i) {
			varlabs[i] = readString(f, fileinfo.varlab_width);
		}
		if (readtag(f, "</variable_labels>") == -1) {
			std::println(std::clog, "Warning: Variable labels not expected length");
		}

		if (map.chars != f.tellg()) {
			std::println(std::clog, "Warning: Characteristics not at expected offset");
		}
		if (readtag(f, "<characteristics>") == -1) {
			f.seekg(map.chars, f.beg);
		}
		while (readtag(f, "<ch>") != -1) {
			std::uint32_t charlen = readValue<std::uint32_t>(f, byteorder);

			std::string varname = readString(f, fileinfo.varname_width);
			charlen -= fileinfo.varname_width;
			std::string charname = readString(f, fileinfo.varname_width);
			charlen -= fileinfo.varname_width;
			std::string contents = readString(f, charlen);
			characteristics.push_back({ varname, charname, contents });

			if (readtag(f, "</ch>") == -1) {
				std::println(std::clog, "Warning: end </ch> not found");
			}
		}

		if (readtag(f, "</characteristics>") == -1) {
			std::println(std::clog, "Warning: Characteristics not expected length");
		}

		std::vector<size_t>(nvar).swap(strlens);
		std::vector<ColumnData>(nvar).swap(datacolumns);
		for (std::uint32_t i = 0; i < nvar; ++i) {
			switch (typlist[i]) {
			case 65530: // byte
				datacolumns[i] = std::vector<std::int8_t>(nobs);
				break;
			case 65529: // int16
				datacolumns[i] = std::vector<std::int16_t>(nobs);
				break;
			case 65528: // int32
				datacolumns[i] = std::vector<std::int32_t>(nobs);
				break;
			case 65527: // float
				datacolumns[i] = std::vector<float>(nobs);
				break;
			case 65526: // double
				datacolumns[i] = std::vector<double>(nobs);
				break;
			case 65525:  // alias
				break;
			case 32768: // strL
				datacolumns[i] = std::vector<strL>(nobs);
				break;
			default: // str{1..2045}
				datacolumns[i] = std::vector<std::string>(nobs);
				if (typlist[i] > 2045) {
					std::println(std::cerr, "Invalid string variable length");
					return;
				}
				strlens[i] = typlist[i];
			}
		}

		std::streamoff strlspos = map.strls;

		if (map.data != f.tellg()) {
			std::println(std::clog, "Warning: Data not at expected offset");
		}
		if (readtag(f, "<data>") == -1) {
			f.seekg(map.data, f.beg);
		}
		auto ticktime = std::chrono::system_clock::now();
		for (std::uint64_t i = 0; i < nobs; i++) {
			std::chrono::duration<double> difftime = std::chrono::system_clock::now() - ticktime;
			if (difftime.count() > 1) {
				ticktime = std::chrono::system_clock::now();
				std::println(std::clog, "Reading record {:d} of {:d}", i, nobs);
			}
			for (std::uint32_t j = 0; j < nvar; j++) {
				std::size_t slen = strlens[j];
				auto& column = datacolumns[j];
				std::visit(overloaded{
					[](std::monostate) {}, // alias
					[&](std::vector<std::int8_t>& data) {
						data[i] = readValue<std::int8_t>(f, byteorder);
					},
					[&](std::vector<std::int16_t>& data) {
						data[i] = readValue<std::int16_t>(f, byteorder);
					},
					[&](std::vector<std::int32_t>& data) {
						data[i] = readValue<std::int32_t>(f, byteorder);
					},
					[&](std::vector<float>& data) {
						data[i] = readValue<float>(f, byteorder);
					},
					[&](std::vector<double>& data) {
						data[i] = readValue<double>(f, byteorder);
					},
					[&](std::vector<std::string>& data) {
						data[i] = readString(f, slen);
					},
					[&](std::vector<strL>& data) {
						std::uint32_t v = 0;
						std::uint64_t o = 0;
						if (fileversion == 117) {
							v = readValue<std::uint32_t>(f, byteorder);
							o = readValue<std::uint32_t>(f, byteorder);
						}
						else if (fileversion == 118 || fileversion == 120) {
							v = readValue<std::uint16_t>(f, byteorder);
							o = readValue_uint48(f, byteorder);
						}
						else if (fileversion == 119 || fileversion == 121) {
							v = readValue_uint24(f, byteorder);
							o = readValue_uint40(f, byteorder);
						}
						if (v == (j + 1) && o == (i + 1)) { // Direct reference
							std::streamoff dtapos = f.tellg();
							f.seekg(strlspos, f.beg);
							if (strlspos == map.strls && readtag(f, "<strls>") == -1) {
								std::println(std::clog, "Warning: Long strings not at expected offset");
							}
							std::string GSO = readString(f, 3);
							if (GSO != "GSO") {
								std::println(std::clog, "Warning: Unexpected GSO value found");
							}
							std::uint32_t GSO_v = readValue<std::uint32_t>(f, byteorder);
							if (GSO_v != v) {
								std::println(std::clog, "Warning expected GSO_v found");
							}
							std::uint64_t GSO_o = (fileversion == 117) ? readValue<std::uint32_t>(f, byteorder) : readValue<std::uint64_t>(f, byteorder);
							if (GSO_o != o) {
								std::println(std::clog, "Warning expected GSO_v found");
							}
							std::uint8_t t = readValue<std::uint8_t>(f, byteorder);
							std::uint32_t len = readValue<std::uint32_t>(f, byteorder);
							if (t == 129) { // binary
								data[i] = readCharacters(f, len);
							}
							else if (t == 130) { // null terminated string
								data[i] = readString(f, len);
							}
							else {
								std::println(std::clog, "Unexpected strL type");
							}
							strlspos = f.tellg();
							f.seekg(dtapos, f.beg);
						}
						else if (v == 0 && o == 0) { // Empty string
						}
						else if (o < (i + 1) && v == (j + 1)) { // Back reference to same column
							std::get<std::vector<strL> >(datacolumns[j])[i] = std::get<std::vector<strL> >(datacolumns[std::size_t(v) - 1])[o - i];
						}
						else if (v < j) { // Back reference to different column
							std::get<std::vector<strL> >(datacolumns[j])[i] = std::get<std::vector<strL> >(datacolumns[std::size_t(v) - 1])[o - i];
						}
						else {
							std::println(std::clog, "Warning: Forward reference");
						}
					}
				}, column);
			}
		}

		if (readtag(f, "</data>") == -1) {
			std::println(std::clog, "Warning: Data not expected length");
		}

		if (map.strls != f.tellg()) {
			std::println(std::clog, "Warning: Long strings not at expected offset");
		}
		if (readtag(f, "<strls>") == -1) {
			f.seekg(map.strls, f.beg);
		}
		auto gsopos = f.tellg();
		f.seekg(map.vallabs, f.beg);
		if (readtag(f, "<value_labels>") == -1) {
			// Value labels not where the maps says, so try to parse up to them instead
			std::println(std::clog, "Warning: Value labels not at expected offset");
			f.seekg(gsopos, f.beg);
			while (readtag(f, "GSO") != -1) {
				readValue<std::uint32_t>(f, byteorder);
				if (fileversion == 117) {
					readValue<std::uint32_t>(f, byteorder);
				}
				else {
					readValue<std::uint64_t>(f, byteorder);
				}
				std::uint8_t type = readValue<std::uint8_t>(f, byteorder);
				std::uint32_t len = readValue<std::uint32_t>(f, byteorder);
				if (type == 129) {
					strL contents = readCharacters(f, len);
				}
				else if (type == 130) {
					strL contents = readString(f, len);
				}
			}
			if (readtag(f, "</strls>") == -1) {
				std::println(std::clog, "Warning: Long strings not expected length");
			}
			if (readtag(f, "<value_labels>") == -1) {
				std::println(std::clog, "Unable to find value labels");
				return;
			}
		}
		while (readtag(f, "<lbl>") != -1) {
			std::uint32_t valtablen = readValue<std::uint32_t>(f, byteorder);
			std::string labname = readString(f, fileinfo.varname_width);
			std::string pad = readString(f, 3);

			if (valtablen > 0) {
				std::uint32_t nument = readValue<std::uint32_t>(f, byteorder);
				std::uint32_t txtlen = readValue<std::uint32_t>(f, byteorder);

				std::vector<std::uint32_t> offs(nument, 0);
				std::vector<std::int32_t> vals(nument, 0);

				if (nument > 0) {
					readVector(f, offs, byteorder);
					readVector(f, vals, byteorder);
				}

				std::vector<char> txt = readCharacters(f, txtlen);

				std::vector<std::pair<std::int32_t, std::string> > table(nument);

				for (std::uint32_t z = 0; z < nument; z++) {
					std::string_view label = txt.data() + offs[z];
					table[z] = { vals[z], std::string(label) };
				}
				label_table[labname] = table;
			}

			if (readtag(f, "</lbl>") == -1) {
				std::println(std::clog, "Warning: Label is not expected length");
			}
		}
		if (readtag(f, "</value_labels>") == -1) {
			std::println(std::clog, "Warning: Value labels not expected length");
		}
		if (readtag(f, "</stata_dta>") == -1) {
			std::println(std::clog, "Warning: Data file is not expected length");
		}
	}

	inline void DTAFile::readStata(std::istream& f) {
		fileversion = readValue<std::uint8_t>(f); // format version
		if (fileversion == 60) { // '<' indicates new style file format
			f.seekg(0, f.beg);
			readStataxml(f);
			return;
		}
		std::uint8_t order = readValue<std::uint8_t>(f);
		if (fileversion == 102) {
			byteorder = std::endian::little;
		}
		else {
			if (order == 2) {
				byteorder = std::endian::little;
			}
			else if (order == 1) {
				byteorder = std::endian::big;
			}
			else {
				std::println(std::cerr, "Unrecognised byte ordering");
				return;
			}
		}
		std::uint8_t filetype = readValue<std::uint8_t>(f, byteorder); // file type
		if (filetype != 1) {
			std::println(std::cerr, "Unhandled file type");
			return;
		}
		readValue<std::uint8_t>(f, byteorder); // reserved

		dtainformation fileinfo = getVersionInformation(fileversion);
		if (fileinfo.varname_width == 0 || fileinfo.varlab_width == 0 || fileinfo.format_width == 0) {
			std::println(std::cerr, "Unrecognised Stata format");
			return;
		}

		nvar = readValue<std::uint16_t>(f, byteorder);
		nobs = (fileversion == 102) ? readValue<std::uint16_t>(f, byteorder) : readValue<std::uint32_t>(f, byteorder);

		data_label = readString(f, fileinfo.varlab_width);
		time_stamp = (fileversion > 104) ? readString(f, 18) : "";
		// Warn if the time_stamp field does not match the length in the epecification.
		// Do not stop though, as this is the case for some of the Stata provided example data.
		if (time_stamp.empty() == false && time_stamp.length() != 17) {
			std::println(std::cerr, "Time stamp: {:s} is not the expected length ({:d} instead of 17)", time_stamp, time_stamp.length());
		}

		std::vector<std::uint8_t> typlist(nvar, 0);
		std::vector<std::string>(nvar).swap(varlist);
		std::vector<std::uint32_t>(nvar + 1).swap(srtlist);
		std::vector<std::string>(nvar).swap(fmtlist);
		std::vector<std::string>(nvar).swap(lbllist);
		std::vector<std::string>(nvar).swap(varlabs);

		readVector(f, typlist, byteorder);
		for (std::uint16_t i = 0; i < nvar; ++i) {
			varlist[i] = readString(f, fileinfo.varname_width);
		}
		for (std::uint32_t i = 0; i < nvar + 1; ++i) {
			srtlist[i] = readValue<std::uint16_t>(f, byteorder);
		}
		for (std::uint16_t i = 0; i < nvar; ++i) {
			fmtlist[i] = readString(f, fileinfo.format_width);
		}
		for (std::uint16_t i = 0; i < nvar; ++i) {
			lbllist[i] = readString(f, fileinfo.varname_width);
		}
		for (std::uint16_t i = 0; i < nvar; ++i) {
			varlabs[i] = readString(f, fileinfo.varlab_width);
		}

		std::vector<size_t>(nvar).swap(strlens);
		std::vector<ColumnData>(nvar).swap(datacolumns);
		for (size_t i = 0; i < nvar; ++i) {
			if (fileversion >= 111 && fileversion != 112) {
				switch (typlist[i]) {
				case 251: // byte
					datacolumns[i] = std::vector<std::int8_t>(nobs);
					break;
				case 252: // int16
					datacolumns[i] = std::vector<std::int16_t>(nobs);
					break;
				case 253: // int32
					datacolumns[i] = std::vector<std::int32_t>(nobs);
					break;
				case 254: // float
					datacolumns[i] = std::vector<float>(nobs);
					break;
				case 255: // double
					datacolumns[i] = std::vector<double>(nobs);
					break;
				default: // string
					datacolumns[i] = std::vector<std::string>(nobs);
					if (typlist[i] > 244) {
						std::println(std::cerr, "Invalid string variable length");
						return;
					}
					strlens[i] = typlist[i];
				}
			}
			else {
				switch (typlist[i]) {
				case 'b': // byte
					datacolumns[i] = std::vector<std::int8_t>(nobs);
					break;
				case 'i': // int16
					datacolumns[i] = std::vector<std::int16_t>(nobs);
					break;
				case 'l': // int32
					datacolumns[i] = std::vector<std::int32_t>(nobs);
					break;
				case 'f':  // float
					datacolumns[i] = std::vector<float>(nobs);
					break;
				case 'd':
					datacolumns[i] = std::vector<double>(nobs);
					break;
				default:  // string
					datacolumns[i] = std::vector<std::string>(nobs);
					if (typlist[i] - 0x7f > 128) {
						std::println(std::cerr, "Invalid string variable length");
						return;
					}
					strlens[i] = typlist[i] - 0x7f;
				}
			}
		}

		if (fileversion > 104) {
			std::uint8_t data_type{};
			std::uint32_t len{};
			do {
				data_type = readValue<std::uint8_t>(f, byteorder);
				len = (fileversion >= 110) ? readValue<std::uint32_t>(f, byteorder) : readValue<std::uint16_t>(f, byteorder);
				if (data_type == 1) { // characteristic
					std::string varname = readString(f, fileinfo.varname_width);
					len -= fileinfo.varname_width;
					std::string charname = readString(f, fileinfo.varname_width);
					len -= fileinfo.varname_width;
					std::string contents = readString(f, len);
					characteristics.push_back({ varname, charname, contents });
				}
				else { // unknown expansion field type - skip
					readCharacters(f, len);
				}
			} while (data_type != 0 || len != 0);
		}

		auto ticktime = std::chrono::system_clock::now();
		for (std::uint32_t i = 0; i < nobs; i++) {
			std::chrono::duration<double> difftime = std::chrono::system_clock::now() - ticktime;
			if (difftime.count() > 1) {
				ticktime = std::chrono::system_clock::now();
				std::println(std::clog, "Reading record {:d} of {:d}", i, nobs);
			}
			for (std::uint16_t j = 0; j < nvar; j++) {
				std::size_t slen = strlens[j];
				auto& column = datacolumns[j];
				std::visit(overloaded{
					[](std::monostate) { },
					[&](std::vector<std::int8_t>& data) {
						data[i] = readValue<std::int8_t>(f, byteorder);
					},
					[&](std::vector<std::int16_t>& data) {
						data[i] = readValue<std::int16_t>(f, byteorder);
					},
					[&](std::vector<std::int32_t>& data) {
						data[i] = readValue<std::int32_t>(f, byteorder);
					},
					[&](std::vector<float>& data) {
						data[i] = readValue<float>(f, byteorder);
					},
					[&](std::vector<double>& data) {
						data[i] = readValue<double>(f, byteorder);
					},
					[&](std::vector<std::string>& data) {
						data[i] = readString(f, slen);
					},
					[](std::vector<strL>& data) { }
				}, column);
			}
		}

		while (!f.eof()) {
			if (fileversion < 108) {
				std::uint16_t entries = readValue<std::uint16_t>(f, byteorder);

				if (f.eof())
					break;
				std::string labname = readString(f, fileinfo.varname_width);

				readString(f, 1); // padding
				std::vector<std::uint16_t> codes(entries);
				readVector(f, codes, byteorder);
				std::vector<std::pair<std::int32_t, std::string> > table(entries);

				for (std::uint16_t i = 0; i < entries; ++i) {
					std::string vallab = readString(f, 8);
					table[i] = { codes[i], vallab };
				}
				label_table[labname] = table;
			}
			else {
				std::uint32_t valtablen = readValue<std::uint32_t>(f, byteorder);
				if (f.eof())
					break;
				std::string labname = readString(f, fileinfo.varname_width);

				readString(f, 3); // padding

				if (valtablen > 0) {
					std::uint32_t nument = readValue<std::uint32_t>(f, byteorder);
					std::uint32_t txtlen = readValue<std::uint32_t>(f, byteorder);

					std::vector<std::int32_t> offs(nument, 0);
					std::vector<std::int32_t> vals(nument, 0);
					if (nument > 0) {
						readVector(f, offs, byteorder);
						readVector(f, vals, byteorder);
					}

					std::vector<char> txt = readCharacters(f, txtlen);
					std::vector<std::pair<std::int32_t, std::string> > table(nument);

					for (std::uint32_t z = 0; z < nument; z++) {
						std::string_view label = txt.data() + offs[z];
						table[z] = { vals[z], std::string(label) };
					}
					label_table[labname] = table;
				}
			}
		}
	}

	inline bool DTAFile::writeStataxml(std::ostream& f, std::uint8_t version, std::endian byteorder) const {
		dtainformation fileinfo = getVersionInformation(version);

		if (fileinfo.varname_width == 0 || fileinfo.varlab_width == 0 || fileinfo.format_width == 0) {
			std::println(std::cerr, "Unrecognised Stata format");
			return false;
		}

		mapdef map{};

		map.hdr = f.tellp();
		writetag(f, "<stata_dta>");
		writetag(f, "<header>");

		writetag(f, "<release>");
		writeString(f, std::to_string(version));
		writetag(f, "</release>");

		writetag(f, "<byteorder>");
		if (byteorder == std::endian::big) {
			writeString(f, "MSF");
		}
		if (byteorder == std::endian::little) {
			writeString(f, "LSF");
		}
		writetag(f, "</byteorder>");

		writetag(f, "<K>");
		if (version != 119 && version != 121) {
			std::uint16_t nv = static_cast<std::uint16_t>(nvar);
			writeValue(f, nv, byteorder);
		}
		else {
			std::uint32_t nv = static_cast<std::uint32_t>(nvar);
			writeValue(f, nv, byteorder);
		}
		writetag(f, "</K>");

		writetag(f, "<N>");
		if (version == 117) {
			std::uint32_t nnobs = static_cast<std::uint32_t>(nobs);
			writeValue(f, nnobs, byteorder);
		}
		else {
			std::uint64_t nnobs = static_cast<std::uint64_t>(nobs);
			writeValue(f, nnobs, byteorder);
		}
		writetag(f, "</N>");

		writetag(f, "<label>");
		// NOTE: no trailing '\0'
		if (version == 117) {
			std::uint8_t labellen = std::min(static_cast<std::uint8_t>(fileinfo.varlab_width - 1), static_cast<std::uint8_t>(data_label.length()));
			writeValue(f, labellen, byteorder);
			if (labellen > 0) {
				writeString(f, data_label);
			}
		}
		else {
			std::uint16_t labellen = std::min(std::uint16_t(fileinfo.varlab_width - 1), static_cast<std::uint16_t>(data_label.length()));
			writeValue(f, labellen, byteorder);
			if (labellen > 0) {
				writeString(f, data_label);
			}
		}
		writetag(f, "</label>");

		writetag(f, "<timestamp>");

		std::uint8_t timelen = static_cast<std::uint8_t>(time_stamp.length());
		if (timelen != 0 && timelen != 17) {
			std::println(std::cerr, "Invalid time stamp length ({:d})", timelen);
			timelen = 0;
		}
		writeValue(f, timelen, byteorder);
		if (timelen > 0) {
			writeString(f, time_stamp);
		}
		writetag(f, "</timestamp>");

		writetag(f, "</header>");

		map.map = f.tellp();
		writetag(f, "<map>");
		f.write((char*)&map, sizeof(mapdef)); // Doesn't take byteorder into account, but we are going to re-write the map later on with this adjustment
		writetag(f, "</map>");

		map.types = f.tellp();
		writetag(f, "<variable_types>");
		for (std::uint32_t i = 0; i < nvar; ++i) {
			const ColumnData& column = datacolumns[i];
			const std::size_t& slen = strlens[i];
			std::uint16_t vartype = std::visit(overloaded{
				[version](std::monostate) -> std::uint16_t {
					if (version == 120 || version == 121) {
						return 65525;
					}
					else {
						std::println(std::cerr, "Chosen format does not alias types");
						return 0;
					}
				},
				[](const std::vector<std::int8_t>& data) -> std::uint16_t {
					return 65530;
				},
				[](const std::vector<std::int16_t>& data) -> std::uint16_t {
					return 65529;
				},
				[](const std::vector<std::int32_t>& data) -> std::uint16_t {
					return 65528;
				},
				[](const std::vector<float>& data) -> std::uint16_t {
					return 65527;
				},
				[](const std::vector<double>& data) -> std::uint16_t {
					return 65526;
				},
				[slen](const std::vector<std::string>& data) -> std::uint16_t {
					return static_cast<std::uint16_t>(slen);
				},
				[](const std::vector<strL>& data) -> std::uint16_t {
					return 32768;
				},
			}, column);

			if (vartype == 0) {
				return false;
			}
			writeValue(f, vartype, byteorder);
		}
		writetag(f, "</variable_types>");

		map.varnames = f.tellp();
		writetag(f, "<varnames>");
		for (std::uint32_t i = 0; i < nvar; ++i) {
			writeStringZ(f, varlist[i], fileinfo.varname_width);
		}
		writetag(f, "</varnames>");

		map.srtlist = f.tellp();
		writetag(f, "<sortlist>");
		if (version != 119 && version != 121) {
			for (std::uint32_t i = 0; i < nvar + 1; ++i) {
				std::uint16_t srt = srtlist[i];
				writeValue(f, srt, byteorder);
			}
		}
		else {
			for (std::uint32_t i = 0; i < nvar + 1; ++i) {
				std::uint32_t srt = srtlist[i];
				writeValue(f, srt, byteorder);
			}
		}
		writetag(f, "</sortlist>");

		map.fmts = f.tellp();
		writetag(f, "<formats>");
		for (std::uint32_t i = 0; i < nvar; ++i) {
			writeStringZ(f, fmtlist[i], fileinfo.format_width);
		}
		writetag(f, "</formats>");

		map.vlblnames = f.tellp();
		writetag(f, "<value_label_names>");
		for (std::uint32_t i = 0; i < nvar; ++i) {
			writeStringZ(f, lbllist[i], fileinfo.varname_width);
		}
		writetag(f, "</value_label_names>");

		map.varlabs = f.tellp();
		writetag(f, "<variable_labels>");
		for (std::uint32_t i = 0; i < nvar; ++i) {
			writeStringZ(f, varlabs[i], fileinfo.varlab_width);
		}
		writetag(f, "</variable_labels>");

		map.chars = f.tellp();
		writetag(f, "<characteristics>");
		if (characteristics.size() > 0) {
			for (auto& [varname, charname, content] : characteristics) {
				writetag(f, "<ch>");
				std::uint32_t len = 0;
				len += static_cast<std::uint32_t>(fileinfo.varname_width);
				len += static_cast<std::uint32_t>(fileinfo.varname_width);
				len += static_cast<std::uint32_t>(content.length() + 1);
				writeValue(f, len, byteorder);
				writeStringZ(f, varname, fileinfo.varname_width);
				writeStringZ(f, charname, fileinfo.varname_width);
				writeStringZ(f, content);
				writetag(f, "</ch>");
			}
		}
		writetag(f, "</characteristics>");

		map.data = f.tellp();
		writetag(f, "<data>");
		for (std::uint64_t i = 0; i < nobs; i++) {
			for (std::uint32_t j = 0; j < nvar; j++) {
				const ColumnData& column = datacolumns[j];
				const std::size_t& slen = strlens[j];
				std::visit(overloaded{
					[version](std::monostate) {
						if (version != 120 && version != 121) {
							std::println(std::cerr, "Chosen format does not support alias types");
						}
					},
					[&f, i, byteorder](const std::vector<std::int8_t>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, byteorder](const std::vector<std::int16_t>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, byteorder](const std::vector<std::int32_t>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, byteorder](const std::vector<float>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, byteorder](const std::vector<double>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, slen](const std::vector<std::string>& data) {
						writeString(f, data[i], slen);
					},
					[&f, i, j, byteorder, version](const std::vector<strL>& data) {
						bool empty = std::visit(overloaded{
							[](const std::string& str) {
								return str.empty();
							},
							[](const std::vector<char>& chars) {
								return chars.empty();
							},
							[](const auto&) {
								return true;
							}
						}, data[i]);
						if (empty == true) {
							std::uint64_t zero{};
							writeValue(f, zero, byteorder);
						}
						else {
							if (version == 117) { // 4, 4 bytes
								writeValue(f, j + 1, byteorder);
								std::uint32_t obs = static_cast<std::uint32_t>(i + 1);
								writeValue(f, obs, byteorder);
							}
							else if (version == 118 || version == 120) { // 2, 6 bytes
								std::uint16_t var = static_cast<std::uint16_t>(j + 1);
								writeValue(f, var, byteorder);
								writeValue_uint48(f, i + 1, byteorder);
							}
							else if (version == 119 || version == 121) { // 3, 5 bytes
								writeValue_uint24(f, j + 1, byteorder);
								writeValue_uint40(f, i + 1, byteorder);
							}
							else {
								std::println(std::cerr, "Unexpected version");
							}
						}
					},
				}, column);
			}
		}
		writetag(f, "</data>");

		map.strls = f.tellp();
		writetag(f, "<strls>");
		std::string GSO = "GSO";
		for (std::uint64_t i = 0; i < nobs; i++) {
			for (std::uint32_t j = 0; j < nvar; j++) {
				if (std::holds_alternative<std::vector<strL> >(datacolumns[j])) {
					const auto& content = std::get<std::vector<strL> >(datacolumns[j])[i];
					std::visit(overloaded{
						[&](const std::string& str) {
							if (str.empty() == false) {
								f.write((char*)GSO.data(), 3);
								writeValue(f, j + 1, byteorder);
								if (fileversion == 117) {
									writeValue(f, static_cast<std::uint32_t>(i + 1), byteorder);
								}
								else {
									writeValue(f, i + 1, byteorder);
								}
								std::uint8_t type = 130;
								writeValue(f, type, byteorder);
								std::uint32_t len = static_cast<std::uint32_t>(str.length() + 1);
								writeValue(f, len, byteorder);
								writeStringZ(f, str);
							}
						},
						[&](const std::vector<char>& chars) {
							if (chars.empty() == false) {
								f.write((char*)GSO.data(), 3);
								writeValue(f, j + 1, byteorder);
								if (fileversion == 117) {
									writeValue(f, static_cast<std::uint32_t>(i + 1), byteorder);
								}
								else {
									writeValue(f, i + 1, byteorder);
								}
								std::uint8_t type = 129;
								writeValue(f, type, byteorder);
								std::uint32_t len = static_cast<std::uint32_t>(chars.size());
								writeValue(f, len, byteorder);
								writeCharacters(f, chars);
							}
						}
					}, content);
				}
			}
		}
		writetag(f, "</strls>");

		map.vallabs = f.tellp();
		writetag(f, "<value_labels>");
		for (auto& [label, table] : label_table) {
			writetag(f, "<lbl>");
			std::uint32_t catsize = static_cast<std::int32_t>(table.size());
			std::vector<std::uint32_t> offs(catsize, 0);
			std::vector<std::int32_t> vals(catsize, 0);

			std::uint32_t buflen = 0;
			for (std::uint32_t j = 0; j < catsize; j++) {
				offs[j] = buflen;
				vals[j] = table[j].first;
				buflen += static_cast<std::uint32_t>(table[j].second.length());
				buflen++;
			}

			std::uint32_t cattablen = sizeof(std::int32_t) + sizeof(std::int32_t) + (sizeof(std::int32_t) * catsize) + (sizeof(std::int32_t) * catsize) + buflen;
			writeValue(f, cattablen, byteorder);

			writeStringZ(f, label, fileinfo.varname_width);

			std::vector<char> pads(3, '\0');
			writeCharacters(f, pads);

			writeValue(f, catsize, byteorder);
			writeValue(f, buflen, byteorder);
			for (std::uint32_t i = 0; i < catsize; ++i) {
				writeValue(f, offs[i], byteorder);
			}
			for (std::uint32_t i = 0; i < catsize; ++i) {
				writeValue(f, vals[i], byteorder);
			}

			for (std::uint32_t j = 0; j < catsize; j++) {
				const std::string& catname = table[j].second;
				writeStringZ(f, catname);
			}
			writetag(f, "</lbl>");
		}
		writetag(f, "</value_labels>");
		map.tlr = f.tellp();

		writetag(f, "</stata_dta>");
		map.bot = f.tellp();

		f.seekp(map.map);
		writetag(f, "<map>");
		writeValue(f, map.hdr, byteorder);
		writeValue(f, map.map, byteorder);
		writeValue(f, map.types, byteorder);
		writeValue(f, map.varnames, byteorder);
		writeValue(f, map.srtlist, byteorder);
		writeValue(f, map.fmts, byteorder);
		writeValue(f, map.vlblnames, byteorder);
		writeValue(f, map.varlabs, byteorder);
		writeValue(f, map.chars, byteorder);
		writeValue(f, map.data, byteorder);
		writeValue(f, map.strls, byteorder);
		writeValue(f, map.vallabs, byteorder);
		writeValue(f, map.tlr, byteorder);
		writeValue(f, map.bot, byteorder);
		writetag(f, "</map>");
		return true;
	}

	inline bool DTAFile::writeStata(std::ostream& f, std::uint8_t version, std::endian byteorder) const {
		if (version > 115) {
			return writeStataxml(f, version, byteorder);
		}

		if (version == 102) {
			if (byteorder != std::endian::little) {
				std::println("NOTE: 102 format does not support saving in big-endian format");
			}
			byteorder = std::endian::little;
		}

		std::vector<std::uint8_t> typlist(nvar);
		if (version >= 111 && version != 112) {
			for (std::uint32_t i = 0; i < nvar; ++i) {
				for (std::uint32_t i = 0; i < nvar; ++i) {
					const ColumnData& column = datacolumns[i];
					const std::size_t& slen = strlens[i];
					typlist[i] = std::visit(overloaded{
						[](std::monostate) -> std::uint8_t {
							std::println(std::cerr, "Chosen format does not support alias types");
							return 0;
						},
						[](const std::vector<std::int8_t>& data) -> std::uint8_t {
							return 251;
						},
						[](const std::vector<std::int16_t>& data) -> std::uint8_t {
							return 252;
						},
						[](const std::vector<std::int32_t>& data) -> std::uint8_t {
							return 253;
						},
						[](const std::vector<float>& data) -> std::uint8_t {
							return 254;
						},
						[](const std::vector<double>& data) -> std::uint8_t {
							return 255;
						},
						[slen](const std::vector<std::string>& data) -> std::uint8_t {
							if (slen > 244) {
								std::println(std::cerr, "Chosen format does not support strings longer than 244 characters");
								return false;
							}
							return static_cast<std::uint8_t>(slen);
						},
						[](const std::vector<strL>& data) -> std::uint8_t {
							std::println(std::cerr, "Chosen format does not support strL types");
							return 0;
						},
					}, column);
					if (typlist[i] == 0) {
						return false;
					}
				}
			}
		}
		else {
			for (std::uint32_t i = 0; i < nvar; ++i) {
				const ColumnData& column = datacolumns[i];
				const std::size_t& slen = strlens[i];
				typlist[i] = std::visit(overloaded{
					[](std::monostate) -> std::uint8_t {
						std::println(std::cerr, "Chosen format does not support alias types");
						return 0;
					},
					[](const std::vector<std::int8_t>& data) -> std::uint8_t {
						return 'b';
					},
					[](const std::vector<std::int16_t>& data) -> std::uint8_t {
						return 'i';
					},
					[](const std::vector<std::int32_t>& data) -> std::uint8_t {
						return 'l';
					},
					[](const std::vector<float>& data) -> std::uint8_t {
						return 'f';
					},
					[](const std::vector<double>& data) -> std::uint8_t {
						return 'd';
					},
					[slen](const std::vector<std::string>& data) -> std::uint8_t {
						if (slen > 128) {
							std::println(std::cerr, "Chosen format does not support strings longer than 128 characters");
							return false;
						}
						return static_cast<std::uint32_t>(slen) + 0x7f;
					},
					[](const std::vector<strL>& data) -> std::uint8_t {
						std::println(std::cerr, "Chosen format does not support strL types");
						return 0;
					},
				}, column);
				if (typlist[i] == 0) {
					return false;
				}
			}
		}

		writeValue(f, std::uint8_t(version), byteorder); // format version
		if (version == 102) {
			writeValue(f, std::uint8_t(0), byteorder);
		}
		else {
			if (byteorder == std::endian::little) {
				writeValue(f, std::uint8_t(2), byteorder);
			}
			else if (byteorder == std::endian::big) {
				writeValue(f, std::uint8_t(1), byteorder);
			}
			else {
				writeValue(f, std::uint8_t(0), byteorder); // Unknown byteorder
			}
		}
		writeValue(f, std::uint8_t(1), byteorder); // file type
		writeValue(f, std::uint8_t(0), byteorder); // reserved
			
		dtainformation fileinfo = getVersionInformation(version);

		if (fileinfo.varname_width == 0 || fileinfo.varlab_width == 0 || fileinfo.format_width == 0) {
			std::println(std::cerr, "Unrecognised Stata format");
			return false;
		}

		if (nvar > std::numeric_limits<uint16_t>::max()) {
			std::println(std::cerr, "Chosen format version does not support {:d} variables", nvar);
			return false;
		}
		uint16_t nnvar = static_cast<uint16_t>(nvar);
		writeValue(f, nnvar, byteorder);
		if (version == 102) {
			if (nobs > std::numeric_limits<uint16_t>::max()) {
				std::println(std::cerr, "Chosen format version does not support {:d} observations", nobs);
				return false;
			}
			std::uint16_t nnobs = static_cast<std::uint16_t>(nobs);
			writeValue(f, nnobs, byteorder);
		}
		else {
			if (nobs > std::numeric_limits<uint32_t>::max()) {
				std::println(std::cerr, "Chosen format version does not support {:d} observations", nobs);
				return false;
			}
			std::uint32_t nnobs = static_cast<std::uint32_t>(nobs);
			writeValue(f, nnobs, byteorder);
		}

		writeString(f, data_label, fileinfo.varlab_width);

		if (version > 104) {
			if (time_stamp.empty() == false) {
				writeStringZ(f, time_stamp, 18);
			}
			else {
				writeStringZ(f, "", 18);
			}
		}

		if (nvar > 0) f.write((char*)typlist.data(), typlist.size());

		for (std::uint32_t i = 0; i < nvar; i++) {
			writeStringZ(f, varlist[i], fileinfo.varname_width);
		}

		for (std::uint32_t i = 0; i < nvar + 1; ++i) {
			std::uint16_t srt = srtlist[i];
			writeValue(f, srt, byteorder);
		}

		for (std::uint32_t i = 0; i < nvar; i++) {
			writeStringZ(f, fmtlist[i], fileinfo.format_width);
		}

		for (std::uint32_t i = 0; i < nvar; i++) {
			writeStringZ(f, lbllist[i], fileinfo.varname_width);
		}

		for (std::uint32_t i = 0; i < nvar; i++) {
			writeStringZ(f, varlabs[i], fileinfo.varlab_width);
		}

		if (version > 104) {
			if (characteristics.size() > 0) {
				for (auto& [varname, charname, content] : characteristics) {
					std::uint8_t data_type = 1;
					writeValue(f, data_type, byteorder);
					if (version >= 110) {
						std::uint32_t len = 0;
						len += fileinfo.varname_width;
						len += fileinfo.varname_width;
						len += static_cast<uint32_t>(content.length()) + 1;
						writeValue(f, len, byteorder);
					}
					else {
						std::uint16_t len = 0;
						len += fileinfo.varname_width;
						len += fileinfo.varname_width;
						len += static_cast<uint16_t>(content.length()) + 1;
						writeValue(f, len, byteorder);
					}
					writeStringZ(f, varname, fileinfo.varname_width);
					writeStringZ(f, charname, fileinfo.varname_width);
					writeStringZ(f, content);
				}
			}
			// Write end record
			if (version >= 110) {
				std::vector<char> rec_end(5, '\0');
				writeCharacters(f, rec_end);
			}
			else {
				std::vector<char> rec_end(3, '\0');
				writeCharacters(f, rec_end);
			}
		}
		for (std::uint64_t i = 0; i < nobs; i++) {
			for (std::uint32_t j = 0; j < nvar; j++) {
				const ColumnData& column = datacolumns[j];
				const std::size_t& slen = strlens[j];
				std::visit(overloaded{
					[](std::monostate) {
						std::println(std::cerr, "Chosen format version does not support alias type");
					},
					[&f, i, byteorder](const std::vector<std::int8_t>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, byteorder](const std::vector<std::int16_t>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, byteorder](const std::vector<std::int32_t>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, byteorder](const std::vector<float>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, byteorder](const std::vector<double>& data) {
						writeValue(f, data[i], byteorder);
					},
					[&f, i, slen](const std::vector<std::string>& data) {
						writeString(f, data[i], slen);
					},
					[](const std::vector<strL>& data) {
						std::println(std::cerr, "Chosen format version does not support strL type");
					},
				}, column);
			}
		}

		for (auto& [label, table] : label_table) {
			if (version < 108) {
				std::uint16_t catsize = static_cast<std::uint16_t>(table.size());
				writeValue(f, catsize, byteorder);
				writeStringZ(f, label, fileinfo.varname_width);
				std::vector<char> pad(1, '\0');
				writeCharacters(f, pad);
				for (std::uint16_t j = 0; j < catsize; j++) {
					std::int16_t code = static_cast<std::int16_t>(table[j].first);
					writeValue(f, code, byteorder);
				}
				for (std::uint16_t j = 0; j < catsize; j++) {
					const std::string& label = table[j].second;
					writeString(f, label, 8);
				}
			}
			else {
				std::uint32_t catsize = static_cast<std::int32_t>(table.size());
				std::vector<std::uint32_t> offs(catsize, 0);
				std::vector<std::int32_t> vals(catsize, 0);

				std::uint32_t buflen = 0;
				for (std::uint32_t j = 0; j < catsize; j++) {
					offs[j] = buflen;
					vals[j] = table[j].first;
					buflen += static_cast<std::uint32_t>(table[j].second.length());
					buflen++;
				}

				std::uint32_t cattablen = sizeof(std::int32_t) + sizeof(std::int32_t) + (sizeof(std::int32_t) * catsize) + (sizeof(std::int32_t) * catsize) + buflen;

				writeValue(f, cattablen, byteorder);
				writeStringZ(f, label, fileinfo.varname_width);
				std::vector<char> pads(3, '\0');
				writeCharacters(f, pads);
				writeValue(f, catsize, byteorder);
				writeValue(f, buflen, byteorder);
				for (std::uint32_t i = 0; i < catsize; ++i) {
					writeValue(f, offs[i], byteorder);
				}
				for (std::uint32_t i = 0; i < catsize; ++i) {
					writeValue(f, vals[i], byteorder);
				}
				for (std::uint32_t j = 0; j < catsize; j++) {
					const std::string& catname = table[j].second;
					writeStringZ(f, catname);
				}
			}
		}
		return true;
	}

	inline bool DTAFile::Save(std::string_view filename, std::uint8_t version, std::endian byteorder) const {
		if (version != fileversion) {
			DTAFile copy(*this);
			if (copy.changeVersion(version) == true) {
				return copy.Save(filename, version, byteorder);
			}
			else {
				return false;
			}

		}
		else {
			std::ofstream f(std::filesystem::path(filename), std::ifstream::binary);
			if (f.bad()) {
				std::println(std::cerr, "Invalid file");
				return false;
			}
			return writeStata(f, version, byteorder);
		}
	}

	inline bool DTAFile::changeVersion(std::uint8_t version) {
		if (version < 103 && nobs > std::numeric_limits<std::uint16_t>::max()) {
			std::println(std::cerr, "Too many obversations to store in chosen format");
			return false;
		}
		if (version < 118 && nobs > std::numeric_limits<std::uint32_t>::max()) {
			std::println(std::cerr, "Too many obversations to store in chosen format");
			return false;
		}
		if (version != 119 && version != 121 && nvar > std::numeric_limits<std::uint16_t>::max()) {
			std::println(std::cerr, "Too many variables to store in the chosen format");
			return false;
		}
		dtainformation fileinfo = getVersionInformation(version);
		if (fileinfo.format_width == 0 || fileinfo.varlab_width == 0 || fileinfo.varname_width == 0) {
			std::println(std::cerr, "Invalid version");
			return false;
		}

		if (data_label.length() > fileinfo.varlab_width - 1) {
			std::println(std::cerr, "Data label ({:s}) too long to store in chosen format", data_label);
			return false;
		}

		if (time_stamp.length() != 0 && time_stamp.length() != 17) {
			// Warn, but don't return if time stamp is unexpected length (this is the case for some Stata examples).
			std::println(std::cerr, "Wrong length time stamp ({:d})", time_stamp.length());
		}

		for (const auto& varname : varlist) {
			if (varname.length() > fileinfo.varname_width - 1) {
				std::println(std::cerr, "Variable name ({:s}) too long for chosen format", varname);
				return false;
			}
		}

		for (const auto& label : lbllist) {
			if (label.length() > fileinfo.varname_width - 1) {
				std::println(std::cerr, "Value label name ({:s}) too long for chosen format", label);
				return false;
			}
		}

		for (const auto& format : fmtlist) {
			if (format.length() > fileinfo.format_width - 1) {
				std::println(std::cerr, "Format ({:s}) too long for chosen format", format);
				return false;
			}
		}

		for (const auto& varlabel : varlabs) {
			if (varlabel.length() > fileinfo.varlab_width - 1) {
				std::println(std::cerr, "Variable label ({:s}) too long for chosen format", varlabel);
				return false;
			}
		}

		for (const auto& [varlab, table] : label_table) {
			if (varlab.length() > fileinfo.varname_width - 1) {
				std::println(std::cerr, "Value label name ({:s}) too long for chosen format", varlab);
				return false;
			}
			if (version < 110) {
				for (const auto& [code, label] : table) {
					if (label.length() > 8) {
						std::println(std::cerr, "Value label ({:s}) too long for chosen format", label);
						return false;
					}
				}
			}
		}

		for (const auto& [varname, charname, characteristic] : characteristics) {
			if (varname.length() > fileinfo.varname_width - 1) {
				std::println(std::cerr, "Value name ({:s}) too long for chosen format", varname);
				return false;
			}
			if (charname.length() > fileinfo.varname_width - 1) {
				std::println(std::cerr, "Characteristic name ({:s}) too long for chosen format", charname);
				return false;
			}
			if (version < 110) {
				if (characteristic.size() > std::numeric_limits<std::uint16_t>::max()) {
					std::println(std::cerr, "Characteristic too long ({:d}) for chosen format", characteristic.size());
					return false;
				}
			}
			else {
				if (characteristic.size() > std::numeric_limits<std::uint32_t>::max()) {
					std::println(std::cerr, "Characteristic too long ({:d}) for chosen format", characteristic.size());
					return false;
				}
			}
		}

		for (auto& column : datacolumns) {
			bool okay = std::visit(overloaded{
				[](std::monostate) {
					return true;
				},
				[&](std::vector<std::int8_t>& data) {
					ColumnSummary<std::int8_t> summ(data, fileversion);
					bool widentype = false;
					if (version < 104) { // byte format not supported until 104, change to an int
						widentype = true;
					}
					if (version < 112) {
						for (const auto& value : data) {
							if (isMissing(value, fileversion) == false && value > stata_limits_v2<std::int8_t>::max()) {
								widentype = true;
							}
						}
					}

					if (widentype == true) {
						std::vector<std::int16_t> outdata(data.size());
						for (std::size_t i = 0; i < data.size(); ++i) {
							if (isMissing(data[i], fileversion) == true) {
								if (version < 112) {
									outdata[i] = stata_limits_v1<std::int16_t>::missing();
								}
								else {
									outdata[i] = stata_limits_v2<std::int16_t>::missing();
								}
							}
							else {
								outdata[i] = data[i];
							}
						}
						column = outdata;
					}
					else {
						if (fileversion < 112 && version >= 112) { // Update to v2 int8
							for (auto&& value : data) {
								if (isMissing(value, fileversion) == true) {
									value = stata_limits_v2<std::int8_t>::missing();
								}
							}
						}
						if (fileversion >= 112 && version < 112) { // Update to v1 int8
							for (auto&& value : data) {
								if (isMissing(value, fileversion) == true) {
									value = stata_limits_v1<std::int8_t>::missing();
								}
							}
						}
					}
					return true;
				},
				[&](std::vector<std::int16_t>& data) {
					ColumnSummary<std::int16_t> summ(data, fileversion);
					bool widentype = false;
					if (version < 112) {
						for (const auto& value : data) {
							if (isMissing(value, fileversion) == false && value > stata_limits_v2<std::int16_t>::max()) {
								widentype = true;
							}
						}
					}

					if (widentype == true) {
						std::vector<std::int32_t> outdata(data.size());
						for (std::size_t i = 0; i < data.size(); ++i) {
							if (isMissing(data[i], fileversion) == true) {
								if (version < 112) {
									outdata[i] = stata_limits_v1<std::int32_t>::missing();
								}
								else {
									outdata[i] = stata_limits_v2<std::int32_t>::missing();
								}
							}
							else {
								outdata[i] = data[i];
							}
						}
						column = outdata;
					}
					else {
						if (fileversion < 112 && version >= 112) { // Update to v2 int16
							for (auto&& value : data) {
								if (isMissing(value, fileversion) == true) {
									value = stata_limits_v2<std::int16_t>::missing();
								}
							}
						}
						if (fileversion >= 112 && version < 112) { // Update to v1 int16
							for (auto&& value : data) {
								if (isMissing(value, fileversion) == true) {
									value = stata_limits_v1<std::int16_t>::missing();
								}
							}
						}
					}
					return true;
				},
				[&](std::vector<std::int32_t>& data) {
					ColumnSummary<std::int32_t> summ(data, fileversion);
					bool widentype = false;
					if (version < 112) {
						for (const auto& value : data) {
							if (isMissing(value, fileversion) == false && value > stata_limits_v2<std::int32_t>::max()) {
								widentype = true;
							}
						}
					}

					if (widentype == true) {
						std::vector<double> outdata(data.size());
						for (std::size_t i = 0; i < data.size(); ++i) {
							if (isMissing(data[i], fileversion) == true) {
								if (version < 112) {
									outdata[i] = stata_limits_v1<double>::missing();
								}
								else {
									outdata[i] = stata_limits_v2<double>::missing();
								}
							}
							else {
								outdata[i] = data[i];
							}
						}
						column = outdata;
					}
					else {
						if (fileversion < 112 && version >= 112) { // Update to v2 int32
							for (auto&& value : data) {
								if (isMissing(value, fileversion) == true) {
									value = stata_limits_v2<std::int32_t>::missing();
								}
							}
						}
						if (fileversion >= 112 && version < 112) { // Update to v1 int32
							for (auto&& value : data) {
								if (isMissing(value, fileversion) == true) {
									value = stata_limits_v1<std::int32_t>::missing();
								}
							}
						}
					}
					return true;
				},
				[&](std::vector<float>& data) {
					ColumnSummary<float> summ(data, fileversion);
					bool widentype = false;
					if (version < 108) {
						for (const auto& value : data) {
							if (isMissing(value, fileversion) == false && (value > stata_limits_v1<float>::max() || value < stata_limits_v1<float>::lowest())) {
								widentype = true;
							}
						}
					}
					if (widentype == true) {
						std::vector<double> outdata(data.size());
						for (std::size_t i = 0; i < data.size(); ++i) {
							if (isMissing(data[i], fileversion) == true) {
								if (version < 108) {
									outdata[i] = stata_limits_v1<double>::missing();
								}
								else {
									outdata[i] = stata_limits_v2<double>::missing();
								}
							}
							else {
								outdata[i] = data[i];
							}
						}
						column = outdata;
					}
					else {
						if (fileversion < 108 && version >= 108) { // Update to v2 float
							for (auto&& value : data) {
								if (isMissing(value, fileversion) == true) {
									value = stata_limits_v2<float>::missing();
								}
							}
						}
						if (fileversion >= 108 && version < 108) { // Update to v1 float
							for (auto&& value : data) {
								if (isMissing(value, fileversion) == true) {
									value = stata_limits_v1<float>::missing();
								}
							}
						}
					}
					return true;
				},
				[&](std::vector<double>& data) {
					ColumnSummary<double> summ(data, fileversion);
					if (version < 108) {
						for (const auto& value : data) {
							if (isMissing(value, fileversion) == false && (value > stata_limits_v1<double>::max()) || value < stata_limits_v1<double>::lowest()) {
								std::println(std::cerr, "WARNING: {:g} is not a valid non-missing value for chosen format", value);
							}
						}
					}

					if (fileversion < 108 && version >= 108) { // Update to v2 double
						for (auto&& value : data) {
							if (isMissing(value, fileversion) == true) {
								value = stata_limits_v2<double>::missing();
							}
						}
					}
					if (fileversion >= 108 && version < 108) { // Update to v1 double
						for (auto&& value : data) {
							if (isMissing(value, fileversion) == true) {
								value = stata_limits_v1<double>::missing();
							}
						}
					}
					return true;
				},
				[&](std::vector<std::string>& data) {
					if (version == 102) {
						std::println(std::cerr, "string types not supported in chosen format");
						return false;
					}
					ColumnSummary<std::string> summ(data, fileversion);
					if (version < 111 && summ.max > 80) {
						std::println(std::cerr, "String variable too long ({:d}) to store in chosen format where maximum is {:d}", summ.max, 80);
						return false;
					}
					if (version < 117 && summ.max > 244) {
						std::println(std::cerr, "String variable too long ({:d}) to store in chosen format where maximum is {:d}", summ.max, 244);
						return false;
					}
					return true;
				},
				[&](std::vector<strL>& data) {
					if (version < 117) {
						std::println(std::cerr, "strL type not supported in chosen format");
						return false;
					}
					return true;
				}
			}, column);
			if (okay == false) {
				return false;
			}
		}

		fileversion = version;
		return true;
	}

	[[nodiscard]] inline std::uint8_t DTAFile::GetOriginalFileVersion() const {
		return fileversion;
	}

	[[nodiscard]] inline std::endian DTAFile::GetOriginalByteOrder() const {
		return byteorder;
	}

	[[nodiscard]] inline std::string DTAFile::GetDataLabel() const {
		return data_label;
	}

	[[nodiscard]] inline std::string DTAFile::GetTimeStamp() const {
		return time_stamp;
	}

	[[nodiscard]] inline std::string DTAFile::GetVariableName(std::uint32_t varnumber) const {
		if (varnumber < 1 || varnumber > nvar) {
			std::println(std::cerr, "Variable index out of range");
			return "";
		}
		return varlist[varnumber - 1];
	}

	[[nodiscard]] inline std::string DTAFile::GetVariableLabel(std::string_view varname) const {
		try {
			std::uint32_t index = variableIndex(varname);
			return lbllist[index];
		}
		catch (std::out_of_range&) {
			std::println(std::cerr, "{:s} not found", varname);
		}
		return "";
	}

	[[nodiscard]] inline std::string DTAFile::GetValueLabelName(std::string_view varname) const {
		try {
			std::uint32_t index = variableIndex(varname);
			return varlabs[index];
		}
		catch (std::out_of_range&) {
			std::println(std::cerr, "{:s} not found", varname);
		}
		return "";
	}


	[[nodiscard]] inline std::string DTAFile::GetVariableType(std::string_view varname) const {
		try {
			std::uint32_t index = variableIndex(varname);
			const ColumnData& column = datacolumns[index];
			return std::visit(overloaded{
				[](std::monostate) -> std::string {
					return "alias";
				},
				[](const std::vector<std::int8_t>& ) -> std::string {
					return "byte";
				},
				[](const std::vector<std::int16_t>& ) -> std::string {
					return "int";
				},
				[](const std::vector<std::int32_t>& ) -> std::string {
					return "long";
				},
				[](const std::vector<float>& ) -> std::string {
					return "float";
				},
				[](const std::vector<double>& ) -> std::string {
					return "double";
				},
				[&, index](const std::vector<std::string>& ) -> std::string {
					return "str" + std::to_string(strlens[index]);
				},
				[](const std::vector<strL>& ) -> std::string {
					return "strL";
				},
			}, column);
		}
		catch (std::out_of_range&) {
			std::println(std::cerr, "{:s} not found", varname);
		}
		return "";
	}

	[[nodiscard]] inline std::string DTAFile::GetFormat(std::string_view varname) const {
		try {
			std::uint32_t index = variableIndex(varname);
			return fmtlist[index];
		}
		catch (std::out_of_range&) {
			std::println(std::cerr, "{:s} not found", varname);
		}
		return "";
	}

	[[nodiscard]] inline std::vector<std::string> DTAFile::GetValueLabelNames() const {
		std::vector<std::string> labnames;
		for (const auto& [labname, table] : label_table) {
			labnames.push_back(labname);
		}
		return labnames;
	}

	[[nodiscard]] inline std::vector<std::int32_t> DTAFile::GetValueLabelCodes(std::string labname) const {
		if (label_table.contains(labname) == false) {
			std::println(std::cerr, "Label name {:s} not found", labname);
			throw std::out_of_range("Label name not found");
		}
		std::vector<std::int32_t> codes;
		const auto& table = label_table.at(labname);
		for (const auto& [code, text] : table) {
			codes.push_back(code);
		}
		return codes;
	}

	[[nodiscard]] inline std::string DTAFile::GetValueLabelString(std::string labname, std::int32_t value) const {
		if (label_table.contains(labname) == false) {
			std::println(std::cerr, "Label name {:s} not found", labname);
			throw std::out_of_range("Label name not found");
		}
		const auto& table = label_table.at(labname);
		for (const auto& [code, text] : table) {
			if (code == value) {
				return text;
			}
		}
		return "";
	}

	[[nodiscard]] inline std::uint64_t DTAFile::GetNumObservations() const {
		return nobs;
	}

	[[nodiscard]] inline std::uint32_t DTAFile::GetNumVariables() const {
		return nvar;
	}

	[[nodiscard]] inline std::vector<std::string> DTAFile::GetSortOrder() const {
		std::vector<std::string> sortorder;
		for (const auto& varnum : srtlist) {
			if (varnum == 0) {
				break;
			}
			sortorder.push_back(varlist[varnum - 1]);
		}
		return sortorder;
	}

	[[nodiscard]] inline std::vector<std::string> DTAFile::GetCharacteristicNames(std::string_view varname) const {
		std::vector<std::string> charnames;
		for (const auto& [vname, cname, content] : characteristics) {
			charnames.push_back(cname);
		}
		return charnames;
	}

	[[nodiscard]] inline std::string DTAFile::GetCharacteristic(std::string_view varname, std::string_view charname) const {
		for (const auto& [vname, cname, content] : characteristics) {
			if (vname == varname && cname == charname) {
				return content;
			}
		}
		return "";
	}

	inline bool DTAFile::SetDataLabel(std::string_view label) {
		dtainformation fileinfo = getVersionInformation(fileversion);

		if (label.size() > fileinfo.varlab_width - 1) {
			std::println(std::cerr, "{:s} too long for chosen format", label);
			return false;
		}
		data_label = std::string(label);

		return true;
	}

	inline void DTAFile::StampTime() {
		auto time = std::chrono::system_clock::now();
		time_stamp = std::format("{:%d} {:%b} {:%Y} {:%R}", time, time, time, time);
	}

	inline bool DTAFile::SetVariableLabel(std::string_view varname, std::string_view label) {
		dtainformation fileinfo = getVersionInformation(fileversion);

		if (label.size() > fileinfo.varlab_width - 1) {
			std::println(std::cerr, "{:s} too long for chosen format", label);
			return false;
		}
		auto index = variableIndex(varname);
		if (index > nvar) {
			std::println(std::cerr, "{:s} not found", varname);
			return false;
		}
		varlabs[index] = std::string(label);

		return true;
	}

	inline bool DTAFile::AssignValueLabel(std::string_view varname, std::string_view labname) {
		dtainformation fileinfo = getVersionInformation(fileversion);

		if (labname.size() > fileinfo.varname_width - 1) {
			std::println(std::cerr, "{:s} too long for chosen format", labname);
			return false;
		}
		auto index = variableIndex(varname);
		if (index > nvar) {
			std::println(std::cerr, "{:s} not found", varname);
			return false;
		}
		bool exists = false;
		for (const auto& [valname, table] : label_table) {
			if (valname == labname) {
				exists = true;
			}
		}
		if (exists == false) {
			std::println(std::cerr, "Label entry for {:} not found", varname);
			return false;
		}
		lbllist[index] = std::string(labname);

		return true;
	}

	inline void DTAFile::SortData(const std::vector<std::string_view> &keys) {
		std::vector<std::uint32_t> sortkeys;
		for (const auto& key : keys) {
			auto colindex = variableIndex(key);
			sortkeys.push_back(colindex);
		}

		std::fill(srtlist.begin(), srtlist.end(), 0);
		std::transform(sortkeys.cbegin(), sortkeys.cend(), srtlist.begin(), [](std::uint32_t ix) { return ix + 1; });
		std::vector<std::uint64_t> idx(nobs);
		std::iota(idx.begin(), idx.end(), 0);
		std::sort(idx.begin(), idx.end(), [&](std::uint64_t i1, std::uint64_t i2) -> bool {
			for (const auto& key : sortkeys) {
				auto& column = datacolumns[key];
				return std::visit(overloaded{
					[](std::monostate) {
						return false;
					},
					[i1, i2](const std::vector<std::int8_t>& data) {
						if (data[i1] < data[i2]) {
							return true;
						}
						else {
							return false;
						}
					},
					[i1, i2](const std::vector<std::int16_t>& data) {
						if (data[i1] < data[i2]) {
							return true;
						}
						else {
							return false;
						}
					},
					[i1, i2](const std::vector<std::int32_t>& data) {
						if (data[i1] < data[i2]) {
							return true;
						}
						else {
							return false;
						}
					},
					[i1, i2](const std::vector<float>& data) {
						if (data[i1] < data[i2]) {
							return true;
						}
						else {
							return false;
						}
					},
					[i1, i2](const std::vector<double>& data) {
						if (data[i1] < data[i2]) {
							return true;
						}
						else {
							return false;
						}
					},
					[i1, i2](const std::vector<std::string>& data) {
						if (data[i1] < data[i2]) {
							return true;
						}
						else {
							return false;
						}
					},
					[i1, i2](const std::vector<strL>& data) {
						if (std::holds_alternative<std::string>(data[i1]) && std::holds_alternative<std::string>(data[i2])) {
							const std::string& str1 = std::get<std::string>(data[i1]);
							const std::string& str2 = std::get<std::string>(data[i2]);
							if (str1 < str2) {
								return true;
							}
							else {
								return false;
							}
						}
						else {
							return false;
						}
					}
				}, column);
			}
			return false;
		});
		// NOTE: We are replacing each column with a sorted version rather then sort in place
		// If we wanted to sort in place we would need to go row-by-row so idx is key in sync across columns
		for (auto &column : datacolumns) {
			std::visit(overloaded{
				[](std::monostate) {
					// Alias column, so ignore
				},
				[&](const std::vector<std::int8_t>& data) {
					std::vector<std::int8_t> sorted(data.size());
					for (std::uint64_t i = 0; i < idx.size(); ++i) {
						sorted[i] = data[idx[i]];
					}
					column = sorted;
				},
				[&](const std::vector<std::int16_t>& data) {
					std::vector<std::int16_t> sorted(data.size());
					for (std::uint64_t i = 0; i < idx.size(); ++i) {
						sorted[i] = data[idx[i]];
					}
					column = sorted;
				},
				[&](const std::vector<std::int32_t>& data) {
					std::vector<std::int32_t> sorted(data.size());
					for (std::uint64_t i = 0; i < idx.size(); ++i) {
						sorted[i] = data[idx[i]];
					}
					column = sorted;
				},
				[&](const std::vector<float>& data) {
					std::vector<float> sorted(data.size());
					for (std::uint64_t i = 0; i < idx.size(); ++i) {
						sorted[i] = data[idx[i]];
					}
					column = sorted;
				},
				[&](const std::vector<double>& data) {
					std::vector<double> sorted(data.size());
					for (std::uint64_t i = 0; i < idx.size(); ++i) {
						sorted[i] = data[idx[i]];
					}
					column = sorted;
				},
				[&](const std::vector<std::string>& data) {
					std::vector<std::string> sorted(data.size());
					for (std::uint64_t i = 0; i < idx.size(); ++i) {
						sorted[i] = data[idx[i]];
					}
					column = sorted;
				},
				[&](const std::vector<strL>& data) {
					std::vector<strL> sorted(data.size());
					for (std::uint64_t i = 0; i < idx.size(); ++i) {
						sorted[i] = data[idx[i]];
					}
					column = sorted;
				}
			}, column);
		}
	}

	inline bool DTAFile::SetFormat(std::string_view varname, std::string_view format) {
		dtainformation fileinfo = getVersionInformation(fileversion);

		if (format.size() > fileinfo.format_width - 1) {
			std::println(std::cerr, "{:s} too long for chosen format", format);
			return false;
		}
		auto index = variableIndex(varname);
		if (index > nvar) {
			std::println(std::cerr, "{:s} not found", varname);
		}
		fmtlist[index] = std::string(format);

		return true;
	}

	inline bool DTAFile::SetObservations(std::uint64_t obs) {
		if (fileversion < 103 && nobs > std::numeric_limits<std::uint16_t>::max()) {
			std::println(std::cerr, "Current file format unable to store {:d} observations", nobs);
			return false;
		}
		if (fileversion < 118 && nobs > std::numeric_limits<std::uint32_t>::max()) {
			std::println(std::cerr, "Current file format unable to store {:d} observations", nobs);
			return false;
		}
		if (obs < nobs) {
			std::println(std::cerr, "WARNING {:d} observations is lower than previous limit {:d}, some observations will be lost", obs, nobs);
		}

		for (std::uint32_t i = 0; i < nvar; ++i) {
			ColumnData& column = datacolumns[i];
			std::visit(overloaded{
				[](std::monostate) {
					// Alias type has no rows
				},
				[&, obs](std::vector<std::int8_t>& data) {
					if (fileversion < 112) {
						data.resize(obs, stata_limits_v1<std::int8_t>::missing());
					}
					else {
						data.resize(obs, stata_limits_v2<std::int8_t>::missing());
					}
				},
				[&, obs](std::vector<std::int16_t>& data) {
					if (fileversion < 112) {
						data.resize(obs, stata_limits_v1<std::int16_t>::missing());
					}
					else {
						data.resize(obs, stata_limits_v2<std::int16_t>::missing());
					}
				},
				[&, obs](std::vector<std::int32_t>& data) {
					if (fileversion < 112) {
						data.resize(obs, stata_limits_v1<std::int32_t>::missing());
					}
					else {
						data.resize(obs, stata_limits_v2<std::int32_t>::missing());
					}
				},
				[&, obs](std::vector<float>& data) {
					if (fileversion < 108) {
						data.resize(obs, stata_limits_v1<float>::missing());
					}
					else {
						data.resize(obs, stata_limits_v2<float>::missing());
					}
				},
				[&, obs](std::vector<double>& data) {
					if (fileversion < 108) {
						data.resize(obs, stata_limits_v1<double>::missing());
					}
					else {
						data.resize(obs, stata_limits_v2<double>::missing());
					}
				},
				[&, obs](std::vector<std::string>& data) {
					data.resize(obs);
				},
				[&, obs](std::vector<strL>& data) {
					data.resize(obs);
				},
			}, column);
		}
		nobs = obs;
		return true;
	}

	inline bool DTAFile::SetSortOrder(const std::vector<std::string_view> &variablelist) {
		if (variablelist.size() > srtlist.size() - 1) {
			std::println(std::cerr, "Too many variables specified to sort on {:d}, maximum is {:d}", variablelist.size(), srtlist.size() - 1);
		}
		std::vector<std::uint32_t> varindices;
		for (const auto& varname : variablelist) {
			auto index = variableIndex(varname);
			if (index > nvar) {
				std::println(std::cerr, "Variable {:s} not found", varname);
				return false;
			}
			else {
				varindices.push_back(index + 1);
			}
		}
		std::fill(srtlist.begin(), srtlist.end(), 0);
		std::copy(varindices.cbegin(), varindices.cend(), srtlist.begin());
		return true;
	}

	inline bool DTAFile::AddVariable(std::string_view varname, std::string_view type) {
		if (std::find(reserved_words.cbegin(), reserved_words.cend(), varname) != reserved_words.cend()) {
			std::println(std::cerr, "WARNING: name \"{:s}\" is reserved by Stata for internal use", varname);
		}

		try {
			variableIndex(varname);
			std::println(std::cerr, "{:s} already exists in data", varname);
			return false;
		}
		catch (std::out_of_range&) {}

		if (fileversion != 119 && fileversion != 121 && nvar > std::numeric_limits<std::uint16_t>::max()) {
			std::println(std::cerr, "Too many variables to store in the chosen format");
			return false;
		}
		if (type.empty() == true) {
			std::println(std::cerr, "No type specified");
			return false;
		}

		switch (type.front()) {
		case 'b':
			if (fileversion < 104) {
				std::println(std::cerr, "byte type not supported with chosen format");
				return false;
			}
			else {
				if (fileversion < 112) {
					datacolumns.push_back(std::vector<std::int8_t>(nobs, stata_limits_v1<std::int8_t>::missing()));
				}
				else {
					datacolumns.push_back(std::vector<std::int8_t>(nobs, stata_limits_v2<std::int8_t>::missing()));
				}
				strlens.push_back(0);
				fmtlist.push_back("%8.0g");
			}
			break;
		case 'i':
			if (fileversion < 112) {
				datacolumns.push_back(std::vector<std::int16_t>(nobs, stata_limits_v1<std::int16_t>::missing()));
			}
			else {
				datacolumns.push_back(std::vector<std::int16_t>(nobs, stata_limits_v2<std::int16_t>::missing()));
			}
			strlens.push_back(0);
			fmtlist.push_back("%8.0g");
			break;
		case 'l':
			if (fileversion < 112) {
				datacolumns.push_back(std::vector<std::int32_t>(nobs, stata_limits_v1<std::int32_t>::missing()));
			}
			else {
				datacolumns.push_back(std::vector<std::int32_t>(nobs, stata_limits_v2<std::int32_t>::missing()));
			}
			strlens.push_back(0);
			fmtlist.push_back("%12.0g");
			break;
		case 'f':
			if (fileversion < 108) {
				datacolumns.push_back(std::vector<float>(nobs, stata_limits_v1<float>::missing()));
			}
			else {
				datacolumns.push_back(std::vector<float>(nobs, stata_limits_v2<float>::missing()));
			}
			strlens.push_back(0);
			fmtlist.push_back("%9.0g");
			break;
		case 'd':
			if (fileversion < 108) {
				datacolumns.push_back(std::vector<double>(nobs, stata_limits_v1<double>::missing()));
			}
			else {
				datacolumns.push_back(std::vector<double>(nobs, stata_limits_v2<double>::missing()));
			}
			strlens.push_back(0);
			fmtlist.push_back("%10.0g");
			break;
		case 's':
			if (type == "strL") {
				datacolumns.push_back(std::vector<strL>(nobs));
				strlens.push_back(0);
				fmtlist.push_back("%9s");
			}
			else {
				if (type.substr(0, 3) != "str") {
					std::println(std::cerr, "Unrecognised type {:s}", type);
				}
				if (fileversion < 103) {
					std::println(std::cerr, "fixed string variables are not supported in chosen format");
					return false;
				}
				if (type.length() == 1) {
					std::println(std::cerr, "length must be included when creating a fixed-length string variable");
					return false;
				}
				std::uint16_t slen{};
				auto [ptr, ec] = std::from_chars(type.data() + 3, type.data() + type.size(), slen);
				if (ptr == type.data() + type.size() && ec == std::errc{}) {
					if (fileversion < 111 && slen > 80) {
						std::println(std::cerr, "s{:d} not supported by chosen format", slen);
						return false;
					}
					if (fileversion < 117 && slen > 244) {
						std::println(std::cerr, "s{:d} not supported by chosen format", slen);
						return false;
					}
					if (slen > 2025) {
						std::println(std::cerr, "s{:d} not supported by chosen format", slen);
						return false;
					}
					datacolumns.push_back(std::vector<std::string>(nobs));
					strlens.push_back(slen);
					fmtlist.push_back(std::format("%{}s", slen));
				}
				else {
					std::println(std::cerr, "Error parsing string length");
					return false;
				}
			}
			break;
		case 'a':
			if (fileversion != 120 && fileversion != 121) {
				std::println(std::cerr, "Alias type not supported by chosen version");
				return false;
			}
			else {
				datacolumns.push_back(std::monostate{});
				strlens.push_back(0);
				fmtlist.push_back("%9.0g");
			}
			break;
		default:
			std::println(std::cerr, "Unrecognisd type {:s}", type);
			return false;
		}

		varlist.push_back(std::string(varname));
		lbllist.push_back("");
		varlabs.push_back("");
		srtlist.push_back(0);
		++nvar;

		return true;
	}

	inline bool DTAFile::AddValueLabel(std::string labname, std::int32_t value, std::string label) {
		if (label_table.contains(labname) == true) {
			auto& table = label_table[labname];
			if (fileversion < 108 && label_table.size() > std::numeric_limits<std::uint16_t>::max()) {
				std::println(std::cerr, "Label table for {:s} too large to add ({:d}, {:s})", labname, value, label);
				return false;
			}
			else {
				if (label_table.size() > std::numeric_limits<std::uint32_t>::max()) {
					std::println(std::cerr, "Label table for {:s} too large to add ({:d}, {:s})", labname, value, label);
					return false;
				}
			}
			table.push_back({ value, label });
		}
		else {
			std::vector<std::pair<std::int32_t, std::string> > table;
			table.push_back({ value, label });
			label_table[labname] = table;
		}
		return true;
	}

	inline bool DTAFile::AddCharacteristic(std::string_view varname, std::string_view charname, std::string characteristic) {
		if (varname != "_dta") {
			auto index = variableIndex(varname);
			if (index > nvar) {
				std::println(std::cerr, "{:} not found", varname);
				return false;
			}
		}
		for (auto&& [vname, cname, content] : characteristics) {
			if (vname == varname && cname == charname) {
				std::println(std::cerr, "characteristic {:s} for {:s} already exists", charname, varname);
				return false;
			}
		}
		characteristics.push_back({ std::string(varname), std::string(charname), characteristic });
		return true;
	}

	inline bool DTAFile::RemoveVariable(std::string_view varname) {
		try {
			std::uint32_t index = variableIndex(varname);
			varlist.erase(varlist.begin() + index);
			lbllist.erase(lbllist.begin() + index);
			fmtlist.erase(fmtlist.begin() + index);
			varlabs.erase(varlabs.begin() + index);
			srtlist.erase(srtlist.begin() + index);
			strlens.erase(strlens.begin() + index);
			datacolumns.erase(datacolumns.begin() + index);
			--nvar;
		}
		catch (std::out_of_range&) {
			std::println(std::cerr, "{:s} not found", varname);
			return false;
		}
		return true;
	}

	inline bool DTAFile::RemoveValueLabel(std::string labname, std::int32_t value) {
		if (label_table.contains(labname) == false) {
			std::println(std::cout, "No labels for {:s} found", labname);
			return false;
		}
		auto& table = label_table[labname];
		for (std::size_t i = 0; i < table.size(); ++i) {
			auto& [code, label] = table[i];
			if (code == value) {
				table.erase(table.begin() + i);
				if (table.empty()) {
					label_table.erase(labname);
					std::println(std::cout, "Label list {:s} removed as it is now empty", labname);
					for (size_t j = 0; j < nvar; ++j) {
						if (lbllist[j] == labname) {
							std::println(std::cout, "Label list {:s} no longer associated with {:s} as it has been removed", labname, lbllist[j]);
							lbllist[j] = "";
						}
					}
				}
				return true;
			}
		}
		std::println(std::cerr, "An entry with value {:d} was not found in {:s}", value, labname);
		return false;
	}

	inline bool DTAFile::RemoveCharacteristic(std::string_view varname, std::string_view charname) {
		for (std::size_t i = 0; i < characteristics.size(); ++i) {
			auto& [vname, cname, content] = characteristics[i];
			if (vname == varname && cname == charname) {
				characteristics.erase(characteristics.begin() + i);
				return true;
			}
		}
		std::println(std::cerr, "Characteristic {:s} associated with {:s} not found", charname, varname);
		return false;
	}

	inline bool DTAFile::RenameVariable(std::string_view oldname, std::string_view newname) {
		try {
			std::uint32_t index = variableIndex(oldname);
			try {
				variableIndex(newname);
				std::println(std::cerr, "{:s} already exists", newname);
			}
			catch (std::out_of_range&) {
				varlist[index] = std::string(newname);
			}
		}
		catch (std::out_of_range&) {
			std::println(std::cerr, "{:s} not found", oldname);
			return false;
		}
		return true;
	}

	inline bool DTAFile::RenameValueLabel(std::string oldname, std::string newname) {
		if (label_table.contains(oldname) == false) {
			std::println(std::cerr, "{:s} not found", oldname);
			return false;
		}
		if (label_table.contains(newname) == true) {
			std::println(std::cerr, "{:s} already exists", newname);
			return false;
		}
		auto nh = label_table.extract(oldname);
		nh.key() = newname;
		label_table.insert(std::move(nh));
		for (std::uint32_t i = 0; i < nvar; ++i) {
			if (lbllist[i] == oldname) {
				lbllist[i] = newname;
			}
		}
		return true;
	}

	[[nodiscard]] inline ColumnData DTAFile::GetVariable(std::string_view varname) const {
		auto index = variableIndex(varname);
		if (index < datacolumns.size()) {
			return datacolumns[index];
		}
		return ColumnData();
	}

	[[nodiscard]] inline std::size_t DTAFile::GetMaxStringLength(std::string_view varname) const {
		auto index = variableIndex(varname);
		if (index < datacolumns.size()) {
			const auto& val = datacolumns[index];
			if (std::holds_alternative<std::vector<std::string> >(val)) {
				return strlens[index];
			}
			else {
				std::println(std::cerr, "{:s} is not a string variable", varname);
				return 0;
			}
		}
		std::println(std::cerr, "{:s} not found", varname);
		return 0;
	}

	[[nodiscard]] inline std::vector<CellData> DTAFile::GetObservation(std::uint64_t row) const {
		if (row < 1 || row > nobs) {
			std::println(std::cerr, "{:d} out of range", row);
			throw std::out_of_range("Observations out of range");
		}
		std::vector<CellData> obs(nvar);
		for (std::size_t i = 0; i < nvar; ++i) {
			const auto& val = datacolumns[i];
			obs[i] = std::visit(overloaded{
				[](std::monostate) {
					return CellData(std::string(""));
				},
				[row](const std::vector<std::int8_t>& data) {
					auto value = data[row - 1];
					return CellData(value);
				},
				[row](const std::vector<std::int16_t>& data) {
					auto value = data[row - 1];
					return CellData(value);
				},
				[row](const std::vector<std::int32_t>& data) {
					auto value = data[row - 1];
					return CellData(value);
				},
				[row](const std::vector<float>& data) {
					auto value = data[row - 1];
					return CellData(value);
				},
				[row](const std::vector<double>& data) {
					auto value = data[row - 1];
					return CellData(value);
				},
				[row](const std::vector<std::string>& data) {
					const auto& value = data[row - 1];
					return CellData(value);
				},
				[row](const std::vector<strL>& data) {
					const auto& value = data[row - 1];
					return std::visit(overloaded{
						[](const std::string& val) {
							return CellData(val);
						},
						[](const std::vector<char>& val) {
							return CellData(val);
						}
					}, value);
				}
			}, val);
		}
		return obs;
	}

	[[nodiscard]] inline CellData DTAFile::GetValue(std::string_view varname, std::uint64_t row) const {
		if (row < 1 || row > nobs) {
			std::println(std::cerr, "{:d} out of range", row);
			throw std::out_of_range("Observation out of range");
		}
		auto ver = fileversion;
		auto index = variableIndex(varname);
		if (index < datacolumns.size()) {
			const auto& val = datacolumns[index];
			return std::visit(overloaded{
				[](std::monostate) {
					return CellData(std::string(""));
				},
				[row, &varname, ver](const std::vector<std::int8_t>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<std::int8_t>::max() || value < std::numeric_limits<int8_t>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
					}
					return CellData(value);
				},
				[row, &varname, ver](const std::vector<std::int16_t>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<std::int16_t>::max() || value < std::numeric_limits<std::int16_t>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
					}
					return CellData(value);
				},
				[row, &varname, ver](const std::vector<std::int32_t>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<std::int32_t>::max() || value < std::numeric_limits<std::int32_t>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
					}
					return CellData(value);
				},
				[row, &varname, ver](const std::vector<float>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<float>::max() || value < std::numeric_limits<float>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:g}) for {:s}", value, varname);
					}
					return CellData(value);
				},
				[row, &varname, ver](const std::vector<double>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<double>::max() || value < std::numeric_limits<double>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:g}) for {:s}", value, varname);
					}
					return CellData(value);
				},
				[row, &varname, ver](const std::vector<std::string>& data) {
					const auto& value = data[row - 1];
					return CellData(value);
				},
				[row, &varname, ver](const std::vector<strL>& data) {
					const auto& value = data[row - 1];
					return std::visit(overloaded{
						[](const std::string& val) {
							return CellData(val);
						},
						[](const std::vector<char>& val) {
							return CellData(val);
						}
					}, value);
				}
				}, val);
		}
		return CellData(std::string(""));
	}

	template <typename T>
	[[nodiscard]] inline T DTAFile::GetValue(std::string_view varname, std::uint64_t row) const {
		if (row < 1 || row > nobs) {
			std::println(std::cerr, "{:d} out of range", row);
			throw std::out_of_range("Observation out of range");
		}
		auto ver = fileversion;
		auto index = variableIndex(varname);
		if (index < datacolumns.size()) {
			if constexpr (std::is_integral_v<T>) {
				return std::visit(overloaded{
					[row, &varname, ver](const std::vector<std::int8_t>& data) {
						auto value = data[row - 1];
						if (isMissing(value, ver) == false && (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
						}
						return T(value);
					},
					[row, &varname, ver](const std::vector<std::int16_t>& data) {
						auto value = data[row - 1];
						if (isMissing(value, ver) == false && (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
						}
						return T(value);
					},
					[row, &varname, ver](const std::vector<std::int32_t>& data) {
						auto value = data[row - 1];
						if (isMissing(value, ver) == false && (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
						}
						return T(value);
					},
					[](const auto &data) {
						std::println(std::cerr, "Unsupported integer type");
						throw std::runtime_error("Unsuported type");
						return T();
					}
				}, datacolumns[index]);
			}

			if constexpr (std::is_floating_point_v<T>) {
				return std::visit(overloaded{
					[row, &varname, ver](const std::vector<float>& data) {
						auto value = data[row - 1];
						if (isMissing(value, ver) == false && (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
						}
						return T(value);
					},
					[row, &varname, ver](const std::vector<double>& data) {
						auto value = data[row - 1];
						if (isMissing(value, ver) == false && (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
						}
						return T(value);
					},
					[](const auto& data) {
						std::println(std::cerr, "Unsupported float type");
						throw std::runtime_error("Unsuported type");
						return T();
					}
				}, datacolumns[index]);
			}

			if constexpr (std::is_same_v<T, std::string>) {
				if (std::holds_alternative<std::vector<std::string> >(datacolumns[index])) {
					const auto& column = std::get<std::vector<std::string> >(datacolumns[index]);
					return column[row - 1];
				}
				else {
					std::println(std::cerr, "Not a string variable");
					throw std::runtime_error("Unsuported type");
					return T();
				}
			}

			if constexpr (std::is_same_v<T, strL>) {
				if (std::holds_alternative<std::vector<strL> >(datacolumns[row])) {
					const auto& column = std::get<std::vector<strL> >(datacolumns[index]);
					return column[row - 1];
				}
				else {
					std::println(std::cerr, "Not a strL variable");
					throw std::runtime_error("Unsuported type");
					return T();
				}
			}
		}
		return T();
	}

	[[nodiscard]] inline std::string DTAFile::GetValueString(std::string_view varname, std::uint64_t row) const {
		if (row < 1 || row > nobs) {
			std::println(std::cerr, "{:d} out of range", row);
			throw std::out_of_range("Observation out of range");
		}
		auto ver = fileversion;
		auto index = variableIndex(varname);
		if (index < datacolumns.size()) {
			const auto &val = datacolumns[index];
			return std::visit(overloaded{
				[](std::monostate) {
					return std::string("");
				},
				[row, &varname, ver](const std::vector<std::int8_t>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<std::int8_t>::max() || value < std::numeric_limits<int8_t>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
					}
					if (isMissing(value, ver)) {
						return std::string(".");
					}
					else {
						return std::to_string(value);
					}
				},
				[row, &varname, ver](const std::vector<std::int16_t>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<std::int16_t>::max() || value < std::numeric_limits<std::int16_t>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
					}
					if (isMissing(value, ver)) {
						return std::string(".");
					}
					else {
						return std::to_string(value);
					}
				},
				[row, &varname, ver](const std::vector<std::int32_t>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<std::int32_t>::max() || value < std::numeric_limits<std::int32_t>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
					}
					if (isMissing(value, ver)) {
						return std::string(".");
					}
					else {
						return std::to_string(value);
					}
				},
				[row, &varname, ver](const std::vector<float>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<float>::max() || value < std::numeric_limits<float>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:g}) for {:s}", value, varname);
					}
					if (isMissing(value, ver)) {
						return std::string(".");
					}
					else {
						return std::to_string(value);
					}
				},
				[row, &varname, ver](const std::vector<double>& data) {
					auto value = data[row - 1];
					if (isMissing(value, ver) == false && (value > std::numeric_limits<double>::max() || value < std::numeric_limits<double>::lowest())) {
						std::println(std::cerr, "WARNING: Unexpected value ({:g}) for {:s}", value, varname);
					}
					if (isMissing(value, ver)) {
						return std::string(".");
					}
					else {
						return std::to_string(value);
					}
				},
				[row, &varname, ver](const std::vector<std::string>& data) {
					const auto &value = data[row - 1];
					return std::string(value);
				},
				[row, &varname, ver](const std::vector<strL>& data) {
					const auto &value = data[row - 1];
					return std::visit(overloaded{
						[](const std::string& val) {
							return val;
						},
						[](const std::vector<char>& val) {
							return std::string(val.data());
						}
					}, value);
				} 
			}, val);
		}
		return "";
	}

	template <typename T>
	inline bool DTAFile::SetValue(std::string_view varname, std::uint64_t row, T value) {
		if (row < 1 || row > nobs) {
			std::println(std::cerr, "{:d} is greater than the number of observations (:d)", row, nobs);
			return false;
		}
		auto ver = fileversion;
		auto index = variableIndex(varname);
		if (index < datacolumns.size()) {
			if constexpr (std::is_integral_v<T>) {
				return std::visit(overloaded{
					[row, value, &varname, ver](std::vector<std::int8_t>& data) {
						if (isMissing(value, ver) == false && (static_cast<std::int8_t>(value) > std::numeric_limits<std::int8_t>::max() || static_cast<std::int8_t>(value) < std::numeric_limits<std::int8_t>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
						}
						data[row - 1] = std::uint8_t(value);
						return true;
					},
					[row, value, &varname, ver](std::vector<std::int16_t>& data) {
						if (isMissing(value, ver) == false && (static_cast<std::int16_t>(value) > std::numeric_limits<std::int16_t>::max() || static_cast<std::int16_t>(value) < std::numeric_limits<std::int16_t>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
						}
						data[row - 1] = std::uint16_t(value);
						return true;
					},
					[row, value, &varname, ver](std::vector<std::int32_t>& data) {
						if (isMissing(value, ver) == false && (static_cast<std::int32_t>(value) > std::numeric_limits<std::int32_t>::max() || static_cast<std::int32_t>(value) < std::numeric_limits<std::int32_t>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:d}) for {:s}", value, varname);
						}
						data[row - 1] = std::uint32_t(value);
						return true;
					},
					[](auto& data) {
						std::println(std::cerr, "Unsupported integer type");
						return false;
					}
					}, datacolumns[index]);
			}

			if constexpr (std::is_floating_point_v<T>) {
				return std::visit(overloaded{
					[row, value, &varname, ver](std::vector<float>& data) {
						if (isMissing(value, ver) == false && (static_cast<float>(value) > std::numeric_limits<float>::max() || static_cast<float>(value) < std::numeric_limits<float>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:g}) for {:s}", value, varname);
						}
						data[row - 1] = float(value);
						return true;
					},
					[row, value, &varname, ver](std::vector<double>& data) {
						if (isMissing(value, ver) == false && (static_cast<double>(value) > std::numeric_limits<double>::max() || static_cast<double>(value) < std::numeric_limits<double>::lowest())) {
							std::println(std::cerr, "WARNING: Unexpected value ({:g}) for {:s}", value, varname);
						}
						data[row - 1] = double(value);
						return true;
					},
					[](auto& data) {
						std::println(std::cerr, "Unsupported floating point type");
						return false;
					}
					}, datacolumns[index]);
			}

			if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> || std::is_same_v<T, const char*>) {
				return std::visit(overloaded{
					[&](std::vector<std::string>& data) {
						if (strlens[index] < std::string_view(value).length()) {
							std::println(std::cerr, "{:s} too large to fit into str{:d} variable", value, strlens[index]);
							return false;
						}
						data[row - 1] = value;
						return true;
					},
					[row, value](std::vector<strL>& data) {
						data[row - 1] = value;
						return true;
					},
					[](auto& data) {
						std::println(std::cerr, "Unsupported string type");
						return false;
					}
					}, datacolumns[index]);
			}

			if constexpr (std::is_same_v<T, strL>) {
				if (std::holds_alternative<std::vector<strL> >(datacolumns[index]) == true) {
					auto& column = std::get<std::vector<strL> >(datacolumns[index]);
					column[row - 1] = value;
					return true;
				}
				else {
					std::println(std::cerr, "Unsupported strL type");
					return false;
				}
			}

			if constexpr (std::is_same_v<T, std::monostate>) {
				std::println(std::cerr, "Cannot assign values to alias variables");
				return false;
			}

			return false;
		}
		else {
			std::println(std::cerr, "Invalid column");
			return false;
		}
	}

	inline std::uint32_t DTAFile::variableIndex(std::string_view varname) const{
		for (std::uint32_t i = 0; i < nvar; ++i) {
			if (varlist[i] == varname) {
				return i;
			}
		}
		throw std::out_of_range("Column not found");
	}
}
}

#endif 