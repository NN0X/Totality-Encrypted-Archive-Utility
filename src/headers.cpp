#include <fstream>
#include <iostream>
#include <vector>

#include "headers.h"

//---------------------
// HEADER
// 1 bytes - header size in bytes (including this field)
//
// 1 bytes - flags (compression, encryption, 2 bits for storage mode, 2 bits for compression level, 2 bits reserved)
//	storage mode (size of headers):
//		0 - very low <- this is for extreme cases 
//		1 - low 
//		2 - medium 
//		3 - high <- this is the default
//	
//	compression level:
//		0 - very low
//		1 - low
//		2 - medium <- this is the default
//		3 - high
//
// (0, 1, 2, 3) bytes - number of directories
// (1, 2, 3, 4) bytes - number of files
// (0, 0, 6, 8) bytes - total size of all files (without header, without file and directory tables) before compression in bytes
// (0, 0, 4, 8) bytes - epoch time of creation of this archive 
//
// if compression flag is set:
// 	1 bytes - compression method
// 
// if encryption flag is set:
// 	1 bytes - encryption method
//

void Header::create()
{
	size = 25; // size of header in high storage mode with no compression and no encryption
	compression = 0;
	encryption = 0;
	storageMode = 3;
	compressionLevel = 0;
	numDirectories = 0;
	numFiles = 0;
	totalSize = 0;
	creationTime = 0;
	compressionMethod = 0;
	encryptionMethod = 0;
}

void Header::load(std::string path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Error: could not open file " << path << std::endl;
		return;
	}

	file.read(reinterpret_cast<char*>(&size), 1);

	uint8_t flags;
	file.read(reinterpret_cast<char*>(&flags), 1);
	compression = flags & 0b10000000;
	encryption = flags & 0b01000000;
	storageMode = flags & 0b00110000;
	compressionLevel = flags & 0b00001100;

	switch (storageMode)
	{
	case 0:
		numDirectories = 0;
		totalSize = 0;
		creationTime = 0;
		file.read(reinterpret_cast<char*>(&numFiles), 1);
		break;
	case 1:
		totalSize = 0;
		creationTime = 0;
		file.read(reinterpret_cast<char*>(&numDirectories), 1);
		file.read(reinterpret_cast<char*>(&numFiles), 2);
		break;
	case 2:
		file.read(reinterpret_cast<char*>(&numDirectories), 2);
		file.read(reinterpret_cast<char*>(&numFiles), 3);
		file.read(reinterpret_cast<char*>(&totalSize), 6);
		file.read(reinterpret_cast<char*>(&creationTime), 4);
		break;
	case 3:
		file.read(reinterpret_cast<char*>(&numDirectories), 3);
		file.read(reinterpret_cast<char*>(&numFiles), 4);
		file.read(reinterpret_cast<char*>(&totalSize), 8);
		file.read(reinterpret_cast<char*>(&creationTime), 8);
		break;
	}

	if (compression)
		file.read(reinterpret_cast<char*>(&compressionMethod), 1);
	if (encryption)
		file.read(reinterpret_cast<char*>(&encryptionMethod), 1);
}

void Header::save(std::string path)
{
	std::vector<char> data;
	data.reserve(size);
	data.push_back(size);
	char flags = 0;
	flags |= compression << 7;
	flags |= encryption << 6;
	flags |= storageMode << 4;
	flags |= compressionLevel << 2;
	data.push_back(flags);
	

	char numDirectoriesBytes[3];
	char numFilesBytes[4];
	char totalSizeBytes[8];
	char creationTimeBytes[8];

	switch (storageMode)
	{
	case 0:
		data.push_back(numFiles);
		break;
	case 1:
		data.push_back(numDirectories);
		numFilesBytes[0] = numFiles & 0b11111111;
		numFilesBytes[1] = (numFiles >> 8) & 0b11111111; 
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		break;
	case 2:
		numDirectoriesBytes[0] = numDirectories & 0b11111111;
		numDirectoriesBytes[1] = (numDirectories >> 8) & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);
		data.push_back(numDirectoriesBytes[1]);

		numFilesBytes[0] = numFiles & 0b11111111;
		numFilesBytes[1] = (numFiles >> 8) & 0b11111111;
		numFilesBytes[2] = (numFiles >> 16) & 0b11111111;
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		data.push_back(numFilesBytes[2]);

		totalSizeBytes[0] = totalSize & 0b11111111;
		totalSizeBytes[1] = (totalSize >> 8) & 0b11111111;
		totalSizeBytes[2] = (totalSize >> 16) & 0b11111111;
		totalSizeBytes[3] = (totalSize >> 24) & 0b11111111;
		totalSizeBytes[4] = (totalSize >> 32) & 0b11111111;
		totalSizeBytes[5] = (totalSize >> 40) & 0b11111111;
		data.push_back(totalSizeBytes[0]);
		data.push_back(totalSizeBytes[1]);
		data.push_back(totalSizeBytes[2]);
		data.push_back(totalSizeBytes[3]);
		data.push_back(totalSizeBytes[4]);
		data.push_back(totalSizeBytes[5]);

		creationTimeBytes[0] = creationTime & 0b11111111;
		creationTimeBytes[1] = (creationTime >> 8) & 0b11111111;
		creationTimeBytes[2] = (creationTime >> 16) & 0b11111111;
		creationTimeBytes[3] = (creationTime >> 24) & 0b11111111;
		data.push_back(creationTimeBytes[0]);
		data.push_back(creationTimeBytes[1]);
		data.push_back(creationTimeBytes[2]);
		data.push_back(creationTimeBytes[3]);
		break;
	case 3:
		numDirectoriesBytes[0] = numDirectories & 0b11111111;
		numDirectoriesBytes[1] = (numDirectories >> 8) & 0b11111111;
		numDirectoriesBytes[2] = (numDirectories >> 16) & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);
		data.push_back(numDirectoriesBytes[1]);
		data.push_back(numDirectoriesBytes[2]);

		numFilesBytes[0] = numFiles & 0b11111111;
		numFilesBytes[1] = (numFiles >> 8) & 0b11111111;
		numFilesBytes[2] = (numFiles >> 16) & 0b11111111;
		numFilesBytes[3] = (numFiles >> 24) & 0b11111111;
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		data.push_back(numFilesBytes[2]);
		data.push_back(numFilesBytes[3]);
	
		totalSizeBytes[0] = totalSize & 0b11111111;
		totalSizeBytes[1] = (totalSize >> 8) & 0b11111111;
		totalSizeBytes[2] = (totalSize >> 16) & 0b11111111;
		totalSizeBytes[3] = (totalSize >> 24) & 0b11111111;
		totalSizeBytes[4] = (totalSize >> 32) & 0b11111111;
		totalSizeBytes[5] = (totalSize >> 40) & 0b11111111;
		totalSizeBytes[6] = (totalSize >> 48) & 0b11111111;
		totalSizeBytes[7] = (totalSize >> 56) & 0b11111111;
		data.push_back(totalSizeBytes[0]);
		data.push_back(totalSizeBytes[1]);
		data.push_back(totalSizeBytes[2]);
		data.push_back(totalSizeBytes[3]);
		data.push_back(totalSizeBytes[4]);
		data.push_back(totalSizeBytes[5]);
		data.push_back(totalSizeBytes[6]);
		data.push_back(totalSizeBytes[7]);

		creationTimeBytes[0] = creationTime & 0b11111111;
		creationTimeBytes[1] = (creationTime >> 8) & 0b11111111;	
		creationTimeBytes[2] = (creationTime >> 16) & 0b11111111;
		creationTimeBytes[3] = (creationTime >> 24) & 0b11111111;
		creationTimeBytes[4] = (creationTime >> 32) & 0b11111111;
		creationTimeBytes[5] = (creationTime >> 40) & 0b11111111;
		creationTimeBytes[6] = (creationTime >> 48) & 0b11111111;
		creationTimeBytes[7] = (creationTime >> 56) & 0b11111111;
		data.push_back(creationTimeBytes[0]);
		data.push_back(creationTimeBytes[1]);
		data.push_back(creationTimeBytes[2]);
		data.push_back(creationTimeBytes[3]);
		data.push_back(creationTimeBytes[4]);
		data.push_back(creationTimeBytes[5]);
		data.push_back(creationTimeBytes[6]);
		data.push_back(creationTimeBytes[7]);
		break;
	}

	if (compression)
		data.push_back(compressionMethod);
	if (encryption)
		data.push_back(encryptionMethod);

	if (data.size() != size)
	{
		std::cerr << "Error: header size mismatch" << std::endl;
		std::cerr << "Expected size: " << (unsigned int)size << std::endl;
		std::cerr << "Actual size: " << data.size() << std::endl;
		return;
	}

	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Error: could not open file " << path << std::endl;
		return;
	}
	file.write(data.data(), data.size());
	file.close();
}

//---------------------

void DirectoryHeader::create()
{
	encryption = false;
	size = 0;
	nameLength = 0;
	name = nullptr;
	numFiles = 0;
	fileIds = nullptr;
	numDirectories = 0;
	directoryIds = nullptr;
	creationTime = 0;
	saved = false;
}

void DirectoryHeader::save(std::string path, uint64_t offsetEnc, uint8_t encryptionBit,uint64_t offsetHead, uint8_t storageMode)
{	
	std::vector<char> data;
	data.reserve(size);

	char sizeBytes[5];
	char nameLengthBytes[2];
	char numFilesBytes[4];
	char numDirectoriesBytes[4];
	char fileIdBytes[4];
	char directoryIdBytes[3];
	char creationTimeBytes[8];

	switch (storageMode)
	{
	case 1:
		sizeBytes[0] = size & 0b11111111;
		sizeBytes[1] = (size >> 8) & 0b11111111;
		data.push_back(sizeBytes[0]);
		data.push_back(sizeBytes[1]);

		nameLengthBytes[0] = nameLength & 0b11111111;
		data.push_back(nameLengthBytes[0]);
		
		for (int i = 0; i < nameLength; i++)
		{
			data.push_back(name[i]);
		}

		numFilesBytes[0] = numFiles & 0b11111111;
		data.push_back(numFilesBytes[0]);

		numDirectoriesBytes[0] = numDirectories & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);

		for (int i = 0; i < numFiles; i++)
		{
			fileIdBytes[0] = fileIds[i] & 0b11111111;
			fileIdBytes[1] = (fileIds[i] >> 8) & 0b11111111;
			data.push_back(fileIdBytes[0]);
			data.push_back(fileIdBytes[1]);
		}

		for (int i = 0; i < numDirectories; i++)
		{
			directoryIdBytes[0] = directoryIds[i] & 0b11111111;
			data.push_back(directoryIdBytes[0]);
		}
		break;
	case 2:
		sizeBytes[0] = size & 0b11111111;
		sizeBytes[1] = (size >> 8) & 0b11111111;
		sizeBytes[2] = (size >> 16) & 0b11111111;
		sizeBytes[3] = (size >> 24) & 0b11111111;
		data.push_back(sizeBytes[0]);
		data.push_back(sizeBytes[1]);
		data.push_back(sizeBytes[2]);
		data.push_back(sizeBytes[3]);

		nameLengthBytes[0] = nameLength & 0b11111111;
		nameLengthBytes[1] = (nameLength >> 8) & 0b11111111;
		data.push_back(nameLengthBytes[0]);
		data.push_back(nameLengthBytes[1]);

		for (int i = 0; i < nameLength; i++)
		{
			data.push_back(name[i]);
		}

		numFilesBytes[0] = numFiles & 0b11111111;
		numFilesBytes[1] = (numFiles >> 8) & 0b11111111;
		numFilesBytes[2] = (numFiles >> 16) & 0b11111111;
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		data.push_back(numFilesBytes[2]);

		numDirectoriesBytes[0] = numDirectories & 0b11111111;
		numDirectoriesBytes[1] = (numDirectories >> 8) & 0b11111111;
		numDirectoriesBytes[2] = (numDirectories >> 16) & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);
		data.push_back(numDirectoriesBytes[1]);
		data.push_back(numDirectoriesBytes[2]);

		for (int i = 0; i < numFiles; i++)
		{
			fileIdBytes[0] = fileIds[i] & 0b11111111;
			fileIdBytes[1] = (fileIds[i] >> 8) & 0b11111111;
			fileIdBytes[2] = (fileIds[i] >> 16) & 0b11111111;
			data.push_back(fileIdBytes[0]);
			data.push_back(fileIdBytes[1]);
			data.push_back(fileIdBytes[2]);
		}

		for (int i = 0; i < numDirectories; i++)
		{
			directoryIdBytes[0] = directoryIds[i] & 0b11111111;
			directoryIdBytes[1] = (directoryIds[i] >> 8) & 0b11111111;
			data.push_back(directoryIdBytes[0]);
			data.push_back(directoryIdBytes[1]);
		}

		creationTimeBytes[0] = creationTime & 0b11111111;
		creationTimeBytes[1] = (creationTime >> 8) & 0b11111111;
		creationTimeBytes[2] = (creationTime >> 16) & 0b11111111;
		creationTimeBytes[3] = (creationTime >> 24) & 0b11111111;
		data.push_back(creationTimeBytes[0]);
		data.push_back(creationTimeBytes[1]);
		data.push_back(creationTimeBytes[2]);
		data.push_back(creationTimeBytes[3]);
		break;

	case 3:
		sizeBytes[0] = size & 0b11111111;
		sizeBytes[1] = (size >> 8) & 0b11111111;
		sizeBytes[2] = (size >> 16) & 0b11111111;
		sizeBytes[3] = (size >> 24) & 0b11111111;
		sizeBytes[4] = (size >> 32) & 0b11111111;
		data.push_back(sizeBytes[0]);
		data.push_back(sizeBytes[1]);
		data.push_back(sizeBytes[2]);
		data.push_back(sizeBytes[3]);
		data.push_back(sizeBytes[4]);

		nameLengthBytes[0] = nameLength & 0b11111111;
		nameLengthBytes[1] = (nameLength >> 8) & 0b11111111;
		data.push_back(nameLengthBytes[0]);
		data.push_back(nameLengthBytes[1]);

		for (int i = 0; i < nameLength; i++)
		{
			data.push_back(name[i]);
		}

		numFilesBytes[0] = numFiles & 0b11111111;
		numFilesBytes[1] = (numFiles >> 8) & 0b11111111;
		numFilesBytes[2] = (numFiles >> 16) & 0b11111111;
		numFilesBytes[3] = (numFiles >> 24) & 0b11111111;
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		data.push_back(numFilesBytes[2]);
		data.push_back(numFilesBytes[3]);

		numDirectoriesBytes[0] = numDirectories & 0b11111111;
		numDirectoriesBytes[1] = (numDirectories >> 8) & 0b11111111;
		numDirectoriesBytes[2] = (numDirectories >> 16) & 0b11111111;
		numDirectoriesBytes[3] = (numDirectories >> 24) & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);
		data.push_back(numDirectoriesBytes[1]);
		data.push_back(numDirectoriesBytes[2]);
		data.push_back(numDirectoriesBytes[3]);

		for (int i = 0; i < numFiles; i++)
		{
			fileIdBytes[0] = fileIds[i] & 0b11111111;
			fileIdBytes[1] = (fileIds[i] >> 8) & 0b11111111;
			fileIdBytes[2] = (fileIds[i] >> 16) & 0b11111111;
			fileIdBytes[3] = (fileIds[i] >> 24) & 0b11111111;
			data.push_back(fileIdBytes[0]);
			data.push_back(fileIdBytes[1]);
			data.push_back(fileIdBytes[2]);
			data.push_back(fileIdBytes[3]);
		}

		for (int i = 0; i < numDirectories; i++)
		{
			directoryIdBytes[0] = directoryIds[i] & 0b11111111;
			directoryIdBytes[1] = (directoryIds[i] >> 8) & 0b11111111;
			directoryIdBytes[2] = (directoryIds[i] >> 16) & 0b11111111;
			data.push_back(directoryIdBytes[0]);
			data.push_back(directoryIdBytes[1]);
			data.push_back(directoryIdBytes[2]);
		}

		creationTimeBytes[0] = creationTime & 0b11111111;
		creationTimeBytes[1] = (creationTime >> 8) & 0b11111111;
		creationTimeBytes[2] = (creationTime >> 16) & 0b11111111;
		creationTimeBytes[3] = (creationTime >> 24) & 0b11111111;
		creationTimeBytes[4] = (creationTime >> 32) & 0b11111111;
		creationTimeBytes[5] = (creationTime >> 40) & 0b11111111;
		creationTimeBytes[6] = (creationTime >> 48) & 0b11111111;
		creationTimeBytes[7] = (creationTime >> 56) & 0b11111111;
		data.push_back(creationTimeBytes[0]);
		data.push_back(creationTimeBytes[1]);
		data.push_back(creationTimeBytes[2]);
		data.push_back(creationTimeBytes[3]);
		data.push_back(creationTimeBytes[4]);
		data.push_back(creationTimeBytes[5]);
		data.push_back(creationTimeBytes[6]);
		data.push_back(creationTimeBytes[7]);
		break;
	}

	std::ifstream fileRead(path, std::ios::binary);
	fileRead.seekg(offsetEnc);
	char encryptionByte = 0;
	fileRead.read(&encryptionByte, 1);
	fileRead.close();
	
	char mask = encryption << encryptionBit;
	encryptionByte |= mask;

	std::ofstream fileWrite(path, std::ios::binary);
	fileWrite.seekp(offsetEnc);
	fileWrite.write(&encryptionByte, 1);
	fileWrite.seekp(offsetHead);
	fileWrite.write(data.data(), data.size());
	fileWrite.close();

	saved = true;
}

void DirectoryHeader::remove()
{
}

//---------------------

void FileHeader::create()
{
}

void FileHeader::save(std::string path, uint64_t offset)
{
}

void FileHeader::remove()
{
}
