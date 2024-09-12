#include <iostream>

#include "headers.h"

bool fileExists(std::string path)
{
	return false;
}

uint16_t getRamLimit()
{
	return 0;
}

uint16_t getRamUsage()
{
	return 0;
}

uint64_t getStorageLimit()
{
	return 0;
}

uint64_t getStorageUsage()
{
	return 0;
}

Archive::Archive(std::string name, std::string path)
	: pName(name), pPath(path)
{
	std::string fullPath = path + "/" + name + ".tea";
	if (fileExists(fullPath))
		pHeader.load(path);
	else
	{
		pHeader.size = 25; 
		pHeader.compression = false;
		pHeader.encryption = false;
		pHeader.endianess = false;
		pHeader.storageMode = 3;
		pHeader.compressionLevel = 0;
		pHeader.compressionMethod = 0;
		pHeader.encryptionMethod = 0;
		pHeader.numDirectories = 0;
		pHeader.numFiles = 0;
		pHeader.totalSize = 0;
		pHeader.creationTime = 0;
	}

	pCurrentDirectoryId = 0;
	pCurrentFileId = 0;

	pRamLimit = getRamLimit();
	pRamUsage = getRamUsage();

	pStorageLimit = getStorageLimit();
	pStorageUsage = getStorageUsage();
}

Archive::~Archive()
{
}

void Archive::save()
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
	return 0;
}

uint16_t Archive::getRamUsage()
{
	return 0;
}

uint64_t Archive::getStorageLimit()
{
	return 0;
}

uint64_t Archive::getStorageUsage()
{
	return 0;
}

std::string Archive::getName()
{
	return pName;
}

std::string Archive::getPath()
{
	return pPath;
}
