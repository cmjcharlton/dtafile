#include "DTAFile.hpp"

#include <bit>
#include <print>
#include <string>
#include <string_view>

static std::string to_string(std::endian order) {
	switch (order) {
	case std::endian::little:
		return "LOHI";
	case std::endian::big:
		return "HILO";
	}
	return "????";
}

// Convert input Stata .dta file to chosen output version and optionally byteorder
int main(int argc, char* argv[]) {
	if (argc != 4 && argc != 5) {
		std::println("Usage: convert <input filename> <output filename> <version> [<byteorder>]");
		return 1;
	}

	std::string_view infilename = argv[1];
	std::string_view outfilename = argv[2];
	int version = std::stoi(argv[3]);
	std::endian byteorder = std::endian::native;
	if (argc == 5) {
		if (std::string_view(argv[4]) == "HILO") {
			byteorder = std::endian::big;
		}
		else if (std::string_view(argv[4]) == "LOHI") {
			byteorder = std::endian::little;
		}
		else {
			std::println("Invalid byte order specified");
			return 1;
		}
	}

	dta::DTAFile dtafile(infilename);
	std::println("Input file: {} - original file format: {} ({})", infilename, dtafile.GetOriginalFileVersion(), to_string(dtafile.GetOriginalByteOrder()));
	std::println("Saving {}  - new file format: {} ({}) format", outfilename, version, to_string(byteorder));
	dtafile.Save(outfilename, version, byteorder);
	return 0;
}