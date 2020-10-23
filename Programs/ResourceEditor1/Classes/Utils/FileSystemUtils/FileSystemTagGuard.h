#pragma once

#include <Base/String.h>

class FileSystemTagGuard final
{
public:
    FileSystemTagGuard(const DAVA::String newFilenamesTag);
    ~FileSystemTagGuard();

private:
    DAVA::String oldFilenamesTag;
};
