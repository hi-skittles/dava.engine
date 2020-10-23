#pragma once

#include "FileSystem/FilePath.h"

namespace DAVA
{
class Entity;
class SelectableGroup;
}

class SaveEntityAsAction
{
public:
    SaveEntityAsAction(const DAVA::SelectableGroup* entities, const DAVA::FilePath& path);

    void Run();

protected:
    void RemoveLightmapsRecursive(DAVA::Entity* entity) const;

protected:
    const DAVA::SelectableGroup* entities;
    DAVA::FilePath sc2Path;
};
