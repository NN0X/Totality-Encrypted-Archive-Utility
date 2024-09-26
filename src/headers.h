#ifndef HEADERS_H
#define HEADERS_H

#include <cstdint>
#include <vector>
#include <string>
#include <vector>

// ---------------------
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
// (0, 0, 4, 8) bytes - number of zeroed out bytes (including all tables and file data)
//
// if compression flag is set:
// 	1 bytes - compression method
// 
// if encryption flag is set:
// 	1 bytes - encryption method


class ArchiveHeader
{
public:
	ArchiveHeader();

	void load(std::string path);
	void save(std::string path);

	uint8_t pSize;

	bool pCompression;
	bool pEncryption;
	uint8_t pStorageMode;
	uint8_t pCompressionLevel;

	uint8_t pCompressionMethod;
	uint8_t pEncryptionMethod;

	uint32_t pNumDirectories;
	uint32_t pNumFiles;
	uint64_t pTotalSize;
	uint64_t pCreationTime;
	uint64_t pZeroedOutBytes;
};


// ---------------------
// DIRECTORY HEADER
// 	(0, 2, 4, 5) bytes - directory header size in bytes (including this field)
// 	(0, 1, 1, 1) bytes - flags (1 bit for encryption, 7 bits reserved)
// 	(0, 1, 2, 3) bytes - directory id (unique, in most basic case, it is the index of the directory in the directory table)
// 	(0, 1, 2, 2) bytes - directory name length in bytes
// 	(0, n, n, n) bytes - directory name
// 	(0, 1, 3, 4) bytes - number of files in this directory
// 	(0, 1, 3, 4) bytes - number of directories in this directory
// 	(0, 2n,3n,4n) bytes - ids of files in this directory
// 	(0, 1n,2n,3n) bytes - ids of directories in this directory
// 	(0, 0, 4, 8) bytes - epoch time of creation of this directory

class DirectoryHeader
{
public:
	DirectoryHeader();

	void load(std::string path, uint8_t storageMode);
	void save(std::string path, uint8_t storageMode);
	void remove();
	
	void setEncryption(bool encryption);
	void setID(uint32_t id);
	void setName(std::string name);
	void addFile(uint32_t id);
	void addDirectory(uint32_t id);
	void removeFile(uint32_t id);
	void removeDirectory(uint32_t id);
	void setCreationTime(uint64_t time);

	bool isEncrypted();
	uint32_t getID();
	std::string getName();
	uint64_t getCreationTime();
	uint32_t getNumFiles();
	uint32_t getNumDirectories();
	std::vector<uint32_t> getFileIDs();
	std::vector<uint32_t> getDirectoryIDs();
	bool isSaved();

private:
	bool pEncryption;
	uint64_t pSize;
	uint32_t pID;
	uint16_t pNameLength;
	std::string pName;
	uint32_t pNumFiles;
	std::vector<uint32_t> pFileIDs;
	uint32_t pNumDirectories;
	std::vector<uint32_t> pDirectoryIDs;
	uint64_t pCreationTime;

	bool pSaved;
	uint64_t pHeaderOffset;	// from the beginning of the directory table
};

// ---------------------
// FILE HEADER 
// 	(2, 2, 3, 4) bytes - file header size in bytes (including this field)
// 	(1, 1, 1, 1) bytes - flags (1 bit for encryption, 7 bits reserved)
// 	(1, 2, 3, 4) bytes - file id (unique, in most basic case, it is the index of the file in the file table)
// 	(1, 1, 2, 2) bytes - file name length in bytes
// 	(n, n, n, n) bytes - file name
// 	(4, 5, 6, 8) bytes - file size in bytes
// 	(0, 0, 4, 8) bytes - epoch time of creation of this file
// 	(5, 7, 8, 8) bytes - offset to the beginning of the file data (from the end of the file table)


class FileHeader
{
public:
	FileHeader();

	void load(std::string path, uint8_t storageMode);
	void save(std::string path, uint8_t storageMode);
	void remove();

	void setEncryption(bool encryption);
	void setID(uint32_t id);
	void setName(std::string name);
	//void setFile();
	
	bool isEncrypted();
	uint32_t getID();
	std::string getName();
	uint64_t getCreationTime();
	uint64_t getSize();
	uint64_t getOffset();
	bool isSaved();
	

private:
	bool pEncryption;
	uint16_t pSize;
	uint32_t pID;
	uint16_t pNameLength;
	std::string pName;
	uint64_t pFileSize;
	uint64_t pCreationTime;
	uint64_t pOffset;

	bool pSaved;
	uint64_t pHeaderOffset;	// from the beginning of the file table
};

#endif // HEADERS_H
