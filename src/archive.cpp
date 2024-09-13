#include <iostream>
#include <fstream>

#include "archive.h"
#include "headers.h"

bool fileExists(std::string path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}
	return true;
}

Archive::Archive(std::string name, std::string path)
	: pName(name), pPath(path)
{
	std::string fullPath = path + "/" + name + ".tea";
	bool exists = fileExists(fullPath);
	if (exists)
	{
		pHeader.load(path);
		std::cout << "Archive loaded successfully!" << std::endl;
	}
	else
	{
		pHeader.create();
		pHeader.save(fullPath);
		std::cout << "Archive created successfully!" << std::endl;
	}	

	pCurrentDirectoryId = 0;
	pCurrentFileId = 0;

	pRamLimit = 4096; // 4 GiB
	pStorageLimit = 1024*1024*50; // 50 GiB
	
	pLargestDirectoryID = 0;
	pLargestFileID = 0;
}

Archive::~Archive()
{
}

void Archive::printCurrentFile()
{
}

void Archive::printCurrentDirectory()
{
}

void Archive::printFilesTree()
{
}

void Archive::printDirectoriesTree()
{
}

void Archive::addFile(std::string filePath)
{
}

void Archive::removeFile(std::string filePath)
{
}

void Archive::addDirectory(std::string directoryPath)
{
}

void Archive::removeDirectory(std::string directoryPath)
{
}

void Archive::setFile(std::string fileName)
{
}

void Archive::setDirectory(std::string directoryName)
{
}

void Archive::extractFile(std::string filePath)
{
}

void Archive::extractDirectory(std::string directoryPath)
{
}

void Archive::extractAll(std::string path)
{
}

void Archive::compressHUFFMAN()
{
}

void Archive::compressDEFLATE()
{
}

void Archive::decompress()
{
}

void Archive::encryptAES()
{
}

void Archive::encryptBRUTUS()
{
}

void Archive::encryptDirectoryAES()
{
}

void Archive::encryptDirectoryBRUTUS()
{
}

void Archive::decrypt()
{
}

void Archive::decryptDirectory()
{
}

uint16_t Archive::getRamLimit()
{
	return pRamLimit;
}

uint64_t Archive::getStorageLimit()
{
	return pStorageLimit;
}

std::string Archive::getName()
{
	return pName;
}

std::string Archive::getPath()
{
	return pPath;
}
