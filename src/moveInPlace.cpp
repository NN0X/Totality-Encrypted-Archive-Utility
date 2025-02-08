#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

bool deleteChunk(std::fstream& file, uint64_t position, uint64_t size, const std::string &path, const uint64_t chunk)
{
        file.seekg(0, std::ios::end);
        uint64_t fileSize = file.tellg();
        uint64_t readPos = position + size;
        if (readPos > fileSize)
        {
                std::cerr << "Invalid position or size\n";
                return false;
        }
        std::vector<uint8_t> data(chunk);

        uint64_t remaining = fileSize - position - size;
        uint64_t writePos = position;

        while (remaining > 0)
        {
                uint64_t readSize = remaining > chunk ? chunk : remaining;
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

bool moveFileInPlace(std::fstream &original, std::ofstream &target, const std::string &pathOriginal, uint64_t size, uint64_t chunk)
{
        uint64_t pos;
        for (uint64_t i = 0; i < size; i += chunk)
        {
                if (size - i < chunk)
                {
                        std::vector<uint8_t> data(size - i);
                        original.read(reinterpret_cast<char*>(&data[0]), size - i);
                        pos = original.tellg();
                        original.seekg(pos - (size - i), std::ios::beg);
                        target.write(reinterpret_cast<char*>(&data[0]), size - i);
                        if (!deleteChunk(original, original.tellg(), size - i, pathOriginal, chunk))
                        {
                                std::cerr << "Failed to delete data chunk\n";
                                return false;
                        }
                }
                else
                {
                        std::vector<uint8_t> data(chunk);
                        original.read(reinterpret_cast<char*>(&data[0]), chunk);
                        pos = original.tellg();
                        original.seekg(pos - chunk, std::ios::beg);
                        target.write(reinterpret_cast<char*>(&data[0]), chunk);
                        if (!deleteChunk(original, original.tellg(), chunk, pathOriginal, chunk))
                        {
                                std::cerr << "Failed to delete data chunk\n";
                                return false;
                        }
                }
        }


        return true;
}

