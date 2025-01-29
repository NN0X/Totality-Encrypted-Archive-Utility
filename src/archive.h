#include <vector>
#include <cstdint>

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
//
// Data header:
//
// end of file headers: 8 bytes (from the start of file headers)
// file headers: n bytes
// positions of file headers: n * 8 bytes

#define TEA_SIGNATURE "TEA"
#define TEA_VERSION 0b00000001 // 1
#define TEA_PADDING 0b00000000
#define ROOT 0xFFFFFFFFFFFFFFFF  // root id is the maximum value of uint64_t


// temporary defines
#define DEFAULT_CHUNK_SIZE 1024

enum FlagsIndices
{
	TEA_ENCRYPTED_BIT = 0,
	TEA_COMPRESSED_BIT = 1,
	TEA_COMPRESSION_TYPE_BIT = 2,
	TEA_COMPRESSION_STRENGTH_BIT = 4,
	TEA_DIRECTORY = 6
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
};

enum CompressionTypes
{
	TEA_DEFLATE = 0,
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
	std::vector<FileHeader> mFileHeadersCached;
	std::vector<uint64_t> mPosFileHeaders;
};

// TEA - Totality Encrypted Archive
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
	TEA(const std::string &path, const std::string &name);
	~TEA();

	void close();

	void init();

	bool load();
	bool save();

	bool extract(const std::string &archiveInternalpath);
	bool extract(const std::string &archiveInternalPath, const std::string &path);
	bool add(const std::string &path, const std::string &archiveInternalPath);
	bool add(const std::string &path, const std::string &archiveInternalPath, bool encrypted, bool compressed, int method, int strength, bool directory, const std::vector<bool> &additionalFlags);
	bool add(const std::string &path, const std::string &archiveInternalPath, const std::vector<bool> &flags);
	bool remove(const std::string &archiveInternalPath);

	bool move(const std::string &archiveInternalPathOld, const std::string &archiveInternalPathNew);
	bool rename(const std::string &archiveInternalPathOld, const std::string &archiveInternalPathNew);

	bool encrypt(int method, const std::string &key);
	bool decrypt(int method, const std::string &key);
	
	bool compress(int method, int strength);
	bool decompress(int method, int strength);

	bool list(); // print file tree
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

	void setMetadata(const std::string &metadata);
	std::string getMetadata();

	void setSignature(const std::string &signature);
	std::string getSignature();

	void setVersion(uint8_t version);
	uint8_t getVersion();
};
