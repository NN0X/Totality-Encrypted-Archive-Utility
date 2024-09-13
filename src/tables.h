#ifndef TABLES_H
#define TABLES_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "headers.h"

struct DirectoryTable
{
	uint64_t size;

	std::vector<uint32_t> loadedTopLevelDirectoryIDs;
	std::vector<uint32_t> loadedSubDirectoryIDs;
	std::unordered_map<uint32_t, DirectoryHeader> loadedTopLevelDirectoryHeaders;
	std::unordered_map<uint32_t, DirectoryHeader> loadedSubDirectoryHeaders;

	void load(std::string path, uint32_t idBegin, int count);
	void save(std::string path);
};

struct FileTable
{
	uint64_t size;

	std::vector<uint32_t> loadedFileIDs;
	std::unordered_map<uint32_t, FileHeader> loadedFileHeaders;

	void load(std::string path, uint32_t idBegin, int count);
	void save(std::string path);
};

#endif // TABLES_H
