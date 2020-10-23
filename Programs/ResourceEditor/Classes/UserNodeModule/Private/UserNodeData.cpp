#include "Classes/UserNodeModule/Private/UserNodeData.h"
#include "Classes/UserNodeModule/Private/UserNodeSystem.h"

const char* UserNodeData::drawingEnabledPropertyName = "drawingEnabledPropertyName";

void UserNodeData::SetDrawingEnabled(bool enabled)
{
    DVASSERT(system);
    if (enabled)
    {
        system->EnableSystem();
    }
    else
    {
        system->DisableSystem();
    }
}

bool UserNodeData::IsDrawingEnabled() const
{
    DVASSERT(system);
    return system->IsSystemEnabled();
}
