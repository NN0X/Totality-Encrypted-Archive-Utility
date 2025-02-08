#ifndef MOVEINPLACE_H
#define MOVEINPLACE_H

#include <fstream>
#include <string>
#include <cstdint>

bool moveFileInPlace(std::fstream &original, std::ofstream &target, const std::string &pathOriginal, uint64_t size, uint64_t chunk);

#endif // MOVEINPLACE_H
