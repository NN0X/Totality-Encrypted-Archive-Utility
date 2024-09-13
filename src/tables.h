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

	std::vector<uint32_t> loadedDirectoryIDs;
	std::unordered_map<uint32_t, DirectoryHeader> loadedDirectoryHeaders;

	void create();
	void load(std::string path, uint32_t idBegin, int count); 
	void unload(std::string path, uint32_t idBegin, int count);
};

struct FileTable
{
	uint64_t size;

	std::vector<uint32_t> loadedFileIDs;
	std::unordered_map<uint32_t, FileHeader> loadedFileHeaders;
	
	void create();
	void load(std::string path, uint32_t idBegin, int count);
	void unload(std::string path, uint32_t idBegin, int count);
};

#endif // TABLES_H
