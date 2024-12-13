#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#include "archive.h"

TEA::TEA(const std::string &path, const std::string &name) : mName(name), mPath(path)
{
	init();
}

void TEA::init()
{
	// TODO: create temporary files for data, data headers, common flags

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
	std::fstream archive(mPath, std::ios::binary | std::ios::in | std::ios::out);
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
	// TODO: fix loading of temp files to be in-place

	std::fstream archive(mPath, std::ios::binary | std::ios::in | std::ios::out);

	// write signature
	archive.write(TEA_SIGNATURE, 3);
	archive.put(0);
	// write version
	archive.put(TEA_VERSION);
	archive.put(0);

	// load data.teatemp
	std::ifstream dataTemp(".data.teatemp", std::ios::binary);
	if (dataTemp.is_open())
	{
		dataTemp.seekg(0, std::ios::beg);
		std::vector<uint8_t> data(1024);
		while (dataTemp.read(reinterpret_cast<char*>(&data[0]), 1024))
		{
			archive.write(reinterpret_cast<char*>(&data[0]), 1024);
		}
		dataTemp.close();
	}
	archive.put(0);

	// load data_headers.teatemp
	std::ifstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary);
	if (dataHeadersTemp.is_open())
	{
		dataHeadersTemp.seekg(0, std::ios::beg);
		std::vector<uint8_t> data(1024);
		while (dataHeadersTemp.read(reinterpret_cast<char*>(&data[0]), 1024))
		{
			archive.write(reinterpret_cast<char*>(&data[0]), 1024);
		}
		dataHeadersTemp.close();
	}
	archive.put(0);

	// load common_flags.teatemp
	std::ifstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary);
	if (commonFlagsTemp.is_open())
	{
		commonFlagsTemp.seekg(0, std::ios::beg);
		std::vector<uint8_t> data(1024);
		while (commonFlagsTemp.read(reinterpret_cast<char*>(&data[0]), 1024))
		{
			archive.write(reinterpret_cast<char*>(&data[0]), 1024);
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
	std::filesystem::remove(".common_flags.teatemp");

	std::cout << "Saved archive to " << mPath << "\n";

	return true;
}

bool TEA::add(const std::string &path, const std::string &archiveInternalPath)
{
	// TODO: add file entry to data_headers.teatemp, copy file to data.teatemp, update common_flags.teatemp
	
	std::cout << "Added " << path << " to " << mName << " at " << archiveInternalPath << "\n";
	
	return true;
}
