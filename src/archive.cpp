#include <iostream>
#include <fstream>
#include <unistd.h>

#include "archive.h"
#include "headers.h"

std::string getExecutablePathLinux()
{
	char result[255];
	ssize_t count = readlink("/proc/self/exe", result, 255);
	std::string path = std::string(result, (count > 0) ? count : 0);
	path = path.substr(0, path.find_last_of("/"));
	return path;
}

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
	: pName(name)
{
	pPath = getExecutablePathLinux() + "/" + path;

	pFullPath = pPath + "/" + name + ".tea";
	bool exists = fileExists(pFullPath);
	if (exists)
	{
		pHeader.load(pFullPath);
		pDirectoryTable.load(pFullPath, 0, pHeader.numDirectories);
		pFileTable.load(pFullPath, 0, pHeader.numFiles);
	}
	else
	{
		pHeader.create();
		pDirectoryTable.create();
		pFileTable.create();
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
	pHeader.save(pFullPath);
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
