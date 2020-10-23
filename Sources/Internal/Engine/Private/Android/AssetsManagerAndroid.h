#pragma once

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "FileSystem/ResourceArchive.h"

namespace DAVA
{
class ZipArchive;

class AssetsManagerAndroid : public Singleton<AssetsManagerAndroid>
{
public:
    explicit AssetsManagerAndroid(const String& packageName);
    virtual ~AssetsManagerAndroid();

    bool HasDirectory(const String& relativeDirName) const;
    bool HasFile(const String& relativeFilePath) const;
    bool LoadFile(const String& relativeFilePath, Vector<uint8>& output) const;
    bool ListDirectory(const String& relativeDirName, Vector<ResourceArchive::FileInfo>&) const;

private:
    String packageName;
    std::unique_ptr<ZipArchive> apk;
};
};
