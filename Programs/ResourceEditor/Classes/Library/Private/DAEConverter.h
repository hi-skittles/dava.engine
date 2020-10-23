#pragma once

#include "FileSystem/FilePath.h"

class DAEConverter final
{
public:
    static bool Convert(const DAVA::FilePath& path);
    static bool ConvertAnimations(const DAVA::FilePath& path);
};
