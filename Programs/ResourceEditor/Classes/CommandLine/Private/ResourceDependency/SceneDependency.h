#pragma once

#include <Base/BaseTypes.h>
#include <FileSystem/FilePath.h>

class SceneDependency final
{
public:
    static bool GetDependencies(const DAVA::FilePath& scenePath, DAVA::Set<DAVA::FilePath>& dependencies, DAVA::int32 requestedType);
};
