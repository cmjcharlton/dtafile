#include "DTAFile.hpp"
#include "utility.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <print>
#include <string>
#include <variant>
#include <vector>

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// Prints top N lines to standard output
void Head(const dta::DTAFile& dtafile, std::uint64_t numlines) {
	std::uint64_t obs = dtafile.GetNumObservations();
	numlines = std::min(numlines, obs);
	std::uint32_t vars = dtafile.GetNumVariables();
	std::vector<std::string> varnames(vars);
	for (std::uint32_t j = 0; j < vars; ++j) {
		varnames[j] = dtafile.GetVariableName(j + 1);
	}
	for (std::uint64_t i = 1; i <= numlines; ++i) {
		for (const auto& name : varnames) {
			std::print("{:}", dtafile.GetValueString(name, i));
			if (&name != &varnames.back()) {
				std::print(", ");
			}
			else {
				std::println("");
			}
		}
	}
}

// Prints last N lines to standard output
void Tail(const dta::DTAFile& dtafile, std::uint64_t numlines) {
	std::uint64_t obs = dtafile.GetNumObservations();
	numlines = std::min(numlines, obs);
	std::uint32_t vars = dtafile.GetNumVariables();
	std::vector<std::string> varnames(vars);
	for (std::uint32_t j = 0; j < vars; ++j) {
		varnames[j] = dtafile.GetVariableName(j + 1);
	}

	for (std::uint64_t i = (obs + 1) - numlines; i <= obs; ++i) {
		for (const auto& name : varnames) {
			std::print("{:}", dtafile.GetValueString(name, i));
			if (&name != &varnames.back()) {
				std::print(", ");
			}
			else {
				std::println("");
			}
		}
	}
}

// Prints whole dataset to standard output
void Print(const dta::DTAFile& dtafile) {
	std::uint64_t nobs = dtafile.GetNumObservations();
	std::uint32_t nvar = dtafile.GetNumVariables();
	std::vector<std::string> varnames(nvar);
	for (std::uint32_t j = 0; j < nvar; ++j) {
		varnames[j] = dtafile.GetVariableName(j + 1);
	}

	for (std::uint64_t i = 1; i <= nobs; ++i) {
		for (const auto& name : varnames) {
			std::print("{:}", dtafile.GetValueString(name, i));
			if (&name != &varnames.back()) {
				std::print(", ");
			}
			else {
				std::println("");
			}
		}
	}
}

// Writes a summary table of DTA contents to standard output
void Summary(const dta::DTAFile& dtafile) {
	std::uint32_t nvar = dtafile.GetNumVariables();
	std::vector<std::string> varnames(nvar);
	for (std::uint32_t j = 0; j < nvar; ++j) {
		varnames[j] = dtafile.GetVariableName(j + 1);
	}
	std::string fmt = "|";
	for (const auto& name : varnames) {
		const dta::ColumnData& column = dtafile.GetVariable(name);
		fmt += std::visit(overloaded{
			[](std::monostate) -> std::string {
				return "a|";
			},
			[](const std::vector<std::int8_t>& data) -> std::string {
				return "b|";
			},
			[](const std::vector<std::int16_t>& data) -> std::string {
				return "i|";
			},
			[](const std::vector<std::int32_t>& data) -> std::string {
				return "l|";
			},
			[](const std::vector<float>& data) -> std::string {
				return "f|";
			},
			[](const std::vector<double>& data) -> std::string {
				return "d|";
			},
			[&, name](const std::vector<std::string>& data) -> std::string {
				return "s" + std::to_string(dtafile.GetMaxStringLength(name)) + "|";
			},
			[](const std::vector<dta::strL>& data) -> std::string {
				return "sL|";
			},
			}, column);
	}

	std::print("{:d}, ", dtafile.GetOriginalFileVersion());
	if (dtafile.GetOriginalByteOrder() == std::endian::little) {
		std::print("little, ");
	}
	if (dtafile.GetOriginalByteOrder() == std::endian::big) {
		std::print("big, ");
	}
	std::println("{:d}, {:d}, {:}, ", dtafile.GetNumVariables(), dtafile.GetNumObservations(), fmt);
	if (dtafile.GetValueLabelNames().empty() == false) {
		std::print("true, ");
	}
	else {
		std::print("false, ");
	}
	std::println("");
}

// Prints description of DTA file to standard output
void Description(const dta::DTAFile& dtafile) {
	std::println("Header Information:");
	if (dtafile.GetOriginalByteOrder() == std::endian::big) {
		std::println("Stata file format: {:d}, byte order: HILO", dtafile.GetOriginalFileVersion());
	}
	if (dtafile.GetOriginalByteOrder() == std::endian::little) {
		std::println("Stata file format: {:d}, byte order: LOHI", dtafile.GetOriginalFileVersion());
	}

	std::uint32_t nvar = dtafile.GetNumVariables();
	std::uint64_t nobs = dtafile.GetNumObservations();
	std::println("Number of variables: {:d}", nvar);
	std::println("Number of observations: {:d}", nobs);
	std::println("Dataset label: {}", dtafile.GetDataLabel());
	std::println("Timestamp: {}", dtafile.GetTimeStamp());

	std::vector<std::string> varnames(nvar);
	for (std::uint32_t j = 0; j < nvar; ++j) {
		varnames[j] = dtafile.GetVariableName(j + 1);
	}
	std::println("Variable\tType\tFormat\tValue Label\tVariable Label");

	for (const auto& name : varnames) {
		std::string type = dtafile.GetVariableType(name);
		std::string format = dtafile.GetFormat(name);
		std::string label = dtafile.GetVariableLabel(name);
		std::string varlab = dtafile.GetValueLabelName(name);
		std::println("{}\t{}\t{}\t{}\t{}", name, type, format, label, varlab);
	}
	std::println("");

	std::vector<std::string> labnames = dtafile.GetValueLabelNames();
	for (const auto& labname : labnames) {
		std::println("Value Label Table ({}):", labname);
		auto codes = dtafile.GetValueLabelCodes(labname);
		for (const auto& code : codes) {
			auto label = dtafile.GetValueLabelString(labname, code);
			std::println("{}: {}", code, label);
		}
		std::println("");
	}

	std::println("Variable Summaries:");
	for (const auto& name : varnames) {
		std::string type = dtafile.GetVariableType(name);
		std::uint8_t fileversion = dtafile.GetOriginalFileVersion();
		auto column = dtafile.GetVariable(name);
		std::visit(overloaded{
			[](std::monostate) {
				std::println("N/A");
			},
			[&](const std::vector<std::int8_t>& data) {
				dta::ColumnSummary<std::int8_t> summ(data, fileversion);
				std::println("Min: {:d} Max: {:d} Missing: {:d}", summ.min, summ.max, summ.nmissing);
			},
			[&](const std::vector<std::int16_t>& data) {
				dta::ColumnSummary<std::int16_t> summ(data, fileversion);
				std::println("Min: {:d} Max: {:d} Missing: {:d}", summ.min, summ.max, summ.nmissing);
			},
			[&](const std::vector<std::int32_t>& data) {
				dta::ColumnSummary<std::int32_t> summ(data, fileversion);
				std::println("Min: {:d} Max: {:d} Missing: {:d}", summ.min, summ.max, summ.nmissing);
			},
			[&](const std::vector<float>& data) {
				dta::ColumnSummary<float> summ(data, fileversion);
				std::println("Min: {:g} Max: {:g} Missing: {:d}", summ.min, summ.max, summ.nmissing);
			},
			[&](const std::vector<double>& data) {
				dta::ColumnSummary<double> summ(data, fileversion);
				std::println("Min: {:g} Max: {:g} Missing: {:d}", summ.min, summ.max, summ.nmissing);
			},
			[&](const std::vector<std::string>& data) {
				dta::ColumnSummary<std::string> summ(data, fileversion);
				std::println("Min: {:d} Max: {:d} Missing: {:d}", summ.min, summ.max, summ.nmissing);
			},
			[&](const std::vector<dta::strL>& data) {
				dta::ColumnSummary<dta::strL> summ(data, fileversion);
				std::println("Min: {:d} Max: {:d} Missing: {:d}", summ.min, summ.max, summ.nmissing);
			},
			}, column);
	}
	std::println("");

	std::println("Data sample:");
	Head(dtafile);
	if (nobs > 4) {
		if (nobs > 8) {
			std::println("...");
		}
		Tail(dtafile, std::min(std::uint64_t(4), nobs - 4));
	}
}