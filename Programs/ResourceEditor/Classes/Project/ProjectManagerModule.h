#pragma once

#include <REPlatform/DataNodes/ProjectResources.h>

#include <TArc/Models/RecentMenuItems.h>
#include <TArc/Core/ClientModule.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Utils/QtDelayedExecutor.h>

namespace DAVA
{
class ProjectManagerData;
} // namespace DAVA

class ProjectManagerModule : public DAVA::ClientModule
{
public:
    ProjectManagerModule();
    ~ProjectManagerModule();

protected:
    void PostInit() override;

private:
    void CreateActions();
    void CreateTagsActions();
    void RemoveTagsActions();
    void RegisterOperations();

    void OpenProject();
    void OpenProjectByPath(const DAVA::FilePath& incomePath);
    void OpenProjectImpl(const DAVA::FilePath& incomePath);
    void OpenLastProject();
    bool CloseProject();
    void ReloadSprites();

    bool GetTagsEnabled();
    void SetFilenamesTag(const DAVA::String& tag);

private:
    DAVA::ProjectManagerData* GetData();

private:
    std::unique_ptr<DAVA::RecentMenuItems> recentProjects;
    DAVA::QtConnections connections;
    DAVA::QtDelayedExecutor delayedExecutor;
    std::unique_ptr<DAVA::ProjectResources> projectResources;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ProjectManagerModule, DAVA::ClientModule)
    {
        DAVA::ReflectionRegistrator<ProjectManagerModule>::Begin()
        .ConstructorByPointer()
        .Field("tagsEnabled", &ProjectManagerModule::GetTagsEnabled, nullptr)
        .End();
    }
};
