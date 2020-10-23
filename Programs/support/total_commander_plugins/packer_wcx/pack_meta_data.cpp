#include "pack_meta_data.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include "lz4.h"

extern std::ofstream l;

static bool lz4_compressor_decompress(const std::vector<uint8_t>& in,
                                      std::vector<uint8_t>& out)
{
    int32_t decompressResult = LZ4_decompress_fast(
    reinterpret_cast<const char*>(in.data()),
    reinterpret_cast<char*>(out.data()),
    static_cast<uint32_t>(out.size()));
    if (decompressResult < 0)
    {
        return false;
    }
    return true;
}

pack_meta_data::pack_meta_data(const void* ptr, std::size_t size)
{
    deserialize(ptr, size);
}

uint32_t pack_meta_data::get_num_files() const
{
    return table_files.size();
}

uint32_t pack_meta_data::get_num_packs() const
{
    return table_packs.size();
}

uint32_t pack_meta_data::get_pack_index_for_file(
const uint32_t fileIndex) const
{
    return table_files.at(fileIndex);
}

const std::tuple<std::string, std::string>& pack_meta_data::get_pack_info(
const uint32_t packIndex) const
{
    return table_packs.at(packIndex);
}

const std::vector<uint32_t>& pack_meta_data::get_children(const uint32_t pack_index) const
{
    return table_childs[pack_index];
}

std::vector<uint8_t> pack_meta_data::serialize() const
{
    return std::vector<uint8_t>();
}

struct membuf : std::streambuf
{
    membuf(const void* ptr, size_t size)
    {
        char* begin = const_cast<char*>(static_cast<const char*>(ptr));
        char* end = const_cast<char*>(begin + size);
        setg(begin, begin, end);
    }
};

void pack_meta_data::deserialize(const void* ptr, size_t size)
{
    using namespace std;
    assert(ptr != nullptr);
    assert(size >= 16);

    l << "start deserialize\n";

    membuf buf(ptr, size);

    l << "create membuf\n";

    istream is(&buf);

    l << "create istream\n";

    // 4b header - "meta"
    // 4b num_files
    // num_files b
    // 4b - uncompressed_size
    // 4b - compressed_size
    // compressed_size b
    array<char, 4> header;
    is.read(&header[0], 4);
    if (!is || header != array<char, 4>{ 'm', 'e', 't', '2' })
    {
        l << "read metadata error - not meta\n";
        throw runtime_error("read metadata error - not meta");
    }
    l << "read numFiles\n";
    uint32_t numFiles = 0;
    is.read(reinterpret_cast<char*>(&numFiles), 4);
    if (!is)
    {
        l << "read metadata error - no numFiles\n";
        throw runtime_error("read metadata error - no numFiles");
    }
    l << "numFiles = " << numFiles << '\n';
    table_files.resize(numFiles);

    l << "read numFilesBytes\n";
    const uint32_t numFilesBytes = numFiles * 4;
    is.read(reinterpret_cast<char*>(&table_files[0]), numFilesBytes);
    if (!is)
    {
        l << "read metadata error - no tableFiles\n";
        throw runtime_error("read metadata error - no tableFiles");
    }

    uint32_t uncompressedSize = 0;
    is.read(reinterpret_cast<char*>(&uncompressedSize), 4);
    if (!is)
    {
        l << "read metadata error - no uncompressedSize\n";
        throw runtime_error("read metadata error - no uncompressedSize");
    }
    l << "read uncompressedSize " << uncompressedSize << '\n';
    uint32_t compressedSize = 0;
    is.read(reinterpret_cast<char*>(&compressedSize), 4);
    if (!is)
    {
        l << "read metadata error - no compressedSize\n";
        throw runtime_error("read metadata error - no compressedSize");
    }
    l << "read compressedSize " << compressedSize << '\n';

    l << "numFilesBytes = " << numFilesBytes << " compressedSize = " << compressedSize << " size = " << size << '\n';

    vector<uint8_t> compressedBuf(compressedSize);

    is.read(reinterpret_cast<char*>(&compressedBuf[0]), compressedSize);
    if (!is)
    {
        l << "read metadata error - no compressedBuf\n";
        throw runtime_error("read metadata error - no compressedBuf");
    }
    l << "read compressedBuf\n";
    l << "uncompressedSize >= compressedSize == " << (uncompressedSize >= compressedSize) << '\n';

    if (uncompressedSize < compressedSize)
    {
        l << "warning! uncompressedSize < compressedSize, continue\n";
    }

    l << "decompress start\n";
    vector<uint8_t> uncompressedBuf(uncompressedSize);

    if (!lz4_compressor_decompress(compressedBuf, uncompressedBuf))
    {
        l << "read metadata error - can't decompress\n";
        throw runtime_error("read metadata error - can't decompress");
    }

    l << "finish decompress\n";
    const char* startBuf = reinterpret_cast<const char*>(&uncompressedBuf[0]);

    membuf outBuf(startBuf, uncompressedSize);
    istream ss(&outBuf);

    l << "start parse decompressed data line by line\n";
    // now parse decompressed packs data line by line (%s %s\n) format
    for (string line, packName, packDependency; getline(ss, line);)
    {
        l << "line: " << line << '\n';
        auto first_space = line.find(' ');
        if (first_space == string::npos)
        {
            l << "can't parse packs and dependencies\n";
            throw runtime_error("can't parse packs and dependencies");
        }
        packName = line.substr(0, first_space);
        packDependency = line.substr(first_space + 1);
        table_packs.push_back({ packName, packDependency });
    }

    // read children table
    l << "start reading children table\n";
    uint32_t numPacksWithChildren = 0;
    is.read(reinterpret_cast<char*>(&numPacksWithChildren), sizeof(numPacksWithChildren));
    if (!is)
    {
        throw std::runtime_error("read numPacksWithChildren failed");
    }

    table_childs.resize(table_packs.size());

    for (; numPacksWithChildren > 0; --numPacksWithChildren)
    {
        uint32_t childPackIndex = 0;
        is.read(reinterpret_cast<char*>(&childPackIndex), sizeof(childPackIndex));
        if (!is)
        {
            throw std::runtime_error("read childPackIndex failed");
        }

        uint32_t numChildPacks = 0;
        is.read(reinterpret_cast<char*>(&numChildPacks), sizeof(numChildPacks));
        if (!is)
        {
            throw std::runtime_error("read numChildPacks failed");
        }
        std::vector<uint32_t>& child = table_childs[childPackIndex];
        child.resize(numChildPacks);

        uint32_t numBytes = numChildPacks * sizeof(child[0]);
        is.read(reinterpret_cast<char*>(&child[0]), numBytes);
        if (!is)
        {
            throw std::runtime_error("read numBytes failed");
        }
    }
    l << "finish read children talbe\n";
}
