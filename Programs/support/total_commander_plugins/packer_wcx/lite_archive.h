#pragma once

#include <fstream>
#include "archive.h"
#include "pack_meta_data.h"

class LiteArchive final : public Archive
{
public:
    LiteArchive(const std::string& file);
    ~LiteArchive() override = default;

    const std::vector<pack_format::file_info>& GetFilesInfo() const override;

    const pack_format::file_info* GetFileInfo(const std::string& relative) const override;

    bool HasFile(const std::string& relative) const override;

    bool HoadFile(const std::string& relative, std::vector<uint8_t>& output) override;

    bool HasMeta() const override;

    const pack_meta_data& GetMeta() const override;

    std::string PrintMeta() const override;

private:
    std::ifstream ifile;
    std::string file_name;
    pack_format::LitePack::footer footer;
    std::vector<pack_format::file_info> file_info;
};
