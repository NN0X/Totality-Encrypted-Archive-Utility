#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "archive.h"

TEA::TEA(std::string path, std::string name) : mName(name), mPath(path)
{
	init();
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
}

void TEA::load()
{
	size_t archiveSize = 0;
	std::ifstream archive(mPath, std::ios::binary | std::ios::ate);
	if (archive.is_open())
	{
		archiveSize = archive.tellg();
		archive.seekg(0, std::ios::beg);
	}
	else
	{
		std::cout << "Failed to open archive\n";
		return;
	}

	// load 3 signature bytes and check if they match
	std::string signature;
	signature.resize(3);
	archive.read(&signature[0], 3);
	if (signature != TEA_SIGNATURE)
	{
		std::cout << "Invalid signature\n";
		return;
	}
	// skip 1 byte padding
	archive.seekg(1, std::ios::cur);

	// check version
	uint8_t version;
	archive.read(reinterpret_cast<char*>(&version), sizeof(uint8_t));
	if (version != TEA_VERSION)
	{
		std::cout << "Invalid version\n";
		return;
	}

	// move to the end of the file
	archive.seekg(archiveSize - 3, std::ios::beg);

	// check signature at the end of the file
	std::string signatureEnd;
	signatureEnd.resize(3);
	archive.read(&signatureEnd[0], 3);
	if (signatureEnd != TEA_SIGNATURE)
	{
		std::cout << "Invalid signature at the end of the file\n";
		return;
	}

	// read archive header
	archive.seekg(archiveSize - sizeof(ArchiveHeader) - 4, std::ios::beg);
	std::vector<uint8_t> headerData(sizeof(ArchiveHeader));
	archive.read(reinterpret_cast<char*>(&headerData[0]), sizeof(ArchiveHeader));

	// serialize archive header
	mArchiveHeader.numFiles = *reinterpret_cast<uint64_t*>(&headerData[0]);
	mArchiveHeader.posDataHeaders = *reinterpret_cast<uint64_t*>(&headerData[8]);
	mArchiveHeader.numUniqueFlags = *reinterpret_cast<uint16_t*>(&headerData[16]);
	mArchiveHeader.posCommonFlags = *reinterpret_cast<uint64_t*>(&headerData[18]);
	mArchiveHeader.sizeMetadata = *reinterpret_cast<uint16_t*>(&headerData[26]);
	mArchiveHeader.posMetadata = *reinterpret_cast<uint64_t*>(&headerData[28]);
	mArchiveHeader.globalFlags = *reinterpret_cast<uint16_t*>(&headerData[36]);
	mArchiveHeader.reserved = *reinterpret_cast<uint32_t*>(&headerData[38]);

	// move to metadata
	archive.seekg(mArchiveHeader.posMetadata, std::ios::beg);
	mMetadata.resize(mArchiveHeader.sizeMetadata);
	archive.read(&mMetadata[0], mArchiveHeader.sizeMetadata);
	
	// move to common flags
	archive.seekg(mArchiveHeader.posCommonFlags, std::ios::beg);
	mCommonFlagsCached.resize(mArchiveHeader.numFiles);
	size_t commonFlagsSize = mArchiveHeader.numFiles * mArchiveHeader.numUniqueFlags / 8;
	archive.read(reinterpret_cast<char*>(&mCommonFlagsCached[0]), commonFlagsSize);

	// load end of data headers
	archive.seekg(mArchiveHeader.posDataHeaders, std::ios::beg);
	archive.read(reinterpret_cast<char*>(&mDataHeader.posEndFileHeaders), sizeof(uint64_t));

	// cache 1024 file headers and corresponding positions from the start of the data headers
	archive.seekg(mArchiveHeader.posDataHeaders + sizeof(uint64_t), std::ios::beg);
	archive.read(reinterpret_cast<char*>(&mDataHeader.mFileHeadersCached[0]), 1024 * sizeof(FileHeader));
	archive.read(reinterpret_cast<char*>(&mDataHeader.mPosFileHeaders[0]), 1024 * sizeof(uint64_t));

	// split archive file into temporary files
	// 1. data
	// 2. data headers
	// 3. common flags
	
	// create data.teatemp
	std::ofstream dataTemp("data.teatemp", std::ios::binary);
	if (dataTemp.is_open())
	{
		archive.seekg(6, std::ios::beg);
		size_t endData = mArchiveHeader.posDataHeaders - 1;
		size_t dataSize = endData - archive.tellg();
		// copy data in chunks of 1024 bytes to data.teatemp while deleting it from the archive
		for (size_t i = 0; i < dataSize; i += 1024)
		{
			if (dataSize - i < 1024)
			{
				std::vector<uint8_t> data(dataSize - i);
				archive.read(reinterpret_cast<char*>(&data[0]), dataSize - i);
				dataTemp.write(reinterpret_cast<char*>(&data[0]), dataSize - i);
				// TODO: overwrite copied chunk by moving the rest of the archive
			}
			else
			{
				std::vector<uint8_t> data(1024);
				archive.read(reinterpret_cast<char*>(&data[0]), 1024);
				dataTemp.write(reinterpret_cast<char*>(&data[0]), 1024);
				// TODO: overwrite copied chunk by moving the rest of the archive
			}
		}
	}
	else
	{
		std::cout << "Failed to create data.teatemp\n";
		return;
	}

	// create data_headers.teatemp
	std::ofstream dataHeadersTemp("data_headers.teatemp", std::ios::binary);
	if (dataHeadersTemp.is_open())
	{
		archive.seekg(mArchiveHeader.posDataHeaders, std::ios::beg);
		size_t dataSize = mArchiveHeader.posEndFileHeaders - mArchiveHeader.posDataHeaders;
		// copy data headers in chunks of 1024 bytes to data_headers.teatemp while deleting it from the archive
		for (size_t i = 0; i < dataSize; i += 1024)
		{
			if (dataSize - i < 1024)
			{
				std::vector<uint8_t> data(dataSize - i);
				archive.read(reinterpret_cast<char*>(&data[0]), dataSize - i);
				dataHeadersTemp.write(reinterpret_cast<char*>(&data[0]), dataSize - i);
				// TODO: overwrite copied chunk by moving the rest of the archive
			}
			else
			{
				std::vector<uint8_t> data(1024);
				archive.read(reinterpret_cast<char*>(&data[0]), 1024);
				dataHeadersTemp.write(reinterpret_cast<char*>(&data[0]), 1024);
				// TODO: overwrite copied chunk by moving the rest of the archive
			}
		}
	}
	else
	{
		std::cout << "Failed to create data_headers.teatemp\n";
		return;
	}

	// create common_flags.teatemp
	std::ofstream commonFlagsTemp("common_flags.teatemp", std::ios::binary);
	if (commonFlagsTemp.is_open())
	{
		archive.seekg(mArchiveHeader.posCommonFlags, std::ios::beg);
		size_t dataSize = mArchiveHeader.numFiles * mArchiveHeader.numUniqueFlags / 8;
		// copy common flags in chunks of 1024 bytes to common_flags.teatemp while deleting it from the archive
		for (size_t i = 0; i < dataSize; i += 1024)
		{
			if (dataSize - i < 1024)
			{
				std::vector<uint8_t> data(dataSize - i);
				archive.read(reinterpret_cast<char*>(&data[0]), dataSize - i);
				commonFlagsTemp.write(reinterpret_cast<char*>(&data[0]), dataSize - i);
				// TODO: overwrite copied chunk by moving the rest of the archive
			}
			else
			{
				std::vector<uint8_t> data(1024);
				archive.read(reinterpret_cast<char*>(&data[0]), 1024);
				commonFlagsTemp.write(reinterpret_cast<char*>(&data[0]), 1024);
				// TODO: overwrite copied chunk by moving the rest of the archive
			}
		}
	}
	else
	{
		std::cout << "Failed to create common_flags.teatemp\n";
		return;
	}
	archive.close();

	std::cout << "Loaded archive " << mName << " from " << mPath << "\n";
}

void TEA::save()
{
	// TODO: merge temporary files back into the archive
	std::cout << "Saved archive to " << mPath << "\n";
}

void TEA::add(std::string path, std::string archiveInternalPath)
{
	// TODO: add file entry to data_headers.teatemp, copy file to data.teatemp, update common_flags.teatemp
}
