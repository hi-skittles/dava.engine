#pragma once

#include <TArc/Core/ClientModule.h>

#include <Reflection/Reflection.h>

class ProjectResources;
namespace Mock
{
class ProjectManagerModule : public DAVA::TArc::ClientModule
{
public:
    static const DAVA::String testFolder;
    static const DAVA::String testProjectPath;
    static const DAVA::String testScenePath;

    ~ProjectManagerModule() override;

protected:
    void PostInit() override;

    std::unique_ptr<ProjectResources> projectResources;

    DAVA_VIRTUAL_REFLECTION(ProjectManagerDummyModule, DAVA::TArc::ClientModule);
};
} // namespace Mock