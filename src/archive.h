#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "tables.h"
#include "headers.h"

struct FileData
{
	uint64_t size;
	uint64_t offset;

	uint8_t* data;
};

class Archive
{
public:
	Archive(std::string name, std::string path);
	~Archive();
	
	void printCurrentFile();
	void printCurrentDirectory();

	void printFilesTree();
	void printDirectoriesTree();

	void addFile(std::string filePath);
	void eraseFile(std::string fileName); // zero out the data and remove the file from the file table

	void addDirectory(std::string directoryPath);
	void eraseDirectory(std::string directoryName); // remove the directory from the directory table and erase all files in it

	void rebuild(); // remove all zeroed out bytes and reorganize the archive(sort ids, etc.)

	void setFile(std::string fileName);
	void setDirectory(std::string directoryName);

	void extractFile(std::string filePath);
	void extractDirectory(std::string directoryPath);

	void extractAll(std::string path);
	
	void compressHUFFMAN();
	void compressDEFLATE();

	void decompress();
	
	void encryptAES();
	void encryptBRUTUS();

	void encryptDirectoryAES();
	void encryptDirectoryBRUTUS();

	void decrypt();
	void decryptDirectory();

	uint16_t getRamLimit();
	uint16_t calculateRamUsage();
	uint64_t getStorageLimit();
	uint64_t calculateStorageUsage();

	std::string getName();
	std::string getPath();

private:
	std::string pName;
	std::string pPath;
	std::string pFullPath;

	uint32_t pCurrentDirectoryID; // 0 is top level directory
	uint32_t pCurrentFileID; // 0 is NULL

	uint16_t pRamLimit; // in MiB
	uint16_t pStorageLimit; // in MiB

	ArchiveHeader pArchiveHeader;
	DirectoryTable pDirectoryTable;
	FileTable pFileTable;

	uint64_t pFileLastID;
	uint64_t pDirectoryLastID;
};

#endif // ARCHIVE_H


// Archive Format

// archive is in little endian
//
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
//
// ---------------------
// DIRECTORY TABLE
// (0, 3, 6, 8) bytes - size of directory table in bytes (including this field)
// 
// for each directory: 
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
//
// ---------------------
// FILE TABLE
// (3, 4, 6, 8) bytes - size of file table in bytes (including this field)
//
// for each file: 
// 	(2, 2, 3, 4) bytes - file header size in bytes (including this field)
// 	(1, 1, 1, 1) bytes - flags (1 bit for encryption, 7 bits reserved)
// 	(1, 2, 3, 4) bytes - file id (unique, in most basic case, it is the index of the file in the file table)
// 	(1, 1, 2, 2) bytes - file name length in bytes
// 	(n, n, n, n) bytes - file name
// 	(4, 5, 6, 8) bytes - file size in bytes
// 	(0, 0, 4, 8) bytes - epoch time of creation of this file
// 	(5, 7, 8, 8) bytes - offset to the beginning of the file data (from the end of the file table)
//
// ---------------------
// FILE DATA
//
// for each file and each part of a split file:
// 	n bytes - file data

