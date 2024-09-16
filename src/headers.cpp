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

ArchiveHeader::ArchiveHeader() // temp
{
	pSize = 33; // size of header in high storage mode with no compression and no encryption
	pCompression = 0;
	pEncryption = 0;
	pStorageMode = 3;
	pCompressionLevel = 0;
	pNumDirectories = 0;
	pNumFiles = 0;
	pTotalSize = 0;
	pCreationTime = 0;
	pCompressionMethod = 0;
	pEncryptionMethod = 0;
}

void ArchiveHeader::load(std::string path)
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
	pCompression = flags & 0b10000000;
	pEncryption = flags & 0b01000000;
	pStorageMode = flags & 0b00110000;
	pCompressionLevel = flags & 0b00001100;

	switch (pStorageMode)
	{
	case 0:
		pNumDirectories = 0;
		pTotalSize = 0;
		pCreationTime = 0;
		file.read(reinterpret_cast<char*>(&pNumFiles), 1);
		break;
	case 1:
		pTotalSize = 0;
		pCreationTime = 0;
		file.read(reinterpret_cast<char*>(&pNumDirectories), 1);
		file.read(reinterpret_cast<char*>(&pNumFiles), 2);
		break;
	case 2:
		file.read(reinterpret_cast<char*>(&pNumDirectories), 2);
		file.read(reinterpret_cast<char*>(&pNumFiles), 3);
		file.read(reinterpret_cast<char*>(&pNotalSize), 6);
		file.read(reinterpret_cast<char*>(&pCreationTime), 4);
		file.read(reinterpret_cast<char*>(&pZeroedOutBytes), 4);
		break;
	case 3:
		file.read(reinterpret_cast<char*>(&pNumDirectories), 3);
		file.read(reinterpret_cast<char*>(&pNumFiles), 4);
		file.read(reinterpret_cast<char*>(&pTotalSize), 8);
		file.read(reinterpret_cast<char*>(&pCreationTime), 8);
		file.read(reinterpret_cast<char*>(&pZeroedOutBytes), 8);
		break;
	}

	if (pCompression)
		file.read(reinterpret_cast<char*>(&pCompressionMethod), 1);
	if (pEncryption)
		file.read(reinterpret_cast<char*>(&pEncryptionMethod), 1);
}

void ArchiveHeader::save(std::string path)
{
	std::vector<char> data;
	data.reserve(pSize);
	data.push_back(pSize);
	char flags = 0;
	flags |= pCompression << 7;
	flags |= pEncryption << 6;
	flags |= pStorageMode << 4;
	flags |= pCompressionLevel << 2;
	data.push_back(flags);
	

	char numDirectoriesBytes[3];
	char numFilesBytes[4];
	char totalSizeBytes[8];
	char creationTimeBytes[8];
	char numZeroedOutBytes[8];

	switch (pStorageMode)
	{
	case 0:
		data.push_back(pNumFiles);
		break;
	case 1:
		data.push_back(pNumDirectories);
		numFilesBytes[0] = pNumFiles & 0b11111111;
		numFilesBytes[1] = (pNumFiles >> 8) & 0b11111111; 
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		break;
	case 2:
		numDirectoriesBytes[0] = pNumDirectories & 0b11111111;
		numDirectoriesBytes[1] = (pNumDirectories >> 8) & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);
		data.push_back(numDirectoriesBytes[1]);

		numFilesBytes[0] = pNumFiles & 0b11111111;
		numFilesBytes[1] = (pNumFiles >> 8) & 0b11111111;
		numFilesBytes[2] = (pNumFiles >> 16) & 0b11111111;
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		data.push_back(numFilesBytes[2]);

		totalSizeBytes[0] = pTotalSize & 0b11111111;
		totalSizeBytes[1] = (pTotalSize >> 8) & 0b11111111;
		totalSizeBytes[2] = (pTotalSize >> 16) & 0b11111111;
		totalSizeBytes[3] = (pTotalSize >> 24) & 0b11111111;
		totalSizeBytes[4] = (pTotalSize >> 32) & 0b11111111;
		totalSizeBytes[5] = (pTotalSize >> 40) & 0b11111111;
		data.push_back(totalSizeBytes[0]);
		data.push_back(totalSizeBytes[1]);
		data.push_back(totalSizeBytes[2]);
		data.push_back(totalSizeBytes[3]);
		data.push_back(totalSizeBytes[4]);
		data.push_back(totalSizeBytes[5]);

		creationTimeBytes[0] = pCreationTime & 0b11111111;
		creationTimeBytes[1] = (pCreationTime >> 8) & 0b11111111;
		creationTimeBytes[2] = (pCreationTime >> 16) & 0b11111111;
		creationTimeBytes[3] = (pCreationTime >> 24) & 0b11111111;
		data.push_back(creationTimeBytes[0]);
		data.push_back(creationTimeBytes[1]);
		data.push_back(creationTimeBytes[2]);
		data.push_back(creationTimeBytes[3]);

		numZeroedOutBytes[0] = pZeroedOutBytes & 0b11111111;
		numZeroedOutBytes[1] = (pZeroedOutBytes >> 8) & 0b11111111;
		numZeroedOutBytes[2] = (pZeroedOutBytes >> 16) & 0b11111111;
		numZeroedOutBytes[3] = (pZeroedOutBytes >> 24) & 0b11111111;
		data.push_back(numZeroedOutBytes[0]);
		data.push_back(numZeroedOutBytes[1]);
		data.push_back(numZeroedOutBytes[2]);
		data.push_back(numZeroedOutBytes[3]);
		break;
	case 3:
		numDirectoriesBytes[0] = pNumDirectories & 0b11111111;
		numDirectoriesBytes[1] = (pNumDirectories >> 8) & 0b11111111;
		numDirectoriesBytes[2] = (pNumDirectories >> 16) & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);
		data.push_back(numDirectoriesBytes[1]);
		data.push_back(numDirectoriesBytes[2]);

		numFilesBytes[0] = pNumFiles & 0b11111111;
		numFilesBytes[1] = (pNumFiles >> 8) & 0b11111111;
		numFilesBytes[2] = (pNumFiles >> 16) & 0b11111111;
		numFilesBytes[3] = (pNumFiles >> 24) & 0b11111111;
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		data.push_back(numFilesBytes[2]);
		data.push_back(numFilesBytes[3]);
	
		totalSizeBytes[0] = pTotalSize & 0b11111111;
		totalSizeBytes[1] = (pTotalSize >> 8) & 0b11111111;
		totalSizeBytes[2] = (pTotalSize >> 16) & 0b11111111;
		totalSizeBytes[3] = (pTotalSize >> 24) & 0b11111111;
		totalSizeBytes[4] = (pTotalSize >> 32) & 0b11111111;
		totalSizeBytes[5] = (pTotalSize >> 40) & 0b11111111;
		totalSizeBytes[6] = (pTotalSize >> 48) & 0b11111111;
		totalSizeBytes[7] = (pTotalSize >> 56) & 0b11111111;
		data.push_back(totalSizeBytes[0]);
		data.push_back(totalSizeBytes[1]);
		data.push_back(totalSizeBytes[2]);
		data.push_back(totalSizeBytes[3]);
		data.push_back(totalSizeBytes[4]);
		data.push_back(totalSizeBytes[5]);
		data.push_back(totalSizeBytes[6]);
		data.push_back(totalSizeBytes[7]);

		creationTimeBytes[0] = pCreationTime & 0b11111111;
		creationTimeBytes[1] = (pCreationTime >> 8) & 0b11111111;	
		creationTimeBytes[2] = (pCreationTime >> 16) & 0b11111111;
		creationTimeBytes[3] = (pCreationTime >> 24) & 0b11111111;
		creationTimeBytes[4] = (pCreationTime >> 32) & 0b11111111;
		creationTimeBytes[5] = (pCreationTime >> 40) & 0b11111111;
		creationTimeBytes[6] = (pCreationTime >> 48) & 0b11111111;
		creationTimeBytes[7] = (pCreationTime >> 56) & 0b11111111;
		data.push_back(creationTimeBytes[0]);
		data.push_back(creationTimeBytes[1]);
		data.push_back(creationTimeBytes[2]);
		data.push_back(creationTimeBytes[3]);
		data.push_back(creationTimeBytes[4]);
		data.push_back(creationTimeBytes[5]);
		data.push_back(creationTimeBytes[6]);
		data.push_back(creationTimeBytes[7]);

		numZeroedOutBytes[0] = pZeroedOutBytes & 0b11111111;
		numZeroedOutBytes[1] = (pZeroedOutBytes >> 8) & 0b11111111;
		numZeroedOutBytes[2] = (pZeroedOutBytes >> 16) & 0b11111111;
		numZeroedOutBytes[3] = (pZeroedOutBytes >> 24) & 0b11111111;
		numZeroedOutBytes[4] = (pZeroedOutBytes >> 32) & 0b11111111;
		numZeroedOutBytes[5] = (pZeroedOutBytes >> 40) & 0b11111111;
		numZeroedOutBytes[6] = (pZeroedOutBytes >> 48) & 0b11111111;
		numZeroedOutBytes[7] = (pZeroedOutBytes >> 56) & 0b11111111;
		data.push_back(numZeroedOutBytes[0]);
		data.push_back(numZeroedOutBytes[1]);
		data.push_back(numZeroedOutBytes[2]);
		data.push_back(numZeroedOutBytes[3]);
		data.push_back(numZeroedOutBytes[4]);
		data.push_back(numZeroedOutBytes[5]);
		data.push_back(numZeroedOutBytes[6]);
		data.push_back(numZeroedOutBytes[7]);
		break;
	}

	if (pCompression)
		data.push_back(pCompressionMethod);
	if (pEncryption)
		data.push_back(pEncryptionMethod);

	if (data.size() != pSize)
	{
		std::cerr << "Error: header size mismatch" << std::endl;
		std::cerr << "Expected size: " << (unsigned int)pSize << std::endl;
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

DirectoryHeader::DirectoryHeader()
{
	pEncryption = false;
	pSize = 0;
	pID = 0;
	pNameLength = 0;
	pName = "";
	pNumFiles = 0;
	pFileIDs = std::vector<uint32_t>();
	pNumDirectories = 0;
	pDirectoryIDs = std::vector<uint32_t>();
	pCreationTime = 0;
	pSaved = false;
	pHeaderOffset = 0;
}

void DirectoryHeader::load(std::string path, uint8_t storageMode)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Error: could not open file " << path << std::endl;
		return;
	}

	char flags;

	file.seekg(pHeaderOffset);
	
	switch (storageMode)
	{
	case 1:
		file.read(reinterpret_cast<char*>(&pSize), 2);
		file.read(reinterpret_cast<char*>(&flags), 1);
		file.read(reinterpret_cast<char*>(&pID), 1);
		file.read(reinterpret_cast<char*>(&pNameLength), 1);
		pName = std::string(pNameLength, 0);
		for (int i = 0; i < pNameLength; i++)
		{
			file.read(&pName[i], 1);
		}
		file.read(reinterpret_cast<char*>(&pNumFiles), 1);
		file.read(reinterpret_cast<char*>(&pNumDirectories), 1);
		pFileIDs = std::vector<uint32_t>(pNumFiles, 0);
		for (int i = 0; i < pNumFiles; i++)
		{
			file.read(reinterpret_cast<char*>(&pFileIDs[i]), 2);
		}
		pDirectoryIDs = std::vector<uint32_t>(pNumDirectories, 0);
		for (int i = 0; i < pNumDirectories; i++)
		{
			file.read(reinterpret_cast<char*>(&pDirectoryIDs[i]), 1);
		}
		break;

	case 2:
		file.read(reinterpret_cast<char*>(&pSize), 4);
		file.read(reinterpret_cast<char*>(&flags), 1);
		file.read(reinterpret_cast<char*>(&pID), 2);
		file.read(reinterpret_cast<char*>(&pNameLength), 2);
		pName = std::string(pNameLength, 0);
		for (int i = 0; i < pNameLength; i++)
		{
			file.read(&pName[i], 1);
		}
		file.read(reinterpret_cast<char*>(&pNumFiles), 3);
		file.read(reinterpret_cast<char*>(&pNumDirectories), 3);
		pFileIDs = std::vector<uint32_t>(pNumFiles, 0);
		for (int i = 0; i < pNumFiles; i++)
		{
			file.read(reinterpret_cast<char*>(&pFileIDs[i]), 3);
		}
		pDirectoryIDs = std::vector<uint32_t>(pNumDirectories, 0);
		for (int i = 0; i < pNumDirectories; i++)
		{
			file.read(reinterpret_cast<char*>(&pDirectoryIDs[i]), 2);
		}
		file.read(reinterpret_cast<char*>(&pCreationTime), 4);
		break;
	case 3:
		file.read(reinterpret_cast<char*>(&pSize), 5);
		file.read(reinterpret_cast<char*>(&flags), 1);
		file.read(reinterpret_cast<char*>(&pID), 3);
		file.read(reinterpret_cast<char*>(&pNameLength), 2);
		pName = std::string(pNameLength, 0);
		for (int i = 0; i < pNameLength; i++)
		{
			file.read(&pName[i], 1);
		}
		file.read(reinterpret_cast<char*>(&pNumFiles), 4);
		file.read(reinterpret_cast<char*>(&pNumDirectories), 4);
		pFileIDs = std::vector<uint32_t>(pNumFiles, 0);
		for (int i = 0; i < pNumFiles; i++)
		{
			file.read(reinterpret_cast<char*>(&pFileIDs[i]), 4);
		}
		pDirectoryIDs = std::vector<uint32_t>(pNumDirectories, 0);
		for (int i = 0; i < pNumDirectories; i++)
		{
			file.read(reinterpret_cast<char*>(&pDirectoryIDs[i]), 3);
		}
		file.read(reinterpret_cast<char*>(&pCreationTime), 8);
		break;
	}
	file.close();

	saved = true;
}

void DirectoryHeader::save(std::string path, uint8_t storageMode)
{	
	std::vector<char> data;
	data.reserve(size);

	char sizeBytes[5];
	char flag = 0;
	char idBytes[3];
	char nameLengthBytes[2];
	char numFilesBytes[4];
	char numDirectoriesBytes[4];
	char fileIdBytes[4];
	char directoryIdBytes[3];
	char creationTimeBytes[8];
	
	flag |= encryption << 7;

	switch (storageMode)
	{
	case 1:
		sizeBytes[0] = pSize & 0b11111111;
		sizeBytes[1] = (pSize >> 8) & 0b11111111;
		data.push_back(sizeBytes[0]);
		data.push_back(sizeBytes[1]);
		
		data.push_back(flag);

		idBytes[0] = pID & 0b11111111;
		data.push_back(idBytes[0]);

		nameLengthBytes[0] = pNameLength & 0b11111111;
		data.push_back(nameLengthBytes[0]);

		for (int i = 0; i < pNameLength; i++)
		{
			data.push_back(pName[i]);
		}

		numFilesBytes[0] = pNumFiles & 0b11111111;
		data.push_back(numFilesBytes[0]);

		numDirectoriesBytes[0] = pNumDirectories & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);

		for (int i = 0; i < pNumFiles; i++)
		{
			fileIdBytes[0] = pFileIDs[i] & 0b11111111;
			fileIdBytes[1] = (pFileIDs[i] >> 8) & 0b11111111;
			data.push_back(fileIdBytes[0]);
			data.push_back(fileIdBytes[1]);
		}

		for (int i = 0; i < pNumDirectories; i++)
		{
			directoryIdBytes[0] = pDirectoryIDs[i] & 0b11111111;
			data.push_back(directoryIdBytes[0]);
		}
		break;
	case 2:
		sizeBytes[0] = pSize & 0b11111111;
		sizeBytes[1] = (pSize >> 8) & 0b11111111;
		sizeBytes[2] = (pSize >> 16) & 0b11111111;
		sizeBytes[3] = (pSize >> 24) & 0b11111111;
		data.push_back(sizeBytes[0]);
		data.push_back(sizeBytes[1]);
		data.push_back(sizeBytes[2]);
		data.push_back(sizeBytes[3]);

		data.push_back(flag);

		idBytes[0] = pID & 0b11111111;
		idBytes[1] = (pID >> 8) & 0b11111111;
		data.push_back(idBytes[0]);
		data.push_back(idBytes[1]);

		nameLengthBytes[0] = pNameLength & 0b11111111;
		nameLengthBytes[1] = (pNameLength >> 8) & 0b11111111;
		data.push_back(nameLengthBytes[0]);
		data.push_back(nameLengthBytes[1]);

		for (int i = 0; i < pNameLength; i++)
		{
			data.push_back(pName[i]);
		}

		numFilesBytes[0] = pNumFiles & 0b11111111;
		numFilesBytes[1] = (pNumFiles >> 8) & 0b11111111;
		numFilesBytes[2] = (pNumFiles >> 16) & 0b11111111;
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		data.push_back(numFilesBytes[2]);

		numDirectoriesBytes[0] = pNumDirectories & 0b11111111;
		numDirectoriesBytes[1] = (pNumDirectories >> 8) & 0b11111111;
		numDirectoriesBytes[2] = (pNumDirectories >> 16) & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);
		data.push_back(numDirectoriesBytes[1]);
		data.push_back(numDirectoriesBytes[2]);

		for (int i = 0; i < pNumFiles; i++)
		{
			fileIdBytes[0] = pFileIDs[i] & 0b11111111;
			fileIdBytes[1] = (pFileIDs[i] >> 8) & 0b11111111;
			fileIdBytes[2] = (pFileIDs[i] >> 16) & 0b11111111;
			data.push_back(fileIdBytes[0]);
			data.push_back(fileIdBytes[1]);
			data.push_back(fileIdBytes[2]);
		}

		for (int i = 0; i < pNumDirectories; i++)
		{
			directoryIdBytes[0] = pDirectoryIDs[i] & 0b11111111;
			directoryIdBytes[1] = (pDirectoryIDs[i] >> 8) & 0b11111111;
			data.push_back(directoryIdBytes[0]);
			data.push_back(directoryIdBytes[1]);
		}

		creationTimeBytes[0] = pCreationTime & 0b11111111;
		creationTimeBytes[1] = (pCreationTime >> 8) & 0b11111111;
		creationTimeBytes[2] = (pCreationTime >> 16) & 0b11111111;
		creationTimeBytes[3] = (pCreationTime >> 24) & 0b11111111;
		data.push_back(creationTimeBytes[0]);
		data.push_back(creationTimeBytes[1]);
		data.push_back(creationTimeBytes[2]);
		data.push_back(creationTimeBytes[3]);
		break;
	case 3:
		sizeBytes[0] = pSize & 0b11111111;
		sizeBytes[1] = (pSize >> 8) & 0b11111111;
		sizeBytes[2] = (pSize >> 16) & 0b11111111;
		sizeBytes[3] = (pSize >> 24) & 0b11111111;
		sizeBytes[4] = (pSize >> 32) & 0b11111111;
		data.push_back(sizeBytes[0]);
		data.push_back(sizeBytes[1]);
		data.push_back(sizeBytes[2]);
		data.push_back(sizeBytes[3];
		data.push_back(sizeBytes[4]);

		data.push_back(flag);

		idBytes[0] = pID & 0b11111111;
		idBytes[1] = (pID >> 8) & 0b11111111;
		idBytes[2] = (pID >> 16) & 0b11111111;
		data.push_back(idBytes[0]);
		data.push_back(idBytes[1]);
		data.push_back(idBytes[2]);

		nameLengthBytes[0] = pNameLength & 0b11111111;
		nameLengthBytes[1] = (pNameLength >> 8) & 0b11111111;
		data.push_back(nameLengthBytes[0]);
		data.push_back(nameLengthBytes[1]);
		
		for (int i = 0; i < pNameLength; i++)
		{
			data.push_back(pName[i]);
		}

		numFilesBytes[0] = pNumFiles & 0b11111111;
		numFilesBytes[1] = (pNumFiles >> 8) & 0b11111111;
		numFilesBytes[2] = (pNumFiles >> 16) & 0b11111111;
		numFilesBytes[3] = (pNumFiles >> 24) & 0b11111111;
		data.push_back(numFilesBytes[0]);
		data.push_back(numFilesBytes[1]);
		data.push_back(numFilesBytes[2]);
		data.push_back(numFilesBytes[3]);

		numDirectoriesBytes[0] = pNumDirectories & 0b11111111;
		numDirectoriesBytes[1] = (pNumDirectories >> 8) & 0b11111111;
		numDirectoriesBytes[2] = (pNumDirectories >> 16) & 0b11111111;
		numDirectoriesBytes[3] = (pNumDirectories >> 24) & 0b11111111;
		data.push_back(numDirectoriesBytes[0]);
		data.push_back(numDirectoriesBytes[1]);
		data.push_back(numDirectoriesBytes[2]);
		data.push_back(numDirectoriesBytes[3]);

		for (int i = 0; i < pNumFiles; i++)
		{
			fileIdBytes[0] = pFileIDs[i] & 0b11111111;
			fileIdBytes[1] = (pFileIDs[i] >> 8) & 0b11111111;
			fileIdBytes[2] = (pFileIDs[i] >> 16) & 0b11111111;
			fileIdBytes[3] = (pFileIDs[i] >> 24) & 0b11111111;
			data.push_back(fileIdBytes[0]);
			data.push_back(fileIdBytes[1]);
			data.push_back(fileIdBytes[2]);
			data.push_back(fileIdBytes[3]);
		}

		for (int i = 0; i < pNumDirectories; i++)
		{
			directoryIdBytes[0] = pDirectoryIDs[i] & 0b11111111;
			directoryIdBytes[1] = (pDirectoryIDs[i] >> 8) & 0b11111111;
			directoryIdBytes[2] = (pDirectoryIDs[i] >> 16) & 0b11111111;
			data.push_back(directoryIdBytes[0]);
			data.push_back(directoryIdBytes[1]);
			data.push_back(directoryIdBytes[2]);
		}

		creationTimeBytes[0] = pCreationTime & 0b11111111;
		creationTimeBytes[1] = (pCreationTime >> 8) & 0b11111111;
		creationTimeBytes[2] = (pCreationTime >> 16) & 0b11111111;
		creationTimeBytes[3] = (pCreationTime >> 24) & 0b11111111;
		creationTimeBytes[4] = (pCreationTime >> 32) & 0b11111111;
		creationTimeBytes[5] = (pCreationTime >> 40) & 0b11111111;
		creationTimeBytes[6] = (pCreationTime >> 48) & 0b11111111;
		creationTimeBytes[7] = (pCreationTime >> 56) & 0b11111111;
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
	
	if (pSize != data.size())
	{
		std::cerr << "Error: directory header size mismatch" << std::endl;
		std::cerr << "Directory ID: " << pID << std::endl;
		std::cerr << "Expected size: " << pSize << std::endl;
		std::cerr << "Actual size: " << data.size() << std::endl;
		return;
	}
	
	std::ofstream file(path, std::ios::binary);
	//file.seekp(pOffset);
	//file.write(data.data(), data.size());
	//file.close();

	pSaved = true;
}

void DirectoryHeader::remove()
{

}

//---------------------

FileHeader::FileHeader()
{
	pEncryption = false;
	pSize = 0;
	pID = 0;
	pNameLength = 0;
	pName = "";
	pFileSize = 0;
	pCreationTime = 0;
	pOffset = 0;
	pSaved = false;
	pHeaderOffset = 0;
}

void FileHeader::save(std::string path, uint64_t offset)
{
}

void FileHeader::remove()
{
}
