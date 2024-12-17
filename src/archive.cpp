#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

#include "archive.h"

TEA::TEA(const std::string &path, const std::string &name) : mName(name), mPath(path)
{
	init();
}

TEA::~TEA()
{
	std::remove(".data.teatemp");
	std::remove(".data_headers.teatemp");
	std::remove(".data_headers_positions.teatemp");
	std::remove(".common_flags.teatemp");
}

void TEA::init()
{
	mSignature = TEA_SIGNATURE;
	mVersion = TEA_VERSION;

	mArchiveHeader.mNumFiles = 0;
	mArchiveHeader.mPosDataHeaders = 0;
	mArchiveHeader.mNumUniqueFlags = 7; // default number of bits used for flags
	mArchiveHeader.mPosCommonFlags = 0;
	mArchiveHeader.mSizeMetadata = 0;
	mArchiveHeader.mPosMetadata = 0;
	mArchiveHeader.mGlobalFlags = 0b0000000000000000; // default flags
	mArchiveHeader.mReserved = 0;

	mDataHeader.mPosEndFileHeaders = 0;
	mDataHeader.mFileHeadersCached = std::vector<FileHeader>();
	mDataHeader.mFileHeadersCached.reserve(1024);
	mDataHeader.mPosFileHeaders = std::vector<uint64_t>();
	mDataHeader.mPosFileHeaders.reserve(1024);

	mDataCached = std::vector<uint8_t>();
	mDataCached.reserve(1024);

	mCommonFlagsCached = std::vector<uint8_t>();
	mCommonFlagsCached.reserve(1024);
	
	mMetadata = "";

	std::ofstream dataTemp(".data.teatemp", std::ios::binary);
	dataTemp.close();
	std::ofstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary);
	dataHeadersTemp.close();
	std::ofstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary);
	dataHeadersPositionsTemp.close();
	std::ofstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary);
	commonFlagsTemp.close();
}

bool deleteFileChunk(std::fstream& file, size_t pos, size_t size, const std::string &path)
{
	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	if (pos + size > fileSize)
	{
		std::cerr << "Invalid position or size\n";
		return false;
	}
	std::vector<uint8_t> data(1024);

	size_t remaining = fileSize - pos - size;
	size_t readPos = pos + size;
	size_t writePos = pos;
	
	while (remaining > 0)
	{
		size_t readSize = remaining > 1024 ? 1024 : remaining;
		file.seekg(readPos, std::ios::beg);
		if (!file.read(reinterpret_cast<char*>(&data[0]), readSize))
		{
			std::cerr << "Failed to read data\n";
			return false;
		}
		file.seekp(writePos, std::ios::beg);
		if (!file.write(reinterpret_cast<char*>(&data[0]), readSize))
		{
			std::cerr << "Failed to write data\n";
			return false;
		}
		readPos += readSize;
		writePos += readSize;
		remaining -= readSize;
	}

	std::filesystem::resize_file(path, fileSize - size);

	return true;
}

bool TEA::load()
{
	size_t archiveSize = 0;
	std::string fullPath = mPath + "/" + mName + ".tea";
	std::fstream archive(fullPath, std::ios::binary | std::ios::in | std::ios::out);
	if (archive.is_open())
	{
		archiveSize = archive.tellg();
		archive.seekg(0, std::ios::beg);
	}
	else
	{
		std::cerr << "Failed to open archive\n";
		return false;
	}

	// load 3 signature bytes and check if they match
	std::string signature;
	signature.resize(3);
	archive.read(&signature[0], 3);
	if (signature != TEA_SIGNATURE)
	{
		std::cerr << "Invalid signature\n";
		return false;
	}
	// skip 1 byte padding
	archive.seekg(1, std::ios::cur);

	// check version
	uint8_t version;
	archive.read(reinterpret_cast<char*>(&version), sizeof(uint8_t));
	if (version != TEA_VERSION)
	{
		std::cerr << "Invalid version\n";
		return false;
	}

	// move to the end of the file
	archive.seekg(archiveSize - 3, std::ios::beg);

	// check signature at the end of the file
	std::string signatureEnd;
	signatureEnd.resize(3);
	archive.read(&signatureEnd[0], 3);
	if (signatureEnd != TEA_SIGNATURE)
	{
		std::cerr << "Invalid signature\n";
		return false;
	}

	// read archive header
	archive.seekg(archiveSize - sizeof(ArchiveHeader) - 4, std::ios::beg);
	std::vector<uint8_t> headerData(sizeof(ArchiveHeader));
	archive.read(reinterpret_cast<char*>(&headerData[0]), sizeof(ArchiveHeader));

	// serialize archive header
	mArchiveHeader.mNumFiles = *reinterpret_cast<uint64_t*>(&headerData[0]);
	mArchiveHeader.mPosDataHeaders = *reinterpret_cast<uint64_t*>(&headerData[8]);
	mArchiveHeader.mNumUniqueFlags = *reinterpret_cast<uint16_t*>(&headerData[16]);
	mArchiveHeader.mPosCommonFlags = *reinterpret_cast<uint64_t*>(&headerData[18]);
	mArchiveHeader.mSizeMetadata = *reinterpret_cast<uint16_t*>(&headerData[26]);
	mArchiveHeader.mPosMetadata = *reinterpret_cast<uint64_t*>(&headerData[28]);
	mArchiveHeader.mGlobalFlags = *reinterpret_cast<uint16_t*>(&headerData[36]);
	mArchiveHeader.mReserved = *reinterpret_cast<uint32_t*>(&headerData[38]);

	// move to metadata
	archive.seekg(mArchiveHeader.mPosMetadata, std::ios::beg);
	mMetadata.resize(mArchiveHeader.mSizeMetadata);
	archive.read(&mMetadata[0], mArchiveHeader.mSizeMetadata);
	
	// move to common flags
	archive.seekg(mArchiveHeader.mPosCommonFlags, std::ios::beg);
	mCommonFlagsCached.resize(mArchiveHeader.mNumFiles);
	size_t commonFlagsSize = mArchiveHeader.mNumFiles * mArchiveHeader.mNumUniqueFlags / 8;
	archive.read(reinterpret_cast<char*>(&mCommonFlagsCached[0]), commonFlagsSize);

	// load end of data headers
	archive.seekg(mArchiveHeader.mPosDataHeaders, std::ios::beg);
	archive.read(reinterpret_cast<char*>(&mDataHeader.mPosEndFileHeaders), sizeof(uint64_t));

	// cache 1024 file headers and corresponding positions from the start of the data headers
	archive.seekg(mArchiveHeader.mPosDataHeaders + sizeof(uint64_t), std::ios::beg);
	archive.read(reinterpret_cast<char*>(&mDataHeader.mFileHeadersCached[0]), 1024 * sizeof(FileHeader));
	archive.read(reinterpret_cast<char*>(&mDataHeader.mPosFileHeaders[0]), 1024 * sizeof(uint64_t));

	// split archive file into temporary files
	// 1. data
	// 2. data headers
	// 3. common flags
	
	// create data.teatemp
	std::ofstream dataTemp(".data.teatemp", std::ios::binary);
	if (dataTemp.is_open())
	{
		archive.seekg(6, std::ios::beg);
		size_t endData = mArchiveHeader.mPosDataHeaders - 1;
		size_t dataSize = endData - archive.tellg();
		// copy data in chunks of 1024 bytes to data.teatemp while deleting it from the archive
		for (size_t i = 0; i < dataSize; i += 1024)
		{
			if (dataSize - i < 1024)
			{
				std::vector<uint8_t> data(dataSize - i);
				archive.read(reinterpret_cast<char*>(&data[0]), dataSize - i);
				dataTemp.write(reinterpret_cast<char*>(&data[0]), dataSize - i);
				if (!deleteFileChunk(archive, archive.tellg(), dataSize - i, mPath + "/" + mName))
				{
					std::cerr << "Failed to delete data chunk\n";
					return false;
				}
			}
			else
			{
				std::vector<uint8_t> data(1024);
				archive.read(reinterpret_cast<char*>(&data[0]), 1024);
				dataTemp.write(reinterpret_cast<char*>(&data[0]), 1024);
				if (!deleteFileChunk(archive, archive.tellg(), 1024, mPath + "/" + mName))
				{
					std::cerr << "Failed to delete data chunk\n";
					return false;
				}
			}
		}
		dataTemp.close();
	}
	else
	{
		std::cerr << "Failed to create .data.teatemp\n";
		return false;
	}

	// create data_headers.teatemp
	std::ofstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary);
	if (dataHeadersTemp.is_open())
	{
		archive.seekg(mArchiveHeader.mPosDataHeaders, std::ios::beg);
		size_t dataSize = mDataHeader.mPosEndFileHeaders - mArchiveHeader.mPosDataHeaders;
		// copy data headers in chunks of 1024 bytes to data_headers.teatemp while deleting it from the archive
		for (size_t i = 0; i < dataSize; i += 1024)
		{
			if (dataSize - i < 1024)
			{
				std::vector<uint8_t> data(dataSize - i);
				archive.read(reinterpret_cast<char*>(&data[0]), dataSize - i);
				dataHeadersTemp.write(reinterpret_cast<char*>(&data[0]), dataSize - i);
				if (!deleteFileChunk(archive, archive.tellg(), dataSize - i, mPath + "/" + mName))
				{
					std::cerr << "Failed to delete data headers chunk\n";
					return false;
				}
			}
			else
			{
				std::vector<uint8_t> data(1024);
				archive.read(reinterpret_cast<char*>(&data[0]), 1024);
				dataHeadersTemp.write(reinterpret_cast<char*>(&data[0]), 1024);
				if (!deleteFileChunk(archive, archive.tellg(), 1024, mPath + "/" + mName))
				{
					std::cerr << "Failed to delete data headers chunk\n";
					return false;
				}
			}
		}
		dataHeadersTemp.close();
	}
	else
	{
		std::cerr << "Failed to create .data_headers.teatemp\n";
		return false;
	}

	// create data_headers_positions.teatemp
	std::ofstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary);
	if (dataHeadersPositionsTemp.is_open())
	{
		archive.seekg(mDataHeader.mPosEndFileHeaders, std::ios::beg);
		size_t dataSize = mArchiveHeader.mNumFiles * sizeof(uint64_t);
		// copy data headers in chunks of 1024 bytes to data_headers.teatemp while deleting it from the archive
		for (size_t i = 0; i < dataSize; i += 1024)
		{
			if (dataSize - i < 1024)
			{
				std::vector<uint8_t> data(dataSize - i);
				archive.read(reinterpret_cast<char*>(&data[0]), dataSize - i);
				dataHeadersPositionsTemp.write(reinterpret_cast<char*>(&data[0]), dataSize - i);
				if (!deleteFileChunk(archive, archive.tellg(), dataSize - i, mPath + "/" + mName))
				{
					std::cerr << "Failed to delete data headers positions chunk\n";
					return false;
				}
			}
			else
			{
				std::vector<uint8_t> data(1024);
				archive.read(reinterpret_cast<char*>(&data[0]), 1024);
				dataHeadersPositionsTemp.write(reinterpret_cast<char*>(&data[0]), 1024);
				if (!deleteFileChunk(archive, archive.tellg(), 1024, mPath + "/" + mName))
				{
					std::cerr << "Failed to delete data headers positions chunk\n";
					return false;
				}
			}
		}
		dataHeadersPositionsTemp.close();
	}
	else
	{
		std::cerr << "Failed to create .data_headers_positions.teatemp\n";
		return false;
	}


	// create common_flags.teatemp
	std::ofstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary);
	if (commonFlagsTemp.is_open())
	{
		archive.seekg(mArchiveHeader.mPosCommonFlags, std::ios::beg);
		size_t dataSize = mArchiveHeader.mNumFiles * mArchiveHeader.mNumUniqueFlags / 8;
		// copy common flags in chunks of 1024 bytes to common_flags.teatemp while deleting it from the archive
		for (size_t i = 0; i < dataSize; i += 1024)
		{
			if (dataSize - i < 1024)
			{
				std::vector<uint8_t> data(dataSize - i);
				archive.read(reinterpret_cast<char*>(&data[0]), dataSize - i);
				commonFlagsTemp.write(reinterpret_cast<char*>(&data[0]), dataSize - i);
				if (!deleteFileChunk(archive, archive.tellg(), dataSize - i, mPath + "/" + mName))
				{
					std::cerr << "Failed to delete common flags chunk\n";
					return false;
				}
			}
			else
			{
				std::vector<uint8_t> data(1024);
				archive.read(reinterpret_cast<char*>(&data[0]), 1024);
				commonFlagsTemp.write(reinterpret_cast<char*>(&data[0]), 1024);
				if (!deleteFileChunk(archive, archive.tellg(), 1024, mPath + "/" + mName))
				{
					std::cerr << "Failed to delete common flags chunk\n";
					return false;
				}
			}
		}
		commonFlagsTemp.close();
	}
	else
	{
		std::cerr << "Failed to create .common_flags.teatemp\n";
		return false;
	}
	archive.close();

	std::cout << "Loaded archive " << mName << " from " << mPath << "\n";

	return true;
}

bool TEA::save()
{
	std::string fullPath = mPath + "/" + mName + ".tea";
	std::ofstream archive(fullPath, std::ios::binary | std::ios::out);

	// write signature
	archive.write(TEA_SIGNATURE, 3);
	archive.put(0);
	// write version
	archive.put(TEA_VERSION);
	archive.put(0);

	// load data.teatemp
	std::fstream dataTemp(".data.teatemp", std::ios::binary | std::ios::in | std::ios::out);
	if (dataTemp.is_open())
	{
		dataTemp.seekg(0, std::ios::end);
		size_t size = dataTemp.tellg();
		dataTemp.seekg(0, std::ios::beg);
		for (size_t i = 0; i < size; i+=1024)
		{
			if (size - i < 1024)
			{
				std::vector<uint8_t> data(size - i);
				dataTemp.read(reinterpret_cast<char*>(&data[0]), size - i);
				archive.write(reinterpret_cast<char*>(&data[0]), size - i);
				if (!deleteFileChunk(dataTemp, dataTemp.tellg(), size - i, ".data.teatemp"))
				{
					std::cerr << "Failed to delete data chunk\n";
					return false;
				}
			}
			else
			{
				std::vector<uint8_t> data(1024);
				dataTemp.read(reinterpret_cast<char*>(&data[0]), 1024);
				archive.write(reinterpret_cast<char*>(&data[0]), 1024);
				if (!deleteFileChunk(dataTemp, dataTemp.tellg(), 1024, ".data.teatemp"))
				{
					std::cerr << "Failed to delete data chunk\n";
					return false;
				}
			}
		}

		dataTemp.close();
	}
	archive.put(0);

	// load data_headers.teatemp
	std::fstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary | std::ios::in | std::ios::out);
	if (dataHeadersTemp.is_open())
	{
		dataHeadersTemp.seekg(0, std::ios::end);
		size_t size = dataHeadersTemp.tellg();
		dataHeadersTemp.seekg(0, std::ios::beg);
		for(size_t i = 0; i < size; i+=1024)
		{
			if (size - i < 1024)
			{
				std::vector<uint8_t> data(size - i);
				dataHeadersTemp.read(reinterpret_cast<char*>(&data[0]), size - i);
				archive.write(reinterpret_cast<char*>(&data[0]), size - i);
				if (!deleteFileChunk(dataHeadersTemp, dataHeadersTemp.tellg(), size - i, ".data_headers.teatemp"))
				{
					std::cerr << "Failed to delete data headers chunk\n";
					return false;
				}
			}
			else
			{
				std::vector<uint8_t> data(1024);
				dataHeadersTemp.read(reinterpret_cast<char*>(&data[0]), 1024);
				archive.write(reinterpret_cast<char*>(&data[0]), 1024);
				if (!deleteFileChunk(dataHeadersTemp, dataHeadersTemp.tellg(), 1024, ".data_headers.teatemp"))
				{
					std::cerr << "Failed to delete data headers chunk\n";
					return false;
				}
			}
		}

		dataHeadersTemp.close();
	}

	//load data_headers_positions.teatemp
	std::fstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary | std::ios::in | std::ios::out);
	if (dataHeadersPositionsTemp.is_open())
	{
		dataHeadersPositionsTemp.seekg(0, std::ios::end);
		size_t size = dataHeadersPositionsTemp.tellg();
		dataHeadersPositionsTemp.seekg(0, std::ios::beg);
		for(size_t i = 0; i < size; i+=1024)
		{
			if (size - i < 1024)
			{
				std::vector<uint8_t> data(size - i);
				dataHeadersPositionsTemp.read(reinterpret_cast<char*>(&data[0]), size - i);
				archive.write(reinterpret_cast<char*>(&data[0]), size - i);
				if (!deleteFileChunk(dataHeadersPositionsTemp, dataHeadersPositionsTemp.tellg(), size - i, ".data_headers_positions.teatemp"))
				{
					std::cerr << "Failed to delete data headers positions chunk\n";
					return false;
				}
			}
			else
			{
				std::vector<uint8_t> data(1024);
				dataHeadersPositionsTemp.read(reinterpret_cast<char*>(&data[0]), 1024);
				archive.write(reinterpret_cast<char*>(&data[0]), 1024);
				if (!deleteFileChunk(dataHeadersPositionsTemp, dataHeadersPositionsTemp.tellg(), 1024, ".data_headers_positions.teatemp"))
				{
					std::cerr << "Failed to delete data headers positions chunk\n";
					return false;
				}
			}
		}

		dataHeadersPositionsTemp.close();
	}

	// load common_flags.teatemp
	std::fstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary | std::ios::in | std::ios::out);
	if (commonFlagsTemp.is_open())
	{
		commonFlagsTemp.seekg(0, std::ios::end);
		size_t size = commonFlagsTemp.tellg();
		commonFlagsTemp.seekg(0, std::ios::beg);
		for(size_t i = 0; i < size; i+=1024)
		{
			if(size - i < 1024)
			{
				std::vector<uint8_t> data(size - i);
				commonFlagsTemp.read(reinterpret_cast<char*>(&data[0]), size - i);
				archive.write(reinterpret_cast<char*>(&data[0]), size - i);
				if (!deleteFileChunk(commonFlagsTemp, commonFlagsTemp.tellg(), size - i, ".common_flags.teatemp"))
				{
					std::cerr << "Failed to delete common flags chunk\n";
					return false;
				}
			}
			else
			{
				std::vector<uint8_t> data(1024);
				commonFlagsTemp.read(reinterpret_cast<char*>(&data[0]), 1024);
				archive.write(reinterpret_cast<char*>(&data[0]), 1024);
				if (!deleteFileChunk(commonFlagsTemp, commonFlagsTemp.tellg(), 1024, ".common_flags.teatemp"))
				{
					std::cerr << "Failed to delete common flags chunk\n";
					return false;
				
				}
			}
		}

		commonFlagsTemp.close();
	}
	archive.put(0);

	// write metadata
	archive.write(&mMetadata[0], mMetadata.size());
	archive.put(0);

	// write archive header
	archive.write(reinterpret_cast<char*>(&mArchiveHeader.mNumFiles), sizeof(uint64_t));
	archive.write(reinterpret_cast<char*>(&mArchiveHeader.mPosDataHeaders), sizeof(uint64_t));
	archive.write(reinterpret_cast<char*>(&mArchiveHeader.mNumUniqueFlags), sizeof(uint16_t));
	archive.write(reinterpret_cast<char*>(&mArchiveHeader.mPosCommonFlags), sizeof(uint64_t));
	archive.write(reinterpret_cast<char*>(&mArchiveHeader.mSizeMetadata), sizeof(uint16_t));
	archive.write(reinterpret_cast<char*>(&mArchiveHeader.mPosMetadata), sizeof(uint64_t));
	archive.write(reinterpret_cast<char*>(&mArchiveHeader.mGlobalFlags), sizeof(uint16_t));
	archive.write(reinterpret_cast<char*>(&mArchiveHeader.mReserved), sizeof(uint32_t));
	archive.put(0);

	// write signature at the end of the file
	archive.write(TEA_SIGNATURE, 3);

	// delete temporary files
	std::filesystem::remove(".data.teatemp");
	std::filesystem::remove(".data_headers.teatemp");
	std::filesystem::remove(".data_headers_positions.teatemp");
	std::filesystem::remove(".common_flags.teatemp");

	std::cout << "Saved archive to " << mPath << "\n";

	return true;
}

uint64_t getFileModEpochTime(const std::string &path)
{
	std::filesystem::file_time_type time = std::filesystem::last_write_time(path);
	uint64_t modTimeInEpoch = time.time_since_epoch().count();
	return modTimeInEpoch;
}

uint64_t getCurrentEpochTime()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	std::chrono::duration<double> duration = now.time_since_epoch();
	return duration.count();
}

struct DirSearch
{
	bool mRoot;
	bool mFound;
	bool mDirectory;
	uint64_t mPos;
};

DirSearch findDirectory(const std::string &archiveInternalPath)
{
	if (archiveInternalPath.empty() || archiveInternalPath == ".")
	{
		DirSearch search;
		search.mFound = true;
		search.mRoot = true;
		search.mPos = 0;

		return search;
	}

	// split archiveInternalPath into parts
	std::vector<std::string> pathParts;
	std::string temp = archiveInternalPath;
	while (!temp.empty())
	{
		size_t pos = temp.find('/');
		if (pos == std::string::npos)
		{
			pathParts.push_back(temp);
			break;
		}
		pathParts.push_back(temp.substr(0, pos));
		temp = temp.substr(pos + 1);
	}

	DirSearch search;
	search.mRoot = false;
	search.mFound = false;
	search.mPos = 0;

	// go through data headers temp file and find the file with the given path
	// check if the file is a directory
	// if it is not a directory, mDirectory is false
	std::fstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary | std::ios::in | std::ios::out);
	std::fstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary | std::ios::in | std::ios::out);
	if (dataHeadersTemp.is_open() && dataHeadersPositionsTemp.is_open())
	{
		// TODO: search for the directory
		
		// load sequentially positions of file headers from data_headers_positions.teatemp
		// check if last string in pathParts matches the name of the file
		// if it does, check if the file is a directory
		// do this until the last string in pathParts is found
		// if it is not found, return search with mFound = false
		// if it is found, return search with mFound = true and mDirectory = true
		// if it is found and it is not a directory, return search with mFound = true and mDirectory = false
	}
	else
	{
		std::cerr << "Failed to open data headers temp file\n";
	}

	return search;
}

bool TEA::add(const std::string &path, const std::string &archiveInternalPath, bool encrypted, bool compressed, int method, int strength, bool directory, const std::vector<bool> &additionalFlags)
{
	// TODO: add file entry to data_headers.teatemp, data_headers_positions.teatemp, copy file to data.teatemp, update common_flags.teatemp

	if (path.empty() && !directory)
	{
		std::cerr << "Path is empty\n";
		return false;
	}

	// check if file exists
	if (!std::filesystem::exists(path) && !path.empty())
	{
		std::cerr << "File does not exist\n";
		return false;
	}

	size_t pos = 0;
	size_t size = 0;
	if (!directory)
	{
		// copy file to data.teatemp
		std::fstream dataTemp(".data.teatemp", std::ios::binary | std::ios::in | std::ios::out);
		std::ifstream file(path, std::ios::binary);
		file.seekg(0, std::ios::end);
		size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<uint8_t> data(size);
		file.read(reinterpret_cast<char*>(&data[0]), size);
		dataTemp.seekp(0, std::ios::end);
		pos = dataTemp.tellp();
		dataTemp.write(reinterpret_cast<char*>(&data[0]), size);
		dataTemp.close();
		file.close();
	}

	// create file header
	FileHeader fileHeader;
	fileHeader.mSizeName = archiveInternalPath.size();
	fileHeader.mName = archiveInternalPath;
	fileHeader.mSizeData = size;
	fileHeader.mPosParent = 0;
	fileHeader.mOffsetData = pos;
	fileHeader.mEpochModTime = path.empty() ? getCurrentEpochTime() : getFileModEpochTime(path);
	fileHeader.mReserved = 0;

	std::vector<std::string> pathParts;
	std::string temp = archiveInternalPath;
	while (!temp.empty())
	{
		size_t pos = temp.find('/');
		if (pos == std::string::npos)
		{
			pathParts.push_back(temp);
			break;
		}
		pathParts.push_back(temp.substr(0, pos));
		temp = temp.substr(pos + 1);
	}
	// example internal path: "dir1/dir2/file"
	// dirInternalPath: "dir1/dir2"
	std::string dirInternalPath = "";
	for (size_t i = 0; i < pathParts.size() - 1; ++i)
	{
		dirInternalPath += pathParts[i] + "/";
	}

	DirSearch search = findDirectory(dirInternalPath);

	if (!search.mFound)
	{
		// create directory entry
		if (!TEA::add("", dirInternalPath, encrypted, compressed, method, strength, true, additionalFlags))
		{
			std::cerr << "Failed to create directory entry\n";
			return false;
		}
		search = findDirectory(dirInternalPath);
	}
	if (search.mFound)
	{
		if (search.mDirectory)
		{
			fileHeader.mPosParent = search.mPos;

			// add file entry to data_headers.teatemp

			std::fstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary | std::ios::in | std::ios::out);
			dataHeadersTemp.seekp(0, std::ios::end);
			size_t pos = dataHeadersTemp.tellp();
			dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader), sizeof(FileHeader));
			dataHeadersTemp.close();

			// add file position to data_headers_positions.teatemp
			std::fstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary | std::ios::in | std::ios::out);
			dataHeadersPositionsTemp.seekp(0, std::ios::end);
			dataHeadersPositionsTemp.write(reinterpret_cast<char*>(&pos), sizeof(uint64_t));
			dataHeadersPositionsTemp.close();

			// TODO: update common_flags.teatemp
		}
		else
		{
			std::cerr << "Specified internal path is invalid\n";
			return false;
		}
	}

	// TODO: changes to archive header

	if (directory)
	{
		std::cout << "Added " << archiveInternalPath << " to " << mName << " as a directory\n";
	}
	else
	{
		std::cout << "Added " << path << " to " << mName << " at " << archiveInternalPath << "\n";
	}
	
	return true;
}
