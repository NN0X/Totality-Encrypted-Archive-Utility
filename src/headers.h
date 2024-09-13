#ifndef HEADERS_H
#define HEADERS_H

#include <cstdint>
#include <vector>

struct Header
{
	uint8_t size;

	bool compression;
	bool encryption;
	bool endianess;
	uint8_t storageMode;
	uint8_t compressionLevel;

	uint8_t compressionMethod;
	uint8_t encryptionMethod;

	uint32_t numDirectories;
	uint32_t numFiles;
	uint64_t totalSize;
	uint64_t creationTime;
	
	void create();
	void load(std::string path);
	void save(std::string path);
};

struct DirectoryHeader
{
	bool encryption;
	uint64_t size;
	uint16_t nameLength;
	char* name;
	uint32_t numFiles;
	uint32_t* fileIds;
	uint32_t numDirectories;
	uint32_t* directoryIds;
	uint64_t creationTime;

	bool saved;

	void create();
	void save(std::string path, uint64_t offsetEnc, uint8_t encryptionBit,uint64_t offsetHead, uint8_t storageMode);
	void remove();
};

struct FileHeader
{
	uint16_t size;
	uint32_t id;
	uint16_t nameLength;
	char* name;
	uint64_t fileSize;
	uint64_t creationTime;
	uint64_t offset;

	bool saved;
	
	void create();
	void save(std::string path, uint64_t offset);
	void remove();
};

#endif // HEADERS_H
