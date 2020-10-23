#pragma once

#include "pack_format.h"
#include "pack_meta_data.h"

extern std::ofstream l; // for debug logging

struct Archive
{
    virtual ~Archive() = default;

    virtual const std::vector<pack_format::file_info>& GetFilesInfo() const = 0;

    virtual const pack_format::file_info* GetFileInfo(const std::string& relative) const = 0;

    virtual bool HasFile(const std::string& relative) const = 0;

    virtual bool HoadFile(const std::string& relative, std::vector<uint8_t>& output) = 0;

    virtual bool HasMeta() const = 0;

    virtual const pack_meta_data& GetMeta() const = 0;

    virtual std::string PrintMeta() const = 0;

    int32_t file_index = 0;
    std::string archive_name;
    std::string last_file_name;
};

Archive* Create(const std::string& file);
