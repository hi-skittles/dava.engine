#pragma once

#include "FileSystem/FilePath.h"

namespace DAVA
{
class Entity;
}

class SelectableGroup;
class SaveEntityAsAction
{
public:
    SaveEntityAsAction(const SelectableGroup* entities, const DAVA::FilePath& path);

    void Run();

protected:
    void RemoveLightmapsRecursive(DAVA::Entity* entity) const;

protected:
    const SelectableGroup* entities;
    DAVA::FilePath sc2Path;
};
