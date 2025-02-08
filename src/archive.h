#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

// TEA Format:
//
// signature: 3 bytes
// padding: 1 byte
// version: 1 byte
// padding: 1 byte
// data: n bytes
// padding: 1 byte
// data headers: n bytes
// padding: 1 byte
// common flags: n bytes
// padding: 1 byte
// metadata: n bytes
// padding: 1 byte
// header: 42 bytes
// padding: 1 byte
// signature: 3 bytes
//
// header:
//
// number of files: 8 bytes
// position of data headers: 8 bytes
// number of unique flags: 2 bytes
// position of common flags: 8 bytes
// size of metadata: 2 bytes
// position of metadata: 8 bytes
// global flags: 2 bytes
// reserved: 4 bytes
//
// file header:
//
// size of name: 2 bytes
// name: n bytes
// size of data: 8 bytes
// position of parent: 8 bytes (0xFFFFFFFFFFFFFFFF for root)
// offset from data: 8 bytes
// epoch modification time: 8 bytes
// reserved: 1 byte
//
// Flags (archive header - global flags):
//
// b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16
//
// b1: encrypted
// b2: compressed
// b3: compression type
// b4: compression type
// b5: compression strength
// b6: compression strength
// >= b7: reserved
//
// Flags (common flags):
//
// b1, b2, b3, b4, b5, b6, b7, b8, ...
//
// b1: encrypted
// b2: compressed
// b3: compression type
// b4: compression type
// b5: compression strength
// b6: compression strength
// b7: directory
// >= b8: reserved
// TODO: consider adding common flags for rwx permissions or just flag for executable
//
// Data header:
//
// end of file headers: 8 bytes (from the start of file headers)
// file headers: n bytes
// positions of file headers: n * 8 bytes

#define TEA_SIGNATURE "TEA"
#define TEA_VERSION 0b00000001 // 1
#define TEA_PADDING 0b00000000 // 0x00
#define ROOT 0xFFFFFFFFFFFFFFFF  // root id is the maximum value of uint64_t

#define TEA_SIGNATURE_SIZE 3
#define TEA_PADDING_SIZE 1
#define TEA_VERSION_SIZE 1
#define TEA_HEADER_SIZE 42

#define TEA_GLOBAL_FLAGS_SIZE_BITS 16
#define TEA_RESERVED_SIZE_BITS 32

#define TEA_FILE_FLAGS_SIZE_BITS 8
#define TEA_FILE_RESERVED_SIZE_BITS 8
#define TEA_DIRECTORY_TYPE_NAME "DIR"
#define TEA_FILE_TYPE_NAME "FILE"

const std::string sizeUnits[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};

// temporary defines that should be computed during runtime based on available system information
#define DEFAULT_CHUNK_SIZE 1024*1024       // in bytes (1 MiB)
#define DEFAULT_CACHE_SIZE 1024            // in bytes (1 KiB) //TODO: fix usage of this (now it's used as vector size)
//

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_BLUE "\033[34m"
#define COLOR_LIGHT_BLUE "\033[94m"
#define COLOR_RESET "\033[0m"

enum FlagsIndices
{
        TEA_ENCRYPTED_BIT = 0,
        TEA_COMPRESSED_BIT = 1,
        TEA_COMPRESSION_TYPE_BIT = 2,
        TEA_COMPRESSION_STRENGTH_BIT = 4,
        TEA_DIRECTORY_BIT = 6,
        TEA_FLAGS_RESERVED_START_BIT = 7
};

enum FlagsSizes
{
        TEA_ENCRYPTED_SIZE = 1,
        TEA_COMPRESSED_SIZE = 1,
        TEA_COMPRESSION_TYPE_SIZE = 2,
        TEA_COMPRESSION_STRENGTH_SIZE = 2,
        TEA_DIRECTORY_SIZE = 1
};

enum EncryptionTypes
{
        TEA_BRUTUS = 0,
        TEA_AES = 1,
        TEA_ENCRYPTION_RESERVED = 2,
};

enum CompressionTypes
{
        TEA_DEFLATE = 0,
        TEA_COMPRESSION_RESERVED_1 = 1,
        TEA_COMPRESSION_RESERVED_2 = 2,
};

enum Strengths
{
        TEA_LOW = 0,
        TEA_MEDIUM = 1,
        TEA_HIGH = 2,
};

struct ArchiveHeader
{
        uint64_t mNumFiles;
        uint64_t mPosDataHeaders;
        uint16_t mNumUniqueFlags;
        uint64_t mPosCommonFlags;
        uint16_t mSizeMetadata;
        uint64_t mPosMetadata;
        uint16_t mGlobalFlags;
        uint32_t mReserved;
};

struct FileHeader
{
        uint16_t mSizeName;
        std::string mName;
        uint64_t mSizeData;
        uint64_t mPosParent;
        uint64_t mOffsetData;
        uint64_t mEpochModTime;
        uint8_t mReserved;
};

struct DataHeader
{
        uint64_t mPosEndFileHeaders;
        std::unordered_map<uint64_t, FileHeader> mFileHeadersCached;
        std::vector<uint64_t> mPosFileHeaders;
};

// TEA - Totality Encrypted Archive (contains core functionality)
// INFO: all functions that return return true on success and false on failure
class TEA
{
private:
        std::string mName;
        std::string mPath;
        std::string mSignature;
        uint8_t mVersion;
        ArchiveHeader mArchiveHeader;
        DataHeader mDataHeader;
        std::vector<uint8_t> mDataCached;
        std::vector<uint8_t> mCommonFlagsCached;
        std::string mMetadata;
public:
        TEA(const std::string &path, const std::string &name); // create archive class with specified path and name
        ~TEA();

        void close(); // calls save() but also frees resources

        void init(); // initialize archive class with default values and create temporary files

        bool load(); // load archive from .tea file
        bool save(); // save archive to .tea file (after this operation archive class is in closed state)

        bool rebuild(); // rebuild archive from temporary files (for example after unexpected program termination)

        bool manageCache(); // manage cached data (mFileHeadersCached, mPosFileHeaders, mDataCached, mCommonFlagsCached)

        // extract doesn't remove file from archive
        bool extract(const std::string &archiveInternalpath); // extract file from archive to current directory
        bool extract(const std::string &archiveInternalPath, const std::string &path);
        bool add(const std::string &path, const std::string &archiveInternalPath);
        bool add(const std::string &path, const std::string &archiveInternalPath, bool encrypted, bool compressed, int method, int strength, bool directory, const std::vector<bool> &additionalFlags);
        bool add(const std::string &path, const std::string &archiveInternalPath, const std::vector<bool> &flags);
        bool remove(const std::string &archiveInternalPath); // remove file from archive (doesn't extract)

        bool move(const std::string &archiveInternalPathOld, const std::string &archiveInternalPathNew);
        bool rename(const std::string &archiveInternalPath, const std::string &newName);

        bool encrypt(int method, const std::vector<uint8_t> &key);
        bool decrypt(const std::vector<uint8_t> &key);

        bool compress(int method, int strength);
        bool decompress();

        bool list(); // list all files in archive
        bool tree(); // list all files in archive in tree structure

        bool info(); // print number of files, size, etc.

        bool printDataHEX();
        bool printDataHeadersHEX();
        bool printCommonFlagsHEX();

        bool setArchiveFlags(bool encrypted, bool compressed, int method, int strength, const std::vector<bool> &additionalFlags);
        bool setArchiveFlags(const std::vector<bool> &flags);

        bool getArchiveFlag(int bitIndex, int size, bool &flag);
        bool getArchiveFlags(std::vector<bool> &flags);

        bool setCommonFlags(bool encrypted, bool compressed, int method, int strength, bool directory, const std::vector<bool> &additionalFlags);
        bool setCommonFlags(const std::vector<bool> &flags);
        bool getCommonFlag(int bitIndex, int size, bool &flag);
        bool getCommonFlags(std::vector<bool> &flags);

        void setName(const std::string &name);
        std::string getName();

        void setPath(const std::string &path);
        std::string getPath();

        // debug functions
        void setMetadata(const std::string &metadata);
        std::string getMetadata();

        void setSignature(const std::string &signature);
        std::string getSignature();

        void setVersion(uint8_t version);
        uint8_t getVersion();
};

#endif // ARCHIVE_H
