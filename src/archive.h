#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <string>
#include <unordered_map>

#include "tables.h"
#include "headers.h"

struct FileData
{
	uint64_t size;
	uint8_t* data;
};

class Archive
{
public:
	Archive(std::string name, std::string path);
	~Archive();
	
	void save();

	void printCurrentFile();
	void printCurrentDirectory();

	void printFilesTree();
	void printDirectoriesTree();

	void addFile(std::string filePath);
	void removeFile(std::string filePath);

	void addDirectory(std::string directoryPath);
	void removeDirectory(std::string directoryPath);
	
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
	uint16_t getRamUsage();

	uint64_t getStorageLimit();
	uint64_t getStorageUsage();

	std::string getName();
	std::string getPath();

private:
	std::string pName;
	std::string pPath;

	uint32_t pCurrentDirectoryId; // 0 is top level directory
	uint32_t pCurrentFileId; // 0 is NULL

	uint16_t pRamLimit; // in MiB
	uint16_t pRamUsage; // in MiB
	
	uint64_t pStorageLimit; // in MiB
	uint64_t pStorageUsage; // in MiB

	Header pHeader;
	DirectoryTable pDirectoryTable;
	FileTable pFileTable;
};

#endif // ARCHIVE_H


// Archive Format

// all header fields are in little endian
// 
// ---------------------
// (5, 7, 19, 27) bytes - max size of header in bytes
// HEADER
// 1 bytes - header size in bytes (including this field)
//
// 1 bytes - flags (compression, encryption, endianess excluding header, 2 bits for storage mode, 2 bits for compression level, 1 bit reserved)
// 	storage mode (size of headers):
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
// ---------------------
// DIRECTORY TABLE
// (0, 3, 6, 8) bytes - size of directory table in bytes (including this field)
//
// for each 4 directories: (i = 1, ... , number of directories / 4)
// 	(0, 1, 1, 1) bytes - directory encryption and top flag (if set, this directory is encrypted separately)
// 	// 0 1 0 0 0 0 1 0 <- this means that 4*i directory table entry is encrypted and 1*i directory table entry is a top level directory
// 
// for each directory: 
// 	(0, 2, 4, 5) bytes - directory header size in bytes (including this field)
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

