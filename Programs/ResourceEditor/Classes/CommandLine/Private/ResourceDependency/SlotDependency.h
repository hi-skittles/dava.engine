#pragma once

#include <Base/BaseTypes.h>
#include <FileSystem/FilePath.h>

class SlotDependency final
{
public:
    static bool GetDependencies(const DAVA::FilePath& texturePath, DAVA::Set<DAVA::FilePath>& dependencies, DAVA::int32 requestedType);
};
