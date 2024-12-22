#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <bitset>

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
	size_t readPos = pos + size;
	if (readPos > fileSize)
	{
		std::cerr << "Invalid position or size\n";
		return false;
	}
	std::vector<uint8_t> data(1024);

	size_t remaining = fileSize - pos - size;
	size_t writePos = pos;
	
	while (remaining > 0)
	{
		size_t readSize = remaining > 1024 ? 1024 : remaining;
		file.seekg(readPos, std::ios::beg);
		if (!file.read(reinterpret_cast<char*>(&data[0]), readSize))
		{
			std::cerr << "Failed to read data: " << path << "\n";
			return false;
		}
		file.seekp(writePos, std::ios::beg);
		if (!file.write(reinterpret_cast<char*>(&data[0]), readSize))
		{
			std::cerr << "Failed to write data: " << path << "\n";
			return false;
		}
		readPos += readSize;
		writePos += readSize;
		remaining -= readSize;
	}

	std::filesystem::resize_file(path, fileSize - size);

	return true;
}

bool moveFileDataInPlace(std::fstream &fileOrig, std::ofstream &fileTarget, const std::string &pathOrig, size_t dataSize, size_t chunkSize)
{
	// copy data in chunks of 1024 bytes to fileTarget while deleting it from the fileOrig
	size_t pos;
	for (size_t i = 0; i < dataSize; i += chunkSize)
	{
		if (dataSize - i < chunkSize)
		{
			std::vector<uint8_t> data(dataSize - i);
			fileOrig.read(reinterpret_cast<char*>(&data[0]), dataSize - i);
			pos = fileOrig.tellg();
			fileOrig.seekg(pos - (dataSize - i), std::ios::beg);
			fileTarget.write(reinterpret_cast<char*>(&data[0]), dataSize - i);
			if (!deleteFileChunk(fileOrig, fileOrig.tellg(), dataSize - i, pathOrig))
			{
				std::cerr << "Failed to delete data chunk\n";
				return false;
			}
		}
		else
		{
			std::vector<uint8_t> data(chunkSize);
			fileOrig.read(reinterpret_cast<char*>(&data[0]), chunkSize);
			pos = fileOrig.tellg();
			fileOrig.seekg(pos - chunkSize, std::ios::beg);
			fileTarget.write(reinterpret_cast<char*>(&data[0]), chunkSize);
			if (!deleteFileChunk(fileOrig, fileOrig.tellg(), chunkSize, pathOrig))
			{
				std::cerr << "Failed to delete data chunk\n";
				return false;
			}
		}
	}
	return true;
}

bool TEA::load()
{
	size_t archiveSize = 0;
	std::string fullPath = mPath + "/" + mName + ".tea";
	std::fstream archive(fullPath, std::ios::binary | std::ios::in | std::ios::out);
	if (archive.is_open())
	{
		archive.seekg(0, std::ios::end);
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

	archive.seekg(-3, std::ios::end);

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
	archive.seekg(archiveSize - 42 - 4, std::ios::beg);
	archive.read(reinterpret_cast<char*>(&mArchiveHeader.mNumFiles), sizeof(uint64_t));
	archive.read(reinterpret_cast<char*>(&mArchiveHeader.mPosDataHeaders), sizeof(uint64_t));
	archive.read(reinterpret_cast<char*>(&mArchiveHeader.mNumUniqueFlags), sizeof(uint16_t));
	archive.read(reinterpret_cast<char*>(&mArchiveHeader.mPosCommonFlags), sizeof(uint64_t));
	archive.read(reinterpret_cast<char*>(&mArchiveHeader.mSizeMetadata), sizeof(uint16_t));
	archive.read(reinterpret_cast<char*>(&mArchiveHeader.mPosMetadata), sizeof(uint64_t));
	archive.read(reinterpret_cast<char*>(&mArchiveHeader.mGlobalFlags), sizeof(uint16_t));
	archive.read(reinterpret_cast<char*>(&mArchiveHeader.mReserved), sizeof(uint32_t));

	// move to metadata
	if (mArchiveHeader.mSizeMetadata != 0)
	{
		archive.seekg(mArchiveHeader.mPosMetadata, std::ios::beg);
		mMetadata.resize(mArchiveHeader.mSizeMetadata);
		archive.read(&mMetadata[0], mArchiveHeader.mSizeMetadata);
	}

	// move to common flags
	archive.seekg(mArchiveHeader.mPosCommonFlags, std::ios::beg);
	mCommonFlagsCached.resize(mArchiveHeader.mNumFiles);
	double commonFlagsSizeFloating = (mArchiveHeader.mNumFiles * mArchiveHeader.mNumUniqueFlags) / 8.0;
	// round up to the nearest integer
	size_t commonFlagsSize = static_cast<size_t>(commonFlagsSizeFloating + 0.5);
	archive.read(reinterpret_cast<char*>(&mCommonFlagsCached[0]), commonFlagsSize);

	// load end of data headers
	archive.seekg(mArchiveHeader.mPosDataHeaders, std::ios::beg);
	archive.read(reinterpret_cast<char*>(&mDataHeader.mPosEndFileHeaders), sizeof(uint64_t));

	// cache 1024 file headers and corresponding positions from the start of the data headers
	archive.seekg(mArchiveHeader.mPosDataHeaders + sizeof(uint64_t), std::ios::beg);
	size_t numFileHeaders = mArchiveHeader.mNumFiles > 1024 ? 1024 : mArchiveHeader.mNumFiles;
	mDataHeader.mFileHeadersCached.resize(numFileHeaders);

	for (size_t i = 0; i < numFileHeaders; ++i)
	{
		uint16_t sizeName;
		archive.read(reinterpret_cast<char*>(&sizeName), sizeof(uint16_t));
		mDataHeader.mFileHeadersCached[i].mSizeName = sizeName;
		mDataHeader.mFileHeadersCached[i].mName.resize(sizeName);
		archive.read(&mDataHeader.mFileHeadersCached[i].mName[0], sizeName);
		archive.read(reinterpret_cast<char*>(&mDataHeader.mFileHeadersCached[i].mSizeData), sizeof(uint64_t));
		archive.read(reinterpret_cast<char*>(&mDataHeader.mFileHeadersCached[i].mPosParent), sizeof(uint64_t));
		archive.read(reinterpret_cast<char*>(&mDataHeader.mFileHeadersCached[i].mOffsetData), sizeof(uint64_t));
		archive.read(reinterpret_cast<char*>(&mDataHeader.mFileHeadersCached[i].mEpochModTime), sizeof(uint64_t));
		archive.read(reinterpret_cast<char*>(&mDataHeader.mFileHeadersCached[i].mReserved), sizeof(uint8_t));
	}

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
		if (!moveFileDataInPlace(archive, dataTemp, fullPath, dataSize, 1024))
		{
			std::cerr << "Failed to move data in place\n";
			return false;
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
		if (!moveFileDataInPlace(archive, dataHeadersTemp, fullPath, dataSize, 1024))
		{
			std::cerr << "Failed to move data in place\n";
			return false;
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
		if (!moveFileDataInPlace(archive, dataHeadersPositionsTemp, fullPath, dataSize, 1024))
		{
			std::cerr << "Failed to move data in place\n";
			return false;
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
		if (!moveFileDataInPlace(archive, commonFlagsTemp, fullPath, dataSize, 1024))
		{
			std::cerr << "Failed to move data in place\n";
			return false;
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
		if (!moveFileDataInPlace(dataTemp, archive, ".data.teatemp", size, 1024))
		{
			std::cerr << "Failed to move data in place\n";
			return false;
		}
		dataTemp.close();
	}
	archive.put(0);

	// load data_headers.teatemp
	std::fstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary | std::ios::in | std::ios::out);
	if (dataHeadersTemp.is_open())
	{
		dataHeadersTemp.seekg(0, std::ios::end);
		uint64_t size = dataHeadersTemp.tellg();
		archive.seekp(0, std::ios::end);
		uint64_t posDataHeaders = archive.tellp();
		mArchiveHeader.mPosDataHeaders = posDataHeaders;
		mDataHeader.mPosEndFileHeaders = size + 8;
		dataHeadersTemp.seekg(0, std::ios::beg);
		archive.write(reinterpret_cast<char*>(&mDataHeader.mPosEndFileHeaders), sizeof(uint64_t));
		if (!moveFileDataInPlace(dataHeadersTemp, archive, ".data_headers.teatemp", size, 1024))
		{
			std::cerr << "Failed to move data headers in place\n";
			return false;
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
		if (!moveFileDataInPlace(dataHeadersPositionsTemp, archive, ".data_headers_positions.teatemp", size, 1024))
		{
			std::cerr << "Failed to move data headers positions in place\n";
			return false;
		}
		dataHeadersPositionsTemp.close();
	}
	
	archive.put(0);

	// load common_flags.teatemp
	std::fstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary | std::ios::in | std::ios::out);
	if (commonFlagsTemp.is_open())
	{
		commonFlagsTemp.seekg(0, std::ios::end);
		size_t size = commonFlagsTemp.tellg();
		archive.seekp(0, std::ios::end);
		mArchiveHeader.mPosCommonFlags = archive.tellp();
		commonFlagsTemp.seekg(0, std::ios::beg);
		if (!moveFileDataInPlace(commonFlagsTemp, archive, ".common_flags.teatemp", size, 1024))
		{
			std::cerr << "Failed to move common flags in place\n";
			return false;
		}
		commonFlagsTemp.close();
	}
	archive.put(0);

	archive.seekp(0, std::ios::end);
	uint64_t posMetadata = archive.tellp();
	mArchiveHeader.mPosMetadata = posMetadata;

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

// TODO: test if epoch time is correct
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

	uint64_t pos = 0;
	uint64_t size = 0;
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

	// create file header
	FileHeader fileHeader;
	fileHeader.mSizeName = pathParts.back().size();
	fileHeader.mName = pathParts.back();
	fileHeader.mSizeData = size;
	fileHeader.mOffsetData = pos;
	fileHeader.mEpochModTime = path.empty() ? getCurrentEpochTime() : getFileModEpochTime(path);
	fileHeader.mReserved = 0;

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
	else
	{
		if (search.mDirectory || search.mRoot)
		{
			fileHeader.mPosParent = search.mPos;

			// add file entry to data_headers.teatemp

			std::fstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary | std::ios::in | std::ios::out);
			dataHeadersTemp.seekp(0, std::ios::end);
			size_t pos = dataHeadersTemp.tellp();
			dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mSizeName), sizeof(uint16_t));
			dataHeadersTemp.write(fileHeader.mName.c_str(), fileHeader.mSizeName);
			dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mSizeData), sizeof(uint64_t));
			dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mPosParent), sizeof(uint64_t));
			dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mOffsetData), sizeof(uint64_t));
			dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mEpochModTime), sizeof(uint64_t));
			dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mReserved), sizeof(uint8_t));
			dataHeadersTemp.close();

			// add file position to data_headers_positions.teatemp
			std::fstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary | std::ios::in | std::ios::out);
			dataHeadersPositionsTemp.seekp(0, std::ios::end);
			dataHeadersPositionsTemp.write(reinterpret_cast<char*>(&pos), sizeof(uint64_t));
			dataHeadersPositionsTemp.close();

			// add flags to common_flags.teatemp
			// TODO: check if common_flags.teatemp is correct
			std::fstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary | std::ios::in | std::ios::out);
			commonFlagsTemp.seekp(0, std::ios::end);
			uint8_t flagsRequired = 0;
			flagsRequired |= encrypted ? 0b00000001 : 0;
			flagsRequired |= compressed ? 0b00000010 : 0;
			flagsRequired |= strength & 0b00001100;
			flagsRequired |= method & 0b00110000;
			flagsRequired |= directory ? 0b01000000 : 0;
			if (additionalFlags.size() + 7 > mArchiveHeader.mNumUniqueFlags)
			{
				std::cerr << "Too many additional flags\n";
				return false;
			}
			if (additionalFlags.size() >= 1)
			{
				flagsRequired |= additionalFlags[0] ? 0b10000000 : 0;
			}
			commonFlagsTemp.write(reinterpret_cast<char*>(&flagsRequired), 1);
			if (additionalFlags.size() + 7 > 8)
			{
				std::vector<uint8_t> flagsTemp;
				flagsTemp.push_back(0);
				size_t j = 0;
				for (size_t i = 1; i < additionalFlags.size(); ++i)
				{
					if (i % 8 == 0)
					{
						flagsTemp.push_back(0);
					}
					flagsTemp[j] |= additionalFlags[i] ? 1 << (i % 8) : 0;
					++j;
				}
				if (flagsTemp.size() > 1)
				{
					commonFlagsTemp.write(reinterpret_cast<char*>(&flagsTemp[0]), flagsTemp.size());
				}
			}
			commonFlagsTemp.close();
		}
		else
		{
			std::cerr << "Specified internal path is invalid\n";
			return false;
		}
	}

	// TODO: changes to archive header

	mArchiveHeader.mNumFiles++;

	if (directory)
	{
		std::cout << "Added " << archiveInternalPath << " to " << mName << " as a directory\n";
	}
	else
	{
		std::cout << "Added " << path << " to " << mName << " as " << archiveInternalPath << "\n";
	}
	
	return true;
}

// TODO: implement file structure into list
bool TEA::list()
{
	std::ifstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary);
	std::ifstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary);

	if (dataHeadersTemp.is_open() && dataHeadersPositionsTemp.is_open())
	{
		uint64_t pos;
		uint16_t nameSize;
		std::string name;
		for (size_t _ = 0; _ < mArchiveHeader.mNumFiles; _++)
		{
			dataHeadersPositionsTemp.read(reinterpret_cast<char*>(&pos), sizeof(uint64_t));
			dataHeadersTemp.seekg(pos, std::ios::beg);
			dataHeadersTemp.read(reinterpret_cast<char*>(&nameSize), sizeof(uint16_t));
			name.resize(nameSize);
			dataHeadersTemp.read(&name[0], nameSize);
			std::cout << name << "\n";
		}
		return true;
	}
	else
	{
		std::cerr << "Failed to open data headers temp files\n";
		return false;
	}
}

// print number of files, size, etc.
bool TEA::info()
{
	std::cout << "Archive: " << mName << "\n";
	std::cout << "Number of files: " << mArchiveHeader.mNumFiles << "\n";
	std::cout << "Metadata: " << mMetadata << "\n";
	// cout flags in binary format
	std::cout << "Global flags: " << std::bitset<16>(mArchiveHeader.mGlobalFlags) << "\n";
	std::cout << "Reserved: " << mArchiveHeader.mReserved << "\n";

	return true;
}


void TEA::setName(const std::string &name)
{
	mName = name;
}

// TODO: check if setMetadata works
void TEA::setMetadata(const std::string &metadata)
{
	mMetadata = metadata;
	mArchiveHeader.mSizeMetadata = metadata.size();
}
