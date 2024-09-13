#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

#include "tables.h"

void DirectoryTable::create()
{
	size = 0;
	loadedDirectoryIDs = std::vector<uint32_t>();
	loadedDirectoryHeaders = std::unordered_map<uint32_t, DirectoryHeader>();
}

void DirectoryTable::load(std::string path, uint32_t idBegin, int count)
{
	// Load directory headers from file
}

void FileTable::create()
{
	size = 0;
	loadedFileIDs = std::vector<uint32_t>();
	loadedFileHeaders = std::unordered_map<uint32_t, FileHeader>();
}

void FileTable::load(std::string path, uint32_t idBegin, int count)
{
	// Load file headers from file
}

