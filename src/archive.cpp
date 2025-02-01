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
        mArchiveHeader.mNumUniqueFlags = TEA_FLAGS_RESERVED_START_BIT;
        mArchiveHeader.mPosCommonFlags = 0;
        mArchiveHeader.mSizeMetadata = 0;
        mArchiveHeader.mPosMetadata = 0;
        mArchiveHeader.mGlobalFlags = 0b0000000000000000; // default flags
        mArchiveHeader.mReserved = 0;

        mDataHeader.mPosEndFileHeaders = 0;
        mDataHeader.mFileHeadersCached = std::unordered_map<uint64_t, FileHeader>();
        mDataHeader.mFileHeadersCached.reserve(DEFAULT_CACHE_SIZE);
        mDataHeader.mPosFileHeaders = std::vector<uint64_t>();
        mDataHeader.mPosFileHeaders.reserve(DEFAULT_CACHE_SIZE);

        mDataCached = std::vector<uint8_t>();
        mDataCached.reserve(DEFAULT_CACHE_SIZE);

        mCommonFlagsCached = std::vector<uint8_t>();
        mCommonFlagsCached.reserve(DEFAULT_CACHE_SIZE);

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

bool deleteFileChunk(std::fstream& file, uint64_t pos, uint64_t size, const std::string &path)
{
        file.seekg(0, std::ios::end);
        uint64_t fileSize = file.tellg();
        uint64_t readPos = pos + size;
        if (readPos > fileSize)
        {
                std::cerr << "Invalid position or size\n";
                return false;
        }
        std::vector<uint8_t> data(DEFAULT_CHUNK_SIZE);

        uint64_t remaining = fileSize - pos - size;
        uint64_t writePos = pos;

        while (remaining > 0)
        {
                uint64_t readSize = remaining > DEFAULT_CHUNK_SIZE ? DEFAULT_CHUNK_SIZE : remaining;
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

        file.close();
        std::filesystem::resize_file(path, fileSize - size);
        file.open(path, std::ios::binary | std::ios::in | std::ios::out);

        return true;
}

bool moveFileDataInPlace(std::fstream &fileOrig, std::ofstream &fileTarget, const std::string &pathOrig, uint64_t dataSize, uint64_t chunkSize)
{
        uint64_t pos;
        for (uint64_t i = 0; i < dataSize; i += chunkSize)
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
        uint64_t archiveSize = 0;
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

        if (archiveSize < TEA_HEADER_SIZE + 2 * TEA_SIGNATURE_SIZE + 7 * TEA_PADDING_SIZE + TEA_VERSION_SIZE)
        {
                std::cerr << "Invalid archive size\n";
                return false;
        }

        // load 3 signature bytes and check if they match
        std::string signature;
        signature.resize(TEA_SIGNATURE_SIZE);
        archive.read(&signature[0], TEA_SIGNATURE_SIZE);
        if (signature != TEA_SIGNATURE)
        {
                std::cerr << "Invalid signature\n";
                return false;
        }
        // skip 1 byte padding
        archive.seekg(TEA_PADDING_SIZE, std::ios::cur);

        // check version
        uint8_t version;
        archive.read(reinterpret_cast<char*>(&version), sizeof(uint8_t));
        if (version != TEA_VERSION)
        {
                std::cerr << "Invalid version\n";
                return false;
        }

        archive.seekg(-TEA_SIGNATURE_SIZE, std::ios::end);

        // check signature at the end of the file
        std::string signatureEnd;
        signatureEnd.resize(TEA_SIGNATURE_SIZE);
        archive.read(&signatureEnd[0], TEA_SIGNATURE_SIZE);
        if (signatureEnd != TEA_SIGNATURE)
        {
                std::cerr << "Invalid signature\n";
                return false;
        }

        // read archive header
        const int offsetFromEndToArchiveHeader = TEA_HEADER_SIZE + TEA_PADDING_SIZE + TEA_SIGNATURE_SIZE;
        archive.seekg(archiveSize - offsetFromEndToArchiveHeader, std::ios::beg);
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

        // split archive file into temporary files
        // 1. data
        // 2. data headers
        // 3. common flags

        // create data.teatemp
        uint64_t offset = 0;
        std::ofstream dataTemp(".data.teatemp", std::ios::binary);

        const int offsetFromBeginToData = TEA_SIGNATURE_SIZE + 2 * TEA_PADDING_SIZE + TEA_VERSION_SIZE;
        if (dataTemp.is_open())
        {
                archive.seekg(offsetFromBeginToData, std::ios::beg);
                uint64_t endData = mArchiveHeader.mPosDataHeaders - TEA_PADDING_SIZE;
                uint64_t dataSize = endData - offsetFromBeginToData;
                offset = dataSize;
                if (!moveFileDataInPlace(archive, dataTemp, fullPath, dataSize, DEFAULT_CHUNK_SIZE))
                {
                        std::cerr << "Failed to move data in place\n";
                        return false;
                }
                archive.close();
                archive.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
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
                // load end of file headers
                archive.seekg(mArchiveHeader.mPosDataHeaders - offset, std::ios::beg);
                archive.read(reinterpret_cast<char*>(&mDataHeader.mPosEndFileHeaders), sizeof(uint64_t));
                uint64_t dataSize = mDataHeader.mPosEndFileHeaders;
                offset += dataSize;
                if (!moveFileDataInPlace(archive, dataHeadersTemp, fullPath, dataSize, DEFAULT_CHUNK_SIZE))
                {
                        std::cerr << "Failed to move data in place\n";
                        return false;
                }
                archive.close();
                archive.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
                dataHeadersTemp.close();
        }
        else
        {
                std::cerr << "Failed to create .data_headers.teatemp\n";
                return false;
        }

        // create data_headers_positions.teatemp
        std::ofstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary);

        const int offsetFromDataHeadersToDataHeadersPositions = sizeof(uint64_t);
        if (dataHeadersPositionsTemp.is_open())
        {
                archive.seekg(offsetFromDataHeadersToDataHeadersPositions, std::ios::beg);
                uint64_t dataSize = mArchiveHeader.mNumFiles * sizeof(uint64_t);
                offset += dataSize;
                if (!moveFileDataInPlace(archive, dataHeadersPositionsTemp, fullPath, dataSize, DEFAULT_CHUNK_SIZE))
                {
                        std::cerr << "Failed to move data in place\n";
                        return false;
                }
                archive.close();
                archive.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
                dataHeadersPositionsTemp.close();
        }
        else
        {
                std::cerr << "Failed to create .data_headers_positions.teatemp\n";
                return false;
        }

        // create common_flags.teatemp
        std::ofstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary);

        const double bitsPerByteFloating = 8.0;
        const double half = 0.5;
        if (commonFlagsTemp.is_open())
        {
                archive.seekg(mArchiveHeader.mPosCommonFlags - offset, std::ios::beg);
                float commonFlagsSizeFloating = (mArchiveHeader.mNumFiles * mArchiveHeader.mNumUniqueFlags) / bitsPerByteFloating;
                uint64_t dataSize = static_cast<uint64_t>(commonFlagsSizeFloating + half);
                offset += dataSize;
                if (!moveFileDataInPlace(archive, commonFlagsTemp, fullPath, dataSize, DEFAULT_CHUNK_SIZE))
                {
                        std::cerr << "Failed to move data in place\n";
                        return false;
                }
                archive.close();
                archive.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
                commonFlagsTemp.close();
        }
        else
        {
                std::cerr << "Failed to create .common_flags.teatemp\n";
                return false;
        }
        archive.close();

        std::cout << "Loaded archive " << mName << " from " << mPath << "\n";

        std::filesystem::remove(fullPath);

        return true;
}

bool TEA::save()
{
        std::string fullPath = mPath + "/" + mName + ".tea";
        std::ofstream archive(fullPath, std::ios::binary | std::ios::out);

        if (!archive.is_open())
        {
                std::cerr << "Failed to open archive\n";
                return false;
        }

        // write signature
        archive.write(TEA_SIGNATURE, TEA_SIGNATURE_SIZE);
        archive.put(0);
        // write version
        archive.put(TEA_VERSION);
        archive.put(0);

        // load data.teatemp
        std::fstream dataTemp(".data.teatemp", std::ios::binary | std::ios::in | std::ios::out);
        if (dataTemp.is_open())
        {
                dataTemp.seekg(0, std::ios::end);
                uint64_t size = dataTemp.tellg();
                archive.seekp(0, std::ios::end);
                dataTemp.seekg(0, std::ios::beg);
                if (!moveFileDataInPlace(dataTemp, archive, ".data.teatemp", size, DEFAULT_CHUNK_SIZE))
                {
                        std::cerr << "Failed to move data in place\n";
                        return false;
                }
                archive.close();
                archive.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
                archive.seekp(0, std::ios::end);
                dataTemp.close();
        }
        else
        {
                std::cerr << "Failed to open data temp file\n";
                return false;
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
                mDataHeader.mPosEndFileHeaders = size;
                dataHeadersTemp.seekg(0, std::ios::beg);
                archive.write(reinterpret_cast<char*>(&mDataHeader.mPosEndFileHeaders), sizeof(uint64_t));
                if (!moveFileDataInPlace(dataHeadersTemp, archive, ".data_headers.teatemp", size, DEFAULT_CHUNK_SIZE))
                {
                        std::cerr << "Failed to move data headers in place\n";
                        return false;
                }
                archive.close();
                archive.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
                dataHeadersTemp.close();
        }
        else
        {
                std::cerr << "Failed to open data headers temp file\n";
                return false;
        }

        //load data_headers_positions.teatemp
        std::fstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary | std::ios::in | std::ios::out);
        if (dataHeadersPositionsTemp.is_open())
        {
                dataHeadersPositionsTemp.seekg(0, std::ios::end);
                uint64_t size = dataHeadersPositionsTemp.tellg();
                archive.seekp(0, std::ios::end);
                dataHeadersPositionsTemp.seekg(0, std::ios::beg);
                if (!moveFileDataInPlace(dataHeadersPositionsTemp, archive, ".data_headers_positions.teatemp", size, DEFAULT_CHUNK_SIZE))
                {
                        std::cerr << "Failed to move data headers positions in place\n";
                        return false;
                }
                archive.close();
                archive.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
                archive.seekp(0, std::ios::end);
                dataHeadersPositionsTemp.close();
        }
        else
        {
                std::cerr << "Failed to open data headers positions temp file\n";
                return false;
        }

        archive.put(0);

        // load common_flags.teatemp
        std::fstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary | std::ios::in | std::ios::out);
        if (commonFlagsTemp.is_open())
        {
                commonFlagsTemp.seekg(0, std::ios::end);
                uint64_t size = commonFlagsTemp.tellg();
                archive.seekp(0, std::ios::end);
                mArchiveHeader.mPosCommonFlags = archive.tellp();
                commonFlagsTemp.seekg(0, std::ios::beg);
                if (!moveFileDataInPlace(commonFlagsTemp, archive, ".common_flags.teatemp", size, DEFAULT_CHUNK_SIZE))
                {
                        std::cerr << "Failed to move common flags in place\n";
                        return false;
                }
                archive.close();
                archive.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
                archive.seekp(0, std::ios::end);
                commonFlagsTemp.close();
        }
        else
        {
                std::cerr << "Failed to open common flags temp file\n";
                return false;
        }
        archive.put(0);

        uint64_t posMetadata = archive.tellp();
        mArchiveHeader.mPosMetadata = posMetadata;

        // write metadata
        if (mMetadata.size() != 0)
        {
                archive.write(&mMetadata[0], mMetadata.size());
        }
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
        archive.write(TEA_SIGNATURE, TEA_SIGNATURE_SIZE);

        archive.close();

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
        auto modTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        uint64_t modTimeInEpoch = std::chrono::duration_cast<std::chrono::seconds>(modTime.time_since_epoch()).count();
        return modTimeInEpoch;
}

uint64_t getCurrentEpochTime()
{
        auto time = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::seconds>(time).count();
}

struct FileSearch
{
        bool mFound;
        bool mDirectory;
        uint64_t mPos; // pos of the file header
};

FileSearch findFile(const std::string &archiveInternalPath, uint64_t numUniqueFlags, uint64_t numFiles, bool skipPath)
{
        std::ifstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary);
        std::ifstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary);
        std::ifstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary);

        if (!dataHeadersTemp.is_open() || !dataHeadersPositionsTemp.is_open() || !commonFlagsTemp.is_open())
        {
                std::cerr << "Failed to open data headers temp files\n";
                return { false, false, 0 };
        }

        // split archiveInternalPath into parts
        // for example: test1/test2/test3
        // parts: test1, test2, test3
        std::vector<std::string> parts;
        std::string part;
        for (uint64_t i = 0; i < archiveInternalPath.size(); i++)
        {
                if (archiveInternalPath[i] == '/')
                {
                        parts.push_back(part);
                        part.clear();
                }
                else
                {
                        part += archiveInternalPath[i];
                }
        }
        parts.push_back(part);
        uint64_t posParent = ROOT; // no parent
        // search for directories
        uint64_t indexTemp;
        if (skipPath)
        {
                indexTemp = parts.size() < 2 ? 0 : parts.size() - 2;
        }
        else
        {
                indexTemp = 0;
        }

        const uint8_t directoryBit = 0b00000010;
        const double bitsPerByteFloating = 8.0;
        const double half = 0.5;
        for (uint64_t i = indexTemp; i < parts.size() - 1; i++)
        {
                // check if file exists
                bool found = false;
                bool directory = false;
                uint64_t pos;
                uint64_t posParentTemp; // no parent
                uint16_t sizeName;
                std::string name;
                for (uint64_t j = 0; j < numFiles; j++)
                {
                        dataHeadersPositionsTemp.seekg(j * sizeof(uint64_t), std::ios::beg);
                        dataHeadersPositionsTemp.read(reinterpret_cast<char*>(&pos), sizeof(uint64_t));
                        dataHeadersTemp.seekg(pos, std::ios::beg);
                        dataHeadersTemp.read(reinterpret_cast<char*>(&sizeName), sizeof(uint16_t));
                        name.resize(sizeName);
                        dataHeadersTemp.read(&name[0], sizeName);
                        dataHeadersTemp.seekg(sizeof(uint64_t), std::ios::cur); // skip size of data
                        dataHeadersTemp.read(reinterpret_cast<char*>(&posParentTemp), sizeof(uint64_t));
                        if (name == parts[i] && posParentTemp == posParent)
                        {
                                // check if directory
                                float commonFlagsSizeFloating = (numUniqueFlags) / bitsPerByteFloating;
                                uint64_t bytesFlags = static_cast<uint64_t>(commonFlagsSizeFloating + half);
                                std::vector<uint8_t> flags(bytesFlags);
                                commonFlagsTemp.seekg(j * bytesFlags, std::ios::beg);
                                commonFlagsTemp.read(reinterpret_cast<char*>(&flags[0]), bytesFlags);
                                uint8_t defaultFlags = flags[0];
                                directory = defaultFlags & directoryBit; // check if directory
                                if (!directory)
                                {
                                        std::cerr << parts[i] << " is not a directory\n";
                                        break;
                                }
                                found = true;
                                posParent = pos;
                                break;
                        }
                        //else if (name == parts[i] && posParentTemp != posParent)
                        //{
                        //        std::cerr << "Invalid path\n";
                        //        break;
                        //}
                }
                if (!found)
                {
                        return { false, false, 0 };
                }
        }

        // search for file
        FileSearch fileSearch;
        fileSearch.mFound = false;
        fileSearch.mDirectory = false;
        fileSearch.mPos = 0;
        uint64_t posParentTemp; // no parent
        std::string name;
        uint16_t sizeName;

        for (uint64_t j = 0; j < numFiles; j++)
        {
                dataHeadersPositionsTemp.seekg(j * sizeof(uint64_t), std::ios::beg);
                dataHeadersPositionsTemp.read(reinterpret_cast<char*>(&fileSearch.mPos), sizeof(uint64_t));
                dataHeadersTemp.seekg(fileSearch.mPos, std::ios::beg);
                dataHeadersTemp.read(reinterpret_cast<char*>(&sizeName), sizeof(uint16_t));
                name.resize(sizeName);
                dataHeadersTemp.read(&name[0], sizeName);
                dataHeadersTemp.seekg(sizeof(uint64_t), std::ios::cur); // skip size of data
                dataHeadersTemp.read(reinterpret_cast<char*>(&posParentTemp), sizeof(uint64_t));
                if (name == parts[parts.size() - 1] && posParentTemp == posParent)
                {
                        // check if directory
                        float commonFlagsSizeFloating = (numUniqueFlags) / bitsPerByteFloating;
                        uint64_t bytesFlags = static_cast<uint64_t>(commonFlagsSizeFloating + half);
                        std::vector<uint8_t> flags(bytesFlags);
                        commonFlagsTemp.seekg(j * bytesFlags, std::ios::beg);
                        commonFlagsTemp.read(reinterpret_cast<char*>(&flags[0]), bytesFlags);
                        uint8_t defaultFlags = flags[0];
                        fileSearch.mDirectory = defaultFlags & directoryBit; // check if directory
                        fileSearch.mFound = true;
                        break;
                }
                else if (name == parts[parts.size() - 1] && posParentTemp != posParent)
                {
                        std::cerr << "Invalid path\n";
                        fileSearch.mFound = false;
                        break;
                }
        }

        return fileSearch;
}

bool addFileHeader(FileHeader &fileHeader, uint64_t *pos)
{
        std::ofstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary | std::ios::in | std::ios::out);
        std::ofstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary | std::ios::in | std::ios::out);

        if (dataHeadersTemp.is_open() && dataHeadersPositionsTemp.is_open())
        {
                dataHeadersTemp.seekp(0, std::ios::end);
                dataHeadersPositionsTemp.seekp(0, std::ios::end);
                uint64_t posTemp = dataHeadersTemp.tellp();
                dataHeadersPositionsTemp.write(reinterpret_cast<char*>(&posTemp), sizeof(uint64_t));
                dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mSizeName), sizeof(uint16_t));
                dataHeadersTemp.write(&fileHeader.mName[0], fileHeader.mSizeName);
                dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mSizeData), sizeof(uint64_t));
                dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mPosParent), sizeof(uint64_t));
                dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mOffsetData), sizeof(uint64_t));
                dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mEpochModTime), sizeof(uint64_t));
                dataHeadersTemp.write(reinterpret_cast<char*>(&fileHeader.mReserved), sizeof(uint8_t));

                if (pos != nullptr)
                {
                        *pos = posTemp;
                }

                dataHeadersTemp.close();
                dataHeadersPositionsTemp.close();
        }
        else
        {
                std::cerr << "Failed to open data headers temp files\n";
                return false;
        }

        return true;
}

bool addFlags(uint64_t numUnique , bool encrypted, bool compressed, int method, int strength, bool directory, const std::vector<bool> &additionalFlags)
{
        if (additionalFlags.size() != numUnique - TEA_FLAGS_RESERVED_START_BIT)
        {
                std::cerr << "Invalid number of additional flags\n";
                return false;
        }

        std::ofstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary | std::ios::in | std::ios::out);

        const double bitsPerByteFloating = 8.0;
        const double half = 0.5;
        const int encrypedSizeOffset = TEA_ENCRYPTED_SIZE - 1;
        const int compressedSizeOffset = TEA_COMPRESSED_SIZE - 1;
        const int compressionTypeSizeOffset = TEA_COMPRESSION_TYPE_SIZE - 1;
        const int compressionStrengthSizeOffset = TEA_COMPRESSION_STRENGTH_SIZE - 1;
        const int directorySizeOffset = TEA_DIRECTORY_SIZE - 1;

        const int encryptedShift = TEA_FLAGS_RESERVED_START_BIT - TEA_ENCRYPTED_BIT - encrypedSizeOffset;
        const int compressedShift = TEA_FLAGS_RESERVED_START_BIT - TEA_COMPRESSED_BIT - compressedSizeOffset;
        const int compressionTypeShift = TEA_FLAGS_RESERVED_START_BIT - TEA_COMPRESSION_TYPE_BIT - compressionTypeSizeOffset;
        const int compressionStrengthShift = TEA_FLAGS_RESERVED_START_BIT - TEA_COMPRESSION_STRENGTH_BIT - compressionStrengthSizeOffset;
        const int directoryShift = TEA_FLAGS_RESERVED_START_BIT - TEA_DIRECTORY_BIT - directorySizeOffset;

        const int bitsPerByte = 8;
        // TODO: test if this works
        if (commonFlagsTemp.is_open())
        {
                commonFlagsTemp.seekp(0, std::ios::end);
                float commonFlagsSizeFloating = (numUnique) / bitsPerByteFloating;
                uint64_t bytesFlags = static_cast<uint64_t>(commonFlagsSizeFloating + half);
                std::vector<uint8_t> flags(bytesFlags);
                uint8_t defaultFlags = 0;
                defaultFlags |= encrypted << encryptedShift; // 7
                defaultFlags |= compressed << compressedShift; // 6
                defaultFlags |= method << compressionTypeShift; // 4
                defaultFlags |= strength << compressionStrengthShift; // 2
                defaultFlags |= directory << directoryShift; // 1

                if (additionalFlags.size() != 0)
                {
                        defaultFlags |= additionalFlags[0];
                }
                flags[0] = defaultFlags;
                uint64_t i = 1; // start from 1 because 0 is default flags
                for (uint64_t j = 1; j < additionalFlags.size(); j++)
                {
                        flags[i] |= additionalFlags[j] << (j % bitsPerByte);
                        if (j % bitsPerByte == 0)
                        {
                                i++;
                        }
                }
                commonFlagsTemp.write(reinterpret_cast<char*>(&flags[0]), bytesFlags);
                commonFlagsTemp.close();
        }
        else
        {
                std::cerr << "Failed to open common flags temp file\n";
                return false;
        }

        return true;
}

bool TEA::add(const std::string &path, const std::string &archiveInternalPath, bool encrypted, bool compressed, int method, int strength, bool directory, const std::vector<bool> &additionalFlags)
{
        if (path.empty() && !directory)
        {
                std::cerr << "Invalid path\n";
                return false;
        }

        if (archiveInternalPath.empty())
        {
                std::cerr << "Invalid archive internal path\n";
                return false;
        }

        std::vector<std::string> archiveInternalPathParts;
        std::string part;
        for (uint64_t i = 0; i < archiveInternalPath.size(); i++)
        {
                if (archiveInternalPath[i] == '/')
                {
                        archiveInternalPathParts.push_back(part);
                        part.clear();
                }
                else
                {
                        part += archiveInternalPath[i];
                }
        }
        archiveInternalPathParts.push_back(part);

        std::vector<std::string> allInternalPaths;
        allInternalPaths.reserve(archiveInternalPathParts.size());
        std::string internalPath;
        for (uint64_t i = 0; i < archiveInternalPathParts.size(); i++)
        {
                internalPath += archiveInternalPathParts[i];
                allInternalPaths.push_back(internalPath);
                internalPath += "/";
        }

        // all internal paths for example: test1/test2/test3
        // allInternalPaths[0] = test1
        // allInternalPaths[1] = test1/test2
        // allInternalPaths[2] = test1/test2/test3
        // take care of path without the file
        uint64_t parentPos = ROOT; // no parent
        for (uint64_t i = 0; i < allInternalPaths.size() - 1; i++)
        {
                std::cout << "Checking: " << allInternalPaths[i] << "\n";
                FileSearch dirSearch = findFile(allInternalPaths[i], mArchiveHeader.mNumUniqueFlags, mArchiveHeader.mNumFiles, false);
                if (!dirSearch.mFound)
                {
                        FileHeader dirHeader;
                        std::vector<std::string> parts;
                        std::string part;
                        for (uint64_t j = 0; j < allInternalPaths[i].size(); j++)
                        {
                                if (allInternalPaths[i][j] == '/')
                                {
                                        parts.push_back(part);
                                        part.clear();
                                }
                                else
                                {
                                        part += allInternalPaths[i][j];
                                }
                        }
                        parts.push_back(part);
                        dirHeader.mSizeName = parts[parts.size() - 1].size();
                        dirHeader.mName = parts[parts.size() - 1];
                        dirHeader.mSizeData = 0;
                        dirHeader.mPosParent = parentPos;
                        dirHeader.mOffsetData = 0;
                        dirHeader.mEpochModTime = getCurrentEpochTime();
                        dirHeader.mReserved = 0;
                        if (!addFileHeader(dirHeader, &dirSearch.mPos))
                        {
                                std::cerr << "Failed to add directory header\n";
                                return false;
                        }
                        if (!addFlags(mArchiveHeader.mNumUniqueFlags, encrypted, compressed, method, strength, true, additionalFlags))
                        {
                                std::cerr << "Failed to add flags\n";
                                return false;
                        }
                        std::cout << "Added directory: " << allInternalPaths[i] << "\n";
                        mArchiveHeader.mNumFiles++;
                }
                else if (!dirSearch.mDirectory)
                {
                        std::cerr << allInternalPaths[i] << " is not a directory\n";
                        return false;
                }
                parentPos = dirSearch.mPos;
        }

        // add file
        FileSearch fileSearch = findFile(archiveInternalPath, mArchiveHeader.mNumUniqueFlags, mArchiveHeader.mNumFiles, true);
        if (fileSearch.mFound)
        {
                std::cerr << "File already exists\n";
                return false;
        }

        FileHeader fileHeader;
        if (!directory)
        {
                fileHeader.mSizeName = archiveInternalPathParts[archiveInternalPathParts.size() - 1].size();
                fileHeader.mName = archiveInternalPathParts[archiveInternalPathParts.size() - 1];
                fileHeader.mPosParent = parentPos;
                fileHeader.mEpochModTime = getFileModEpochTime(path);
                fileHeader.mReserved = 0;

                // copy file data
                std::ifstream file(path, std::ios::binary);
                if (!file.is_open())
                {
                        std::cerr << "Failed to open file\n";
                        return false;
                }
                file.seekg(0, std::ios::end);
                fileHeader.mSizeData = file.tellg();
                file.seekg(0, std::ios::beg);
                std::ofstream dataTemp(".data.teatemp", std::ios::binary | std::ios::in);
                // get data offset
                dataTemp.seekp(0, std::ios::end);
                fileHeader.mOffsetData = dataTemp.tellp();
                dataTemp.seekp(0, std::ios::beg);
                for (uint64_t i = 0; i < fileHeader.mSizeData; i += DEFAULT_CHUNK_SIZE)
                {
                        std::vector<uint8_t> data(DEFAULT_CHUNK_SIZE);
                        uint64_t readSize = fileHeader.mSizeData - i < DEFAULT_CHUNK_SIZE ? fileHeader.mSizeData - i : DEFAULT_CHUNK_SIZE;
                        file.read(reinterpret_cast<char*>(&data[0]), readSize);
                        dataTemp.write(reinterpret_cast<char*>(&data[0]), readSize);
                }
                dataTemp.close();
                file.close();
        }
        else
        {
                fileHeader.mSizeName = archiveInternalPathParts[archiveInternalPathParts.size() - 1].size();
                fileHeader.mName = archiveInternalPathParts[archiveInternalPathParts.size() - 1];
                fileHeader.mPosParent = parentPos;
                if (path.empty())
                {
                        fileHeader.mEpochModTime = getCurrentEpochTime();
                }
                else
                {
                        fileHeader.mEpochModTime = getFileModEpochTime(path);
                }
                fileHeader.mSizeData = 0;
                fileHeader.mOffsetData = 0;
                fileHeader.mReserved = 0;
        }

        // add file header
        if (!addFileHeader(fileHeader, nullptr))
        {
                std::cerr << "Failed to add file header\n";
                return false;
        }

        // add flags
        if (!addFlags(mArchiveHeader.mNumUniqueFlags ,encrypted, compressed, method, strength, directory, additionalFlags))
        {
                std::cerr << "Failed to add flags\n";
                return false;
        }
        mArchiveHeader.mNumFiles++;

        return true;
}

bool padToRight(std::string &str, uint64_t size)
{
        if (str.size() > size)
        {
                return false;
        }
        else if (str.size() == size)
        {
                return true;
        }
        str = str + std::string(size - str.size(), ' ');
        return true;
}

bool TEA::list()
{
        std::ifstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary);
        std::ifstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary);
        std::ifstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary);

        if (dataHeadersTemp.is_open() && dataHeadersPositionsTemp.is_open())
        {
                std::cout << "Listing files in archive " << mName << ":\n\n";
                std::cout << " path" + std::string(32 - 5, ' ') + "type" + std::string(8 - 4, ' ') + "size" + std::string(8 - 4, ' ') + "time" + std::string(16 - 4, ' ') + "flags" + std::string(TEA_FILE_FLAGS_SIZE_BITS - 5, ' ') + " reserved\n";
                std::cout << std::string(32 + 8 + 8 + 16 + TEA_FILE_FLAGS_SIZE_BITS + TEA_FILE_RESERVED_SIZE_BITS + 2, '-') << "\n";

                uint64_t pos;
                float commonFlagsSizeFloating = mArchiveHeader.mNumUniqueFlags / 8.0;
                uint64_t bytesFlags = static_cast<uint64_t>(commonFlagsSizeFloating + 0.5);
                std::vector<uint8_t> flags(bytesFlags);
                FileHeader fileHeader;
                for (uint64_t i = 0; i < mArchiveHeader.mNumFiles; i++)
                {
                        dataHeadersPositionsTemp.read(reinterpret_cast<char*>(&pos), sizeof(uint64_t));
                        dataHeadersTemp.seekg(pos, std::ios::beg);
                        dataHeadersTemp.read(reinterpret_cast<char*>(&fileHeader.mSizeName), sizeof(uint16_t));
                        fileHeader.mName.resize(fileHeader.mSizeName);
                        dataHeadersTemp.read(&fileHeader.mName[0], fileHeader.mSizeName);
                        dataHeadersTemp.read(reinterpret_cast<char*>(&fileHeader.mSizeData), sizeof(uint64_t));
                        dataHeadersTemp.read(reinterpret_cast<char*>(&fileHeader.mPosParent), sizeof(uint64_t));
                        dataHeadersTemp.read(reinterpret_cast<char*>(&fileHeader.mOffsetData), sizeof(uint64_t));
                        dataHeadersTemp.read(reinterpret_cast<char*>(&fileHeader.mEpochModTime), sizeof(uint64_t));
                        dataHeadersTemp.read(reinterpret_cast<char*>(&fileHeader.mReserved), sizeof(uint8_t));
                        commonFlagsTemp.seekg(i * bytesFlags, std::ios::beg);
                        commonFlagsTemp.read(reinterpret_cast<char*>(&flags[0]), bytesFlags);

                        std::string path = "";
                        int maxDepth = 10;
                        int depth = 0;
                        uint64_t posParent = fileHeader.mPosParent;
                        if (fileHeader.mPosParent != ROOT)
                        {
                                while (posParent != ROOT && depth < maxDepth)
                                {
                                        if (mDataHeader.mFileHeadersCached.find(posParent) != mDataHeader.mFileHeadersCached.end())
                                        {
                                                path = mDataHeader.mFileHeadersCached[posParent].mName + "/" + path;
                                                posParent = mDataHeader.mFileHeadersCached[posParent].mPosParent;
                                        }
                                        else
                                        {
                                                std::cerr << "Parent not found\n";
                                                break;
                                        }
                                        depth++;
                                }
                        }
                        if (path.size() > 25)
                        {
                                int index = 0;
                                while (path.find("/", index) != std::string::npos)
                                {
                                        path = path.substr(index);
                                        if (path.size() <= 22)
                                        {
                                                break;
                                        }
                                        index = path.find("/", index) + 1;
                                }
                                path = "..." + path;
                        }
                        else if (depth == maxDepth && posParent != ROOT)
                        {
                                path = "..." + path;
                        }
                        else
                        {
                                path = "/" + path;
                        }

                        path += fileHeader.mName;

                        uint64_t sizeHumanReadable = fileHeader.mSizeData;
                        int unit = 0;
                        while (sizeHumanReadable >= 1024)
                        {
                                sizeHumanReadable /= 1024;
                        }

                        bool isDirectory = flags[0] & 0b00000010;

                        std::string pathPadded = path;
                        if (!padToRight(pathPadded, 32))
                        {
                                std::cerr << "Failed to pad path\n";
                        }
                        std::string typePadded = isDirectory ? TEA_DIRECTORY_TYPE_NAME : TEA_FILE_TYPE_NAME;
                        if (!padToRight(typePadded, 8))
                        {
                                std::cerr << "Failed to pad type\n";
                        }
                        std::string sizePadded = isDirectory ? "" : std::to_string(sizeHumanReadable) + sizeUnits[unit];
                        if (!padToRight(sizePadded, 8))
                        {
                                std::cerr << "Failed to pad size\n";
                        }
                        std::string timePadded = std::to_string(fileHeader.mEpochModTime);
                        if (!padToRight(timePadded, 16))
                        {
                                std::cerr << "Failed to pad time\n";
                        }
                        std::string flagsPadded = std::bitset<TEA_FILE_FLAGS_SIZE_BITS>(flags[0]).to_string();
                        if (!padToRight(flagsPadded, TEA_FILE_FLAGS_SIZE_BITS))
                        {
                                std::cerr << "Failed to pad flags\n";
                        }
                        std::string reservedPadded = std::bitset<TEA_FILE_RESERVED_SIZE_BITS>(fileHeader.mReserved).to_string();
                        if (!padToRight(reservedPadded, TEA_FILE_RESERVED_SIZE_BITS))
                        {
                                std::cerr << "Failed to pad reserved\n";
                        }

                        std::cout << pathPadded << typePadded << sizePadded << timePadded << flagsPadded << " " << reservedPadded << "\n";

                        mDataHeader.mFileHeadersCached[pos] = fileHeader;
                }
        }
        else
        {
                std::cerr << "Failed to open data headers temp files\n";
                return false;
        }

        dataHeadersTemp.close();
        dataHeadersPositionsTemp.close();
        commonFlagsTemp.close();

        std::cout << "\n";

        return true;
}

bool TEA::tree()
{
        return true;
}

// print number of files, size, etc.
bool TEA::info()
{
        std::cout << "Archive: " << mName << "\n";
        std::cout << "Number of files: " << mArchiveHeader.mNumFiles << "\n";
        std::cout << "Metadata: " << mMetadata << "\n";
        // cout flags in binary format
        std::cout << "Global flags: " << std::bitset<TEA_GLOBAL_FLAGS_SIZE_BITS>(mArchiveHeader.mGlobalFlags) << "\n";
        std::cout << "Reserved: " << std::bitset<TEA_RESERVED_SIZE_BITS>(mArchiveHeader.mReserved) << "\n";

        return true;
}

bool TEA::printDataHEX()
{
        std::ifstream dataTemp(".data.teatemp", std::ios::binary);
        if (dataTemp.is_open())
        {
                std::cout << "Data: ";
                dataTemp.seekg(0, std::ios::end);
                uint64_t size = dataTemp.tellg();
                dataTemp.seekg(0, std::ios::beg);
                std::vector<uint8_t> data(size);
                dataTemp.read(reinterpret_cast<char*>(&data[0]), size);
                for (uint64_t i = 0; i < size; ++i)
                {
                        std::cout << std::hex << static_cast<int>(data[i]) << " ";
                }
                std::cout << "\n";
                dataTemp.close();
        }
        else
        {
                std::cerr << "Failed to open data temp file\n";
                return false;
        }

        std::cout << std::dec;

        return true;
}

bool TEA::printDataHeadersHEX()
{
        std::ifstream dataHeadersTemp(".data_headers.teatemp", std::ios::binary);
        if (dataHeadersTemp.is_open())
        {
                std::cout << "Data headers: ";
                dataHeadersTemp.seekg(0, std::ios::end);
                uint64_t size = dataHeadersTemp.tellg();
                dataHeadersTemp.seekg(0, std::ios::beg);
                std::vector<uint8_t> data(size);
                dataHeadersTemp.read(reinterpret_cast<char*>(&data[0]), size);
                for (uint64_t i = 0; i < size; ++i)
                {
                        std::cout << std::hex << static_cast<int>(data[i]) << " ";
                }
                std::cout << "\n";
                dataHeadersTemp.close();
        }
        else
        {
                std::cerr << "Failed to open data headers temp file\n";
                return false;
        }

        std::ifstream dataHeadersPositionsTemp(".data_headers_positions.teatemp", std::ios::binary);
        if (dataHeadersPositionsTemp.is_open())
        {
                std::cout << "Data headers positions: ";
                dataHeadersPositionsTemp.seekg(0, std::ios::end);
                uint64_t size = dataHeadersPositionsTemp.tellg();
                dataHeadersPositionsTemp.seekg(0, std::ios::beg);
                std::vector<uint8_t> data(size);
                dataHeadersPositionsTemp.read(reinterpret_cast<char*>(&data[0]), size);
                for (uint64_t i = 0; i < size; ++i)
                {
                        std::cout << std::hex << static_cast<int>(data[i]) << " ";
                }
                std::cout << "\n";
                dataHeadersPositionsTemp.close();
        }
        else
        {
                std::cerr << "Failed to open data headers positions temp file\n";
                return false;
        }

        std::cout << std::dec;

        return true;
}

bool TEA::printCommonFlagsHEX()
{
        std::ifstream commonFlagsTemp(".common_flags.teatemp", std::ios::binary);
        if (commonFlagsTemp.is_open())
        {
                std::cout << "Common flags: ";
                commonFlagsTemp.seekg(0, std::ios::end);
                uint64_t size = commonFlagsTemp.tellg();
                commonFlagsTemp.seekg(0, std::ios::beg);
                std::vector<uint8_t> data(size);
                commonFlagsTemp.read(reinterpret_cast<char*>(&data[0]), size);
                for (uint64_t i = 0; i < size; ++i)
                {
                        std::cout << std::hex << static_cast<int>(data[i]) << " ";
                }
                std::cout << "\n";
                commonFlagsTemp.close();
        }
        else
        {
                std::cerr << "Failed to open common flags temp file\n";
                return false;
        }

        std::cout << std::dec;

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
