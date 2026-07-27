#include "DTAFile.hpp"

#include <cstdint>
#include <print>
#include <string>
#include <string_view>
#include <vector>

// Always prints underlying values, rather than applying value labels
// Doesn't take locale into account
// Missing data is printed as a dot, NA might be more appropriate

// Write CSV data representation of DTA file to standard output
static void CSV(const dta::DTAFile& dtafile) {
	std::uint64_t obs = dtafile.GetNumObservations();
	std::vector<std::string> varnames = dtafile.GetVariableNames();

	for (const auto &name : varnames) {
		if (name.contains(",")) {
			std::print("\"{}\"", name);
		}
		else {
			std::print("{}", name);
		}
		if (&name != &varnames.back()) {
			std::print(", ");
		}
		else {
			std::println("");
		}
	}

	for (std::uint64_t i = 1; i <= obs; ++i) {
		for (const auto& name : varnames) {
			std::string val = dtafile.GetValueString(name, i);
			if (val.contains(",")) {
				std::print("\"{:}\"", val);
			}
			else {
				std::print("{:}", val);
			}
			if (&name != &varnames.back()) {
				std::print(", ");
			}
			else {
				std::println("");
			}
		}
	}
}

// Read DTA file and output CSV representation to standard output 
int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::println("Usage: dta2csv <filename>");
		return 1;
	}
	std::string_view filename(argv[1]);
	dta::DTAFile dtafile(filename);
	CSV(dtafile);

	return 0;
}