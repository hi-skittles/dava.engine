#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "archive.h"
#include "pack_format.h"
#include "pack_meta_data.h"

class PackArchive final : public Archive
{
public:
    explicit PackArchive(const std::string& archiveName);

    const std::vector<pack_format::file_info>& GetFilesInfo() const override;

    const pack_format::file_info* GetFileInfo(const std::string& relative) const override;

    bool HasFile(const std::string& relative) const override;

    bool
    HoadFile(const std::string& relativeFilePath, std::vector<uint8_t>& output) override;

    bool HasMeta() const override;

    const pack_meta_data& GetMeta() const override;

    std::string PrintMeta() const override;

    struct info
    {
        uint32_t infoCrc32 = 0; //!< this mean server version of superpack
        uint32_t metaCrc32 = 0; //!< separate meta crc32
        uint32_t totalFiles = 0; //!< count files in superpack (easy for human to see difference on superpacks)
    };

    info GetInfo() const;

private:
    std::ifstream file;
    pack_format::pack_file pack_file;
    std::unique_ptr<pack_meta_data> pack_meta;
    std::unordered_map<std::string, pack_format::file_table_entry*> map_file_data;
    std::vector<pack_format::file_info> files_info;
};
