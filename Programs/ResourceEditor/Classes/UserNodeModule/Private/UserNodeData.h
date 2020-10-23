#pragma once

#include "Classes/UserNodeModule/Private/UserNodeSystem.h"

#include "TArc/DataProcessing/TArcDataNode.h"
#include <memory>

namespace DAVA
{
class Entity;
}

class UserNodeData : public DAVA::TArcDataNode
{
private:
    friend class UserNodeModule;

    static const char* drawingEnabledPropertyName;

    void SetDrawingEnabled(bool enabled);
    bool IsDrawingEnabled() const;

    std::unique_ptr<UserNodeSystem> system;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(UserNodeData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<UserNodeData>::Begin()
        .Field(drawingEnabledPropertyName, &UserNodeData::IsDrawingEnabled, &UserNodeData::SetDrawingEnabled)
        .End();
    }
};
