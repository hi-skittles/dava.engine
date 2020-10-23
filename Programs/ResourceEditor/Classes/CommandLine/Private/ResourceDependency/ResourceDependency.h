#pragma once

#include <Base/BaseTypes.h>
#include <FileSystem/FilePath.h>

class ResourceDependency final
{
public:
    static bool GetDependencies(const DAVA::Vector<DAVA::FilePath>& resourcePathes, DAVA::Map<DAVA::FilePath, DAVA::Set<DAVA::FilePath>>& dependencyMap, DAVA::int32 requestedType);
};
