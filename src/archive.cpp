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
	mDataHeader.mFileHeaders = std::vector<FileHeader>();
	mDataHeader.mFileHeaders.reserve(1024);
	mDataHeader.mPosFileHeaders = std::vector<uint64_t>();
	mDataHeader.mPosFileHeaders.reserve(1024);

	mCommonFlags = std::vector<uint8_t>();
	mCommonFlags.reserve(1024);
	mMetadata = "";
}

void TEA::load()
{

}

void TEA::save()
{
	// THIS NEEDS CHANGING TO USE TEMP FILES
	std::vector<uint8_t> archiveHeaderBin = mArchiveHeader.toBytes();
	std::vector<uint8_t> dataHeaderBin = mDataHeader.toBytes();

	std::ofstream file(mPath, std::ios::binary);
	file.write(mSignature.c_str(), mSignature.size());
	file.write((char*)TEA_PADDING, 1);
	file.write((char*)&mVersion, sizeof(mVersion));
	file.write((char*)TEA_PADDING, 1);
	file.write((char*)mData.data(), mData.size());
	file.write((char*)TEA_PADDING, 1);
	file.write((char*)dataHeaderBin.data(), dataHeaderBin.size());
	file.write((char*)TEA_PADDING, 1);
	file.write((char*)mCommonFlags.data(), mCommonFlags.size());
	file.write((char*)TEA_PADDING, 1);
	file.write(mMetadata.c_str(), mMetadata.size());
	file.write((char*)TEA_PADDING, 1);
	file.write((char*)archiveHeaderBin.data(), archiveHeaderBin.size());
	file.write((char*)TEA_PADDING, 1);
	file.write(mSignature.c_str(), mSignature.size());
	file.close();

	std::cout << "Saved archive to " << mPath << "\n";
}

void TEA::add(std::string path, std::string archiveInternalPath)
{

}
