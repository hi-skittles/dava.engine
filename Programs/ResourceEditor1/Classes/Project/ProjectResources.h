#pragma once

#include "FileSystem/FilePath.h"

namespace DAVA
{
namespace TArc
{
class ContextAccessor;
}
}

/**
class is responsible at reading some project-related resources
*/
class ProjectManagerData;
class ProjectResources final
{
public:
    /**
    creates ProjectManagerData and some editor-independent fields in it
    */
    ProjectResources(DAVA::TArc::ContextAccessor*);

    /**
    unloads project data
    */
    ~ProjectResources();

    /**
    adds project Data folder to resources 
    and loads some config and information yamls into corresponding singletons
    */
    void LoadProject(const DAVA::FilePath& path);
    /**
    removes project Data folder from resources
    */
    void UnloadProject();

private:
    ProjectManagerData* GetProjectManagerData();

    DAVA::TArc::ContextAccessor* accessor;
};
