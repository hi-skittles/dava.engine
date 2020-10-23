#include "Test/Private/ProjectSettingsGuard.h"
#include "Test/Private/TestHelpers.h"

namespace TestHelpers
{
DAVA_VIRTUAL_REFLECTION_IMPL(ProjectSettingsGuard)
{
    DAVA::ReflectionRegistrator<ProjectSettingsGuard>::Begin()
    .ConstructorByPointer()
    .End();
}

void ProjectSettingsGuard::PostInit()
{
    using namespace DAVA;

    ContextAccessor* accessor = GetAccessor();
    {
        PropertiesItem item = accessor->CreatePropertiesNode(projectsHistoryKey);
        projectsHistory = item.Get<Vector<String>>(recentItemsKey);
        item.Set(recentItemsKey, Vector<String>());
    }
    {
        PropertiesItem item = accessor->CreatePropertiesNode(projectModulePropertiesKey);
        lastProject = item.Get<String>(lastProjectKey);
        item.Set(lastProjectKey, String());
    }
}

ProjectSettingsGuard::~ProjectSettingsGuard()
{
    using namespace DAVA;
    ContextAccessor* accessor = GetAccessor();
    {
        PropertiesItem item = accessor->CreatePropertiesNode(projectsHistoryKey);
        item.Set(recentItemsKey, projectsHistory);
    }
    {
        PropertiesItem item = accessor->CreatePropertiesNode(projectModulePropertiesKey);
        item.Set(lastProjectKey, lastProject);
    }
    TestHelpers::ClearTestFolder();
}
} // namespace TestHelpers
