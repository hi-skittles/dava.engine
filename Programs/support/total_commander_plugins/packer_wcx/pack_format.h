#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#pragma once

namespace pack_format
{
const std::array<char, 4> file_marker{ { 'D', 'V', 'P', 'K' } };
const std::array<char, 4> file_marker_lite{ { 'D', 'V', 'P', 'L' } };

struct pack_file
{
    // 0 to N bytes of files - packed contents
    struct packed_files_block
    {
    } raw_bytes_of_compressed_files;

    struct files_table_block
    {
        // table with info per file in archive (0 to N_files * sizeof(Data))
        struct files_data
        {
            struct data
            {
                uint64_t start_position = 0; // from begin of file
                uint32_t compressed_size = 0;
                uint32_t original_size = 0;
                uint32_t compressed_crc32 = 0;
                uint32_t type = 0;
                uint32_t original_crc32 = 0;
                uint32_t meta_index = 0; // null bytes, leave for future
            };
            std::vector<data> files;
        } data;

        // 0 to N bytes (all file names concatenated and '\0' separeted and packed)
        // order of file names same as in FilesData
        struct names
        {
            std::vector<uint8_t> compressed_names; // lz4hc
            uint32_t compressed_crc32;
        } names;
    } files_table;

    struct footer_block
    {
        std::array<uint8_t, 8> reserved;
        uint32_t meta_data_crc32; // 0 or hash
        uint32_t meta_data_size; // 0 or size of meta data block
        uint32_t info_crc32;
        struct info
        {
            uint32_t num_files;
            uint32_t names_size_compressed; // lz4hc
            uint32_t names_size_original;
            uint32_t files_table_size;
            uint32_t files_table_crc32; // hash for both FilesData and FileNames if one file change -> hash will change, or if name of file change -> hash will change too
            std::array<char, 4> pack_archive_marker;
        } info;
    } footer;
}; // end PackFile struct

struct LitePack
{
    struct compressed_bytes
    {
    };

    struct footer
    {
        uint32_t sizeUncompressed;
        uint32_t sizeCompressed;
        uint32_t crc32_compressed;
        uint32_t type;
        std::array<char, 4> pack_marker_lite;
    };
};

struct file_info
{
    file_info() = default;

    file_info(const char* relativePath, uint32_t originalSize,
              uint32_t compressedSize, uint32_t compressionType);

    std::string relativeFilePath;
    uint32_t originalSize = 0;
    uint32_t compressedSize = 0;
    uint32_t compressionType = 0;
    uint32_t hash = 0; // crc32
};

using file_table_entry = pack_file::files_table_block::files_data::data;

static_assert(sizeof(pack_file::footer_block) == 44,
              "header block size changed, something bad happened!");
static_assert(sizeof(file_table_entry) == 32,
              "file table entry size changed, something bad happened!");
static_assert(sizeof(LitePack::footer) == 20, "check lite footer size");

} // end of PackFormat namespace
