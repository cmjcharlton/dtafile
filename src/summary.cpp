#include "DTAFile.hpp"
#include "utility.hpp"

#include <print>
#include <string_view>

// Writes summary information for DTA file to standard output
int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::println("Usage: summary <filename>");
		return 1;
	}
	std::string_view filename(argv[1]);
	dta::DTAFile dtafile(filename);
	Description(dtafile);
	return 0;
}