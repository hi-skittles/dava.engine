#pragma once

#include <TArc/Core/ClientModule.h>

#include <Reflection/Reflection.h>

namespace DAVA
{
class ProjectResources;
} // namespace DAVA

namespace Mock
{
class ProjectManagerModule : public DAVA::ClientModule
{
public:
    static const DAVA::String testFolder;
    static const DAVA::String testProjectPath;
    static const DAVA::String testScenePath;

    ~ProjectManagerModule() override;

protected:
    void PostInit() override;

    std::unique_ptr<DAVA::ProjectResources> projectResources;

    DAVA_VIRTUAL_REFLECTION(ProjectManagerDummyModule, DAVA::ClientModule);
};
} // namespace Mock