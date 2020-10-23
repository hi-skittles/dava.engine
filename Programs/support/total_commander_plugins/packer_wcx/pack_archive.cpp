#include "pack_archive.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "lz4.h"

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

std::uint32_t crc32_for_buffer(const char* data, std::uint32_t size);

extern std::ofstream l;

PackArchive::PackArchive(const std::string& archiveName)
{
    file_index = 0;
    l << "inside pack_archive\n";
    using namespace pack_format;

    std::string fileName = archiveName;

    file.open(fileName, std::ios_base::binary | std::ios_base::ate);
    if (!file)
    {
        throw std::runtime_error("can't Open file: " + fileName);
    }

    uint64_t size = file.tellg();
    if (size < sizeof(pack_file.footer))
    {
        throw std::runtime_error(
        "file size less then pack footer: " + fileName);
    }

    if (!file.seekg(size - sizeof(pack_file.footer), std::ios_base::beg))
    {
        throw std::runtime_error("can't seek to footer in file: " + fileName);
    }

    auto& footerBlock = pack_file.footer;
    file.read(reinterpret_cast<char*>(&footerBlock), sizeof(footerBlock));
    if (!file)
    {
        throw std::runtime_error(
        "can't read footer from packfile: " + fileName);
    }

    // count crc32
    uint32_t crc32footer = crc32_for_buffer(
    reinterpret_cast<char*>(&pack_file.footer.info),
    sizeof(pack_file.footer.info));
    if (crc32footer != pack_file.footer.info_crc32)
    {
        throw std::runtime_error("not match crc32 in footer");
    }

    if (footerBlock.info.pack_archive_marker != file_marker)
    {
        throw std::runtime_error("incorrect marker in pack file: " + fileName);
    }

    if (footerBlock.info.num_files > 0)
    {
        uint64_t startFilesTableBlock =
        size - (sizeof(pack_file.footer) + pack_file.footer.info.files_table_size);

        std::vector<char> tmpBuffer;
        tmpBuffer.resize(pack_file.footer.info.files_table_size);

        if (!file.seekg(startFilesTableBlock, std::ios_base::beg))
        {
            throw std::runtime_error(
            "can't seek to filesTable block in file: " + fileName);
        }

        file.read(tmpBuffer.data(), pack_file.footer.info.files_table_size);
        if (!file)
        {
            throw std::runtime_error(
            "can't read filesTable block from file: " + fileName);
        }

        uint32_t crc32filesTable = crc32_for_buffer(tmpBuffer.data(),
                                                    pack_file.footer.info.files_table_size);
        if (crc32filesTable != pack_file.footer.info.files_table_crc32)
        {
            throw std::runtime_error(
            "crc32 not match in filesTable in file: " + fileName);
        }

        std::vector<uint8_t>& compressedNamesBuffer =
        pack_file.files_table.names.compressed_names;
        compressedNamesBuffer.resize(pack_file.footer.info.names_size_compressed,
                                     '\0');

        uint32_t sizeOfFilesData = pack_file.footer.info.num_files * sizeof(file_table_entry);
        const char* startOfCompressedNames = &tmpBuffer[sizeOfFilesData];

        pack_file.files_table.names.compressed_names.resize(
        pack_file.footer.info.names_size_compressed);

        std::copy_n(startOfCompressedNames,
                    pack_file.footer.info.names_size_compressed,
                    reinterpret_cast<char*>(pack_file.files_table.names.compressed_names.data()));

        std::vector<uint8_t> originalNamesBuffer;
        originalNamesBuffer.resize(pack_file.footer.info.names_size_original);
        if (!lz4_compressor_decompress(compressedNamesBuffer,
                                       originalNamesBuffer))
        {
            throw std::runtime_error("can't uncompress file names");
        }

        std::string fileNames(begin(originalNamesBuffer),
                              end(originalNamesBuffer));

        std::vector<file_table_entry>& fileTable = pack_file.files_table.data.files;
        fileTable.resize(footerBlock.info.num_files);

        file_table_entry* startFilesData =
        reinterpret_cast<file_table_entry*>(tmpBuffer.data());

        std::copy_n(startFilesData, footerBlock.info.num_files,
                    fileTable.data());

        files_info.reserve(footerBlock.info.num_files);

        size_t numFiles = std::count_if(begin(fileNames), end(fileNames),
                                        [](const char& ch) {
                                            return '\0' == ch;
                                        });
        if (numFiles != fileTable.size())
        {
            throw std::runtime_error(
            "number of file names not match with table");
        }

        // now fill support structures for fast search by filename
        size_t fileNameIndex{ 0 };

        std::for_each(begin(fileTable), end(fileTable),
                      [&](file_table_entry& fileEntry) {
                          const char* fileNameLoc = &fileNames[fileNameIndex];
                          map_file_data.emplace(fileNameLoc, &fileEntry);

                          file_info info;

                          info.relativeFilePath = fileNameLoc;
                          info.originalSize = fileEntry.original_size;
                          info.compressedSize = fileEntry.compressed_size;
                          info.hash = fileEntry.compressed_crc32;
                          info.compressionType = fileEntry.type;

                          files_info.push_back(info);

                          fileNameIndex = fileNames.find('\0', fileNameIndex + 1);
                          ++fileNameIndex;
                      });
    } // end if (footerBlock.info.numFiles > 0)

    if (footerBlock.meta_data_size > 0)
    {
        l << "parse metadata block\n";
        uint64_t startMetaBlock = size - (sizeof(pack_file.footer) + pack_file.footer.info.files_table_size + footerBlock.meta_data_size);
        l << "start metadata block pos: " << startMetaBlock << '\n';
        file.seekg(startMetaBlock, std::fstream::beg);
        l << "seek to metadata block\n";
        std::vector<uint8_t> metaBlock(footerBlock.meta_data_size);
        l << "allocate memory for metablock\n";
        if (!file)
        {
            throw std::runtime_error("can't seek meta");
        }
        l << "seek to metablock done.\n";
        file.read(reinterpret_cast<char*>(&metaBlock[0]), metaBlock.size());
        if (!file)
        {
            throw std::runtime_error("can't read meta");
        }
        l << "reset pack_meta_data from byte stream\n";
        pack_meta.reset(new pack_meta_data(&metaBlock[0], metaBlock.size()));
    }

    l << "end constructor\n";
    l << "files:\n";
    for (auto& f : files_info)
    {
        l << f.relativeFilePath << '\n';
    }
    l << "total: " << files_info.size() << '\n';
}

const std::vector<pack_format::file_info>& PackArchive::GetFilesInfo() const
{
    return files_info;
}

const pack_format::file_info* PackArchive::GetFileInfo(
const std::string& relativeFilePath) const
{
    auto it = map_file_data.find(relativeFilePath);

    if (it != map_file_data.end())
    {
        // find out index of FileInfo*
        const pack_format::file_table_entry* currentFile = it->second;
        const pack_format::file_table_entry* start =
        pack_file.files_table.data.files.data();
        ptrdiff_t index = std::distance(start, currentFile);
        return &files_info.at(static_cast<uint32_t>(index));
    }
    return nullptr;
}

bool PackArchive::HasFile(const std::string& relativeFilePath) const
{
    auto iterator = map_file_data.find(relativeFilePath);
    return iterator != map_file_data.end();
}

bool PackArchive::HoadFile(const std::string& relativeFilePath,
                           std::vector<uint8_t>& output)
{
    using namespace pack_format;

    if (!HasFile(relativeFilePath))
    {
        return false;
    }

    const file_table_entry& fileEntry =
    *map_file_data.find(relativeFilePath)->second;
    output.resize(fileEntry.original_size);

    file.seekg(fileEntry.start_position, std::ios_base::beg);
    if (!file)
    {
        return false;
    }

    switch (fileEntry.type)
    {
    case 0:
    {
        file.read(reinterpret_cast<char*>(output.data()),
                  fileEntry.original_size);
        if (!file)
        {
            return false;
        }
    }
    break;
    case 1: // Compressor::Type::Lz4:
    case 2: // Compressor::Type::Lz4HC:
    {
        std::vector<uint8_t> packedBuf(fileEntry.compressed_size);

        file.read(reinterpret_cast<char*>(packedBuf.data()),
                  fileEntry.compressed_size);
        if (!file)
        {
            return false;
        }

        if (!lz4_compressor_decompress(packedBuf, output))
        {
            return false;
        }
    }
    break;
    case 3: // Compressor::Type::RFC1951:
    {
        std::vector<uint8_t> packedBuf(fileEntry.compressed_size);

        file.read(reinterpret_cast<char*>(packedBuf.data()),
                  fileEntry.compressed_size);
        if (!file)
        {
            return false;
        }

        //TODO if (!ZipCompressor().Decompress(packedBuf, output))

        return false;
    }
    break;
    } // end switch
    return true;
}

bool PackArchive::HasMeta() const
{
    return pack_meta.get() != nullptr;
}

const pack_meta_data& PackArchive::GetMeta() const
{
    return *pack_meta;
}

PackArchive::info PackArchive::GetInfo() const
{
    info result;

    const pack_format::pack_file::footer_block& footer = pack_file.footer;

    result.infoCrc32 = footer.info_crc32;
    result.metaCrc32 = footer.meta_data_crc32;
    result.totalFiles = footer.info.num_files;

    return result;
}

std::string PackArchive::PrintMeta() const
{
    using namespace std;
    stringstream ss;

    info inf = GetInfo();

    stringstream info_ss;

    info_ss << "DVPK info\n"
            << left << setw(18) << "info_crc32: "
            << "0x" << hex << setfill('0') << setw(8) << right << inf.infoCrc32 << setfill(' ') << '\n'
            << left << setw(18) << "meta_data_crc32: "
            << "0x" << hex << setfill('0') << setw(8) << right << inf.metaCrc32 << setfill(' ') << '\n'
            << left << setw(18) << "total_files: " << dec << setfill(' ') << inf.totalFiles << '\n';

    if (HasMeta())
    {
        const pack_meta_data& meta = GetMeta();

        size_t numFiles = meta.get_num_files();

        // find out max filename
        auto max_it =
        max_element(begin(files_info), end(files_info),
                    [](const pack_format::file_info& l, const pack_format::file_info& r) {
                        return l.relativeFilePath.size() < r.relativeFilePath.size();
                    });

        size_t max_filename = max_it->relativeFilePath.size();
        size_t numbers_in_max_num = std::to_string(numFiles).size();

        stringstream table_header;

        table_header
        << setw(10) << "file-index"
        << " | "
        << setw(max_filename) << left << "file-name"
        << " | "
        << setw(10) << "pack-index"
        << " | "
        << setw(18) << "file-original-size"
        << " | "
        << setw(20) << "file-compressed-size"
        << " | "
        << setw(16) << "compressed-crc32"
        << " | "
        << setw(11) << "compression"
        << " | "
        << setw(14) << "original-crc32"
        << " | "
        << setw(14) << "start-position"
        << " |";

        string header = table_header.str();

        std::string separator_line = string(header.length(), '-');

        info_ss << separator_line << '\n';

        ss << info_ss.rdbuf();
        ss << "FILES\n"
           << header << '\n'
           << separator_line << '\n';

        const string compression_types[] = { "none", "lz4", "lz4hc", "deflate" };

        for (unsigned indexOfFile = 0; indexOfFile < numFiles; ++indexOfFile)
        {
            const auto& file_info = files_info.at(indexOfFile);
            const auto pack_index = meta.get_pack_index_for_file(indexOfFile);
            const string& compression = compression_types[file_info.compressionType];

            const auto& file_data = pack_file.files_table.data.files.at(indexOfFile);

            ss << left << "f" << setw(9) << indexOfFile << " | "
               << setw(max_filename) << file_info.relativeFilePath << " | "
               << 'p' << setw(9) << pack_index << " | "
               << setw(18) << file_info.originalSize << " | "
               << setw(20) << file_info.compressedSize << " | "
               << "0x" << setw(14) << hex << file_info.hash << " | "
               << setw(11) << compression << " | "
               << "0x" << setw(12) << hex << file_data.original_crc32 << " | "
               << setw(14) << dec << file_data.start_position << '\n';
        }
        ss << separator_line << '\n'
           << "END-FILES\n";

        size_t numPacks = meta.get_num_packs();
        string packName;
        string dependencies;
        size_t max_pack_name = strlen("pack-name");
        size_t max_dep_name = 0;
        for (unsigned i = 0; i < numPacks; ++i)
        {
            const auto& info = meta.get_pack_info(i);
            const std::string& packName = std::get<0>(info);
            const std::string& dependencies = std::get<1>(info);
            if (packName.length() > max_pack_name)
            {
                max_pack_name = packName.length();
            }
            if (dependencies.length() > max_dep_name)
            {
                max_dep_name = dependencies.length();
            }
        }

        table_header = stringstream(); // clear prev string stream

        table_header << setw(10) << left << "pack-index"
                     << " | "
                     << setw(max_pack_name) << left << "pack-name"
                     << " | "
                     << setw(max_dep_name) << left << "pack-dependency-indexes";

        header = table_header.str();

        separator_line = string(header.length(), '-');

        ss << "PACKS\n"
           << header << '\n'
           << separator_line << '\n';

        size_t numbers_in_max_pack = std::to_string(numPacks).size();
        for (unsigned indexOfPack = 0; indexOfPack < numPacks; ++indexOfPack)
        {
            const auto& info = meta.get_pack_info(indexOfPack);
            const std::string& packName = std::get<0>(info);
            const std::string& dependencies = std::get<1>(info);
            ss << "p" << setw(9) << indexOfPack << " | "
               << left << setw(max_pack_name)
               << packName << " | " << setw(max_dep_name)
               << dependencies << '\n';
        }
        ss << separator_line
           << "END-PACKS\n";

        ss << "START-DEPENDENCY\n";

        table_header = stringstream();
        table_header << setw(10) << "pack-index"
                     << " | "
                     << "num-of-dependencies"
                     << " | "
                     << "all-dependency-indexes |";
        header = table_header.str();
        separator_line = string(header.length(), '-');
        ss << header << '\n';
        ss << separator_line << '\n';

        for (unsigned indexOfPack = 0; indexOfPack < numPacks; ++indexOfPack)
        {
            const std::vector<uint32_t>& children = meta.get_children(indexOfPack);
            ss << "p" << setw(9) << indexOfPack << " | ";
            ss << setw(19) << children.size() << " | ";
            for (uint32_t child_index : children)
            {
                ss << "p" << child_index << " ";
            }
            ss << '\n';
        }
        ss << "-END-DEPENDENCY------------------------------------------------";
        ss << '\n';
    }

    return ss.str();
}

const std::uint32_t crc32_tab[256] = {
    0x00000000, 0x77073096, 0xee0e612c,
    0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832,
    0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
    0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d,
    0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a,
    0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8,
    0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd,
    0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
    0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180,
    0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e,
    0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab,
    0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589,
    0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
    0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4,
    0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1,
    0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf,
    0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074,
    0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
    0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f,
    0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525,
    0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822,
    0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320,
    0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
    0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b,
    0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268,
    0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76,
    0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43,
    0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
    0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6,
    0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c,
    0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9,
    0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7,
    0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
    0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82,
    0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7,
    0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd,
    0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca,
    0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
    0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d,
    0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53,
    0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330,
    0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e,
    0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
    0x2d02ef8d
};

std::uint32_t crc32_for_buffer(const char* data, std::uint32_t size)
{
    std::uint32_t crc32 = 0xffffffff;
    for (std::uint32_t i = 0; i < size; ++i)
    {
        crc32 = (crc32 >> 8) ^ crc32_tab[(crc32 ^ data[i]) & 0xff];
    }
    crc32 ^= 0xffffffff;
    return crc32;
}
