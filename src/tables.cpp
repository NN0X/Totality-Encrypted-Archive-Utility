#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>

#include "headers.h"
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

void FileTable::addFile(std::string path, uint64_t &id)
{
	FileHeader fileHeader;
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Error: could not open file " << path << std::endl;
		return;
	}
	id++;
	switch (pStorageMode)
	{
		case 0:
			fileHeader.pSize = 14;
			break;
		case 1:
			fileHeader.pSize = 18;
			break;
		case 2:
			fileHeader.pSize = 27;
			break;
		case 3:
			fileHeader.pSize = 35;
			break;
	}
	fileHeader.pName = path.substr(path.find_last_of("/")+1);
	fileHeader.pNameLength = fileHeader.pName.size();
	fileHeader.pSize += fileHeader.pNameLength;
	file.seekg(0, std::ios::end);
	fileHeader.pFileSize = file.tellg();
	fileHeader.pCreationTime = 0;	// TODO: get epoch time of creation of this file (or last modification)
	fileHeader.pID = id;

	pLoadedFileIDs.push_back(id);
	pLoadedFileHeaders[id] = fileHeader;
}

