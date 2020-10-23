#pragma once

#include <TArc/Core/ClientModule.h>

namespace TestHelpers
{
class ProjectSettingsGuard : public DAVA::ClientModule
{
public:
    ~ProjectSettingsGuard();

protected:
    void PostInit() override;

    //last project properties names
    const DAVA::String projectModulePropertiesKey = "ProjectModuleProperties";
    const DAVA::String lastProjectKey = "Last project";

    //recent items properties names
    const DAVA::String projectsHistoryKey = "Projects history";
    const DAVA::String recentItemsKey = "recent items";

    DAVA::Vector<DAVA::String> projectsHistory;
    DAVA::String lastProject;

    DAVA_VIRTUAL_REFLECTION(ProjectSettingsGuard, DAVA::ClientModule);
};
} //namespace TestHelpers
