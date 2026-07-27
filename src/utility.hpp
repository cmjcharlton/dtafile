#ifndef UTILITY_H
#define UTILITY_H

#include "DTAFile.hpp"

#include <cstdint>

void Head(const dta::DTAFile& dtafile, std::uint64_t numlines = 4);
void Tail(const dta::DTAFile& dtafile, std::uint64_t numlines = 4);
void Print(const dta::DTAFile& dtafile);
void Summary(const dta::DTAFile& dtafile);
void Description(const dta::DTAFile& dtafile);

#endif