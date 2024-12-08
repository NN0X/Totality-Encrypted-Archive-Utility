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
// position of parent: 8 bytes (0 if no parent)
// offset from data: 8 bytes
// epoch modification time: 8 bytes
// reserved: 1 byte
//
// Flags (archive header):
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
// end of file headers: 8 bytes
// file headers: n bytes
// positions of file headers: n * 8 bytes

enum FlagsIndices
{
	ENCRYPTED_BIT = 0,
	COMPRESSED_BIT = 1,
	COMPRESSION_TYPE_BIT = 2,
	COMPRESSION_STRENGTH_BIT = 4,
	DIRECTORY = 6
};

enum FlagsSizes
{
	ENCRYPTED_SIZE = 1,
	COMPRESSED_SIZE = 1,
	COMPRESSION_TYPE_SIZE = 2,
	COMPRESSION_STRENGTH_SIZE = 2,
	DIRECTORY_SIZE = 1
};

struct ArchiveHeader
{
	uint64_t numFiles;
	uint64_t posDataHeaders;
	uint16_t numUniqueFlags;
	uint64_t posCommonFlags;
	uint16_t sizeMetadata;
	uint64_t posMetadata;
	uint16_t globalFlags;
	uint32_t reserved;

	void load(std::string path);
	std::vector<uint8_t> toBytes();
};

struct FileHeader
{
	uint16_t sizeName;
	std::string name;
	uint64_t sizeData;
	uint64_t posParent;
	uint64_t offsetData;
	uint64_t epochModTime;
	uint8_t reserved;

	void load(std::string path);
	std::vector<uint8_t> toBytes();
};

struct DataHeader
{
	uint64_t posEndFileHeaders;
	std::vector<FileHeader> fileHeaders;
	std::vector<uint64_t> posFileHeaders;

	void load(std::string path);
	std::vector<uint8_t> toBytes();
};

// TEA - Totality Encrypted Archive
class TEA 
{
private:
	std::string name;
	std::string path;
	std::string signature;
	uint8_t version;
	ArchiveHeader archiveHeader;
	DataHeader dataHeader;
	std::vector<uint8_t> data;
	std::vector<uint8_t> commonFlags;
	std::string metadata;
public:
	TEA(std::string path, std::string name);

	void load();
	void save();
	
	void extract(std::string archiveInternalPath, std::string path);
	void add(std::string path, std::string archiveInternalPath);
	void remove(std::string archiveInternalPath);

	void move(std::string archiveInternalPathOld, std::string archiveInternalPathNew);
	void rename(std::string archiveInternalPathOld, std::string archiveInternalPathNew);

	void encrypt(int method, std::string key);
	void decrypt(int method, std::string key);
	
	void compress(int method, int strength);
	void decompress(int method, int strength);

	void list(); // print file tree
	void info(); // print number of files, size, etc.

	void setArchiveFlags(bool encrypted, bool compressed, int method, int strength, std::vector<bool> additionalFlags);
	bool getArchiveFlag(int bitIndex, int size);
	std::vector<bool> getArchiveFlags();

	void setCommonFlags(bool encrypted, bool compressed, int method, int strength, bool directory, std::vector<bool> additionalFlags);
	bool getCommonFlag(int bitIndex, int size);
	std::vector<bool> getCommonFlags();

	void setName(std::string name);
	std::string getName();

	void setPath(std::string path);
	std::string getPath();

	void setMetadata(std::string metadata);
	std::string getMetadata();

	void setSignature(std::string signature);
	std::string getSignature();

	void setVersion(uint8_t version);
	uint8_t getVersion();
}
