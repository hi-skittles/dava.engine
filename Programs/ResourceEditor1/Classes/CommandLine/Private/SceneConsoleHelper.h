#pragma once

#include "FileSystem/FilePath.h"

namespace DAVA
{
class ProgramOptions;
}

class SceneConsoleHelper
{
public:
    static bool InitializeQualitySystem(const DAVA::ProgramOptions& options, const DAVA::FilePath& targetPathname);
    static void FlushRHI();
};
