#pragma once

#include "FileSystem/ResourceArchive.h"

namespace DAVA
{
class ResourceArchiveImpl
{
public:
    virtual ~ResourceArchiveImpl() = default;

    virtual const Vector<ResourceArchive::FileInfo>& GetFilesInfo() const = 0;
    virtual const ResourceArchive::FileInfo* GetFileInfo(const String& relativeFilePath) const = 0;
    virtual bool HasFile(const String& relativeFilePath) const = 0;
    virtual bool LoadFile(const String& relativeFilePath, Vector<uint8>& output) const = 0;
};

} // end namespace DAVA
