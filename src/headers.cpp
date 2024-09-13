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
