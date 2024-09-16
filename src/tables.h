#ifndef TABLES_H
#define TABLES_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "headers.h"

class DirectoryTable
{
public:
	DirectoryTable(uint8_t storageMode);

	uint64_t pSize;
	uint8_t pStorageMode;

	std::vector<uint32_t> loadedDirectoryIDs;
	std::unordered_map<uint32_t, DirectoryHeader> loadedDirectoryHeaders;

	void load(std::string path, uint32_t idBegin, int count);
	void unload(std::string path, uint32_t idBegin, int count);
};

class FileTable
{
public:
	FileTable(uint8_t storageMode);

	uint64_t pSize;
	uint8_t pStorageMode;

	std::vector<uint32_t> loadedFileIDs;
	std::unordered_map<uint32_t, FileHeader> loadedFileHeaders;
	
	void load(std::string path, uint32_t idBegin, int count);
	void unload(std::string path, uint32_t idBegin, int count);
	void addFile(std::string path, uint64_t &id);
};

#endif // TABLES_H
