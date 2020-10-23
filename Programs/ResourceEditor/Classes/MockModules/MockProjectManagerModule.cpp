#include "Classes/MockModules/MockProjectManagerModule.h"

#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>
#include <REPlatform/DataNodes/ProjectResources.h>

#include <Reflection/ReflectionRegistrator.h>

namespace Mock
{
const DAVA::String ProjectManagerModule::testFolder = DAVA::String("~doc:/Test/");
const DAVA::String ProjectManagerModule::testProjectPath = DAVA::String("~doc:/Test/SceneManagerTest/");
const DAVA::String ProjectManagerModule::testScenePath = DAVA::String("~doc:/Test/SceneManagerTest/DataSource/3d/Maps/scene.sc2");

ProjectManagerModule::~ProjectManagerModule()
{
    DAVA::CommandLineModuleTestUtils::ClearTestFolder(testFolder);
}

void ProjectManagerModule::PostInit()
{
    using namespace DAVA;

    // prepare test environment
    {
        CommandLineModuleTestUtils::CreateTestFolder(testFolder);
        CommandLineModuleTestUtils::CreateProjectInfrastructure(testProjectPath);
    }

    projectResources.reset(new DAVA::ProjectResources(GetAccessor()));
    projectResources->LoadProject(testProjectPath);
}

DAVA_VIRTUAL_REFLECTION_IMPL(ProjectManagerModule)
{
    DAVA::ReflectionRegistrator<ProjectManagerModule>::Begin()
    .ConstructorByPointer()
    .End();
}

} // namespace Mock