#include <iostream>
#include <vector>


class Archive
{
private:
	std::string name;

public:
	Archive(std::string name) : name(name) {}

};

// Archive Format

// all header fields are in little endian
// 
// ---------------------
// (5, 7, 19, 27) bytes - max size of header in bytes
// HEADER
// 1 bytes - header size in bytes (including this field)
//
// 1 bytes - flags (compression, encryption, endianess excluding header, 2 bits for storage mode, 3 bits reserved)
// 	storage mode (size of headers):
//		0 - very low <- this is for extreme cases (max size of headers is ~383KiB for ~1TiB of data)
//		1 - low (max size of headers is ~128MiB for ~64PiB of data)
//		2 - medium (max size of headers is ~12TiB for ~16EiB of data)
//		3 - high <- this is the default (max size of headers is ~768PiB for ~16EiB of data)
//
// (0, 1, 2, 3) bytes - number of directories
// (1, 2, 3, 4) bytes - number of files
// (0, 0, 6, 8) bytes - total size of all files (without header, without file table) before compression in bytes
// (0, 0, 4, 8) bytes - epoch time of creation of this archive 
//
// if compression flag is set:
// 	1 bytes - compression method
// 
// if encryption flag is set:
// 	1 bytes - encryption method
//
// ---------------------
//  0  ~192KiB       ~3TiB               ~256PiB
// (0, 196 127, 3 302 780 051 447, 288 231 458 720 382 962) bytes - max size of directory table in bytes
// DIRECTORY TABLE
// (0, 3, 6, 8) bytes - size of directory table in bytes (including this field)
//
// (0, 1/8, 1/8, 1/8) bytes - max size for directory table entry in bytes
// for each 8 directories: (i = 1, ... , number of directories / 8)
// 	(0, 1, 1, 1) bytes - directory encryption flag (if set, this directory is encrypted separately)
// 	// 0 0 0 0 0 0 0 1 <- this means that 8*i directory table entry is encrypted
// 
// (0, 769, 50 397 193, 17 179 934 734) - max size of directory table entry in bytes
// for each directory: 
// 	(0, 2, 4, 5) bytes - directory header size in bytes (including this field)
// 	(0, 1, 2, 2) bytes - directory name length in bytes
// 	(0, n, n, n) bytes - directory name
// 	(0, 1, 3, 4) bytes - number of files in this directory
// 	(0, 2n,3n,4n) bytes - ids of files in this directory
// 	(0, 0, 4, 8) bytes - epoch time of creation of this directory
//
// ---------------------
//  ~383KiB   ~128MiB           ~9TiB                ~512PiB
// (392 480, 134 420 481, 9 895 790 706 683, 576 742 296 402 198 519) bytes - max size of file table in bytes
// FILE TABLE
// (3, 4, 6, 8) bytes - size of file table in bytes (including this field)
//
// (1/8, 1/8, 1/8, 1/8) bytes - max size for file table entry in bytes
// for each 8 files: (i = 1, ... , number of files / 8)
// 	1 byte - file split flag (if set, this is the header of a split file)
// 	// 0 0 0 0 0 0 0 1 <- this means that 8*i file table entry is a header of a split file
//
// (1 539, 2 051, 589 835, 134 283 279) bytes - max size of file table entry in bytes
// for each file: 
// 	(2, 2, 3, 4) bytes - file header size in bytes (including this field)
// 	(1, 2, 3, 4) bytes - file id (unique, in most basic case, it is the index of the file in the file table)
// 	(1, 1, 2, 2) bytes - file name length in bytes
// 	(n, n, n, n) bytes - file name
// 	(4, 5, 6, 8) bytes - file size in bytes
// 	(0, 0, 4, 8) bytes - epoch time of creation of this file
//	
// 	if file split flag is set:
// 		(1, 1, 2, 3) bytes - number of parts
// 		(5n,7n,8n,8n) bytes - offsets to the beginnings of each parts data (from the end of the file table)
//
// 	if file split flag is not set:
// 		(5, 7, 8, 8) bytes - offset to the beginning of the file data (from the end of the file table)
//
// ---------------------
//        ~1TiB               ~64PiB                 ~16EiB                ~16EiB
// (1 095 216 660 225, 72 056 494 526 234 625, capped at MAX_UINT64, capped at MAX_UINT64) bytes - size of file data in bytes
// FILE DATA
//
// for each file and each part of a split file:
// 	n bytes - file data

int main()
{
	std::vector<std::string> fileNames;
	fileNames.push_back("file1.txt");
	fileNames.push_back("file2.png");
	fileNames.push_back("file3.webm");


}
