#ifndef HEADERS_H
#define HEADERS_H

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

	void load(std::string path);
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

};

#endif // HEADERS_H
