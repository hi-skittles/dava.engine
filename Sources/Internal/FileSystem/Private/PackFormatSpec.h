#pragma once

#include "Compression/Compressor.h"

namespace DAVA
{
namespace PackFormat
{
const Array<char8, 4> FILE_MARKER{ { 'D', 'V', 'P', 'K' } };
const Array<char8, 4> FILE_MARKER_LITE{ { 'D', 'V', 'P', 'L' } };

struct PackFile
{
    // 0 to N bytes of files - packed contents
    struct PackedFilesBlock
    {
    } rawBytesOfCompressedFiles;

    // 0 or footer.metaDataSize bytes
    struct CustomMetadataBlock
    {
    } metadata;

    struct FilesTableBlock
    {
        // table with info per file in archive (0 to N_files * sizeof(Data))
        struct FilesData
        {
            struct Data
            {
                uint64 startPosition; // from begin of file
                uint32 compressedSize;
                uint32 originalSize;
                uint32 compressedCrc32;
                Compressor::Type type;
                uint32 originalCrc32;
                uint32 metaIndex; // can be custom user index in metaData
            };

            Vector<Data> files;
        } data;

        // 0 to N bytes (all file names concatenated and '\0' separated and packed)
        // order of file names same as in FilesData
        struct Names
        {
            Vector<uint8> compressedNames; // lz4hc
            uint32 compressedCrc32;
        } names;
    } filesTable;

    struct FooterBlock
    {
        Array<uint8, 8> reserved{};
        uint32 metaDataCrc32 = 0; // 0 or crc32 for custom user meta block
        uint32 metaDataSize = 0; // 0 or size of custom user meta data block
        uint32 infoCrc32 = 0;
        struct Info
        {
            uint32 numFiles = 0;
            uint32 namesSizeCompressed = 0; // lz4hc
            uint32 namesSizeOriginal = 0;
            uint32 filesTableSize = 0;
            uint32 filesTableCrc32 = 0; // hash for both FilesData and FileNames if one file change -> hash will change, or if name of file change -> hash will change too
            Array<char8, 4> packArchiveMarker{};
        } info;
    } footer;
}; // end PackFile struct

using FileTableEntry = PackFile::FilesTableBlock::FilesData::Data;

/**
	One file packed with our custom compression + 20 bytes footer
	in the end of file with info to decompress content.
	We will use it later in our new DLCManager to download game
	content file by file to reduce size
*/
struct LitePack
{
    struct CompressedBytes
    {
    };

    struct Footer
    {
        uint32 sizeUncompressed;
        uint32 sizeCompressed;
        uint32 crc32Compressed;
        Compressor::Type type;
        Array<char8, 4> packMarkerLite;
    };
};

static_assert(sizeof(LitePack::Footer) == 20, "footer block size changed");
static_assert(sizeof(PackFile::FooterBlock) == 44, "header block size changed");
static_assert(sizeof(FileTableEntry) == 32, "file table entry size changed");

} // end of PackFormat namespace

} // end of DAVA namespace
