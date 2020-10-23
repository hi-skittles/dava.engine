#pragma once

#include "Classes/UserNodeModule/Private/UserNodeSystem.h"

#include "TArc/DataProcessing/DataNode.h"
#include <memory>

namespace DAVA
{
class Entity;
}

class UserNodeData : public DAVA::TArc::DataNode
{
private:
    friend class UserNodeModule;

    static const char* drawingEnabledPropertyName;

    void SetDrawingEnabled(bool enabled);
    bool IsDrawingEnabled() const;

    std::unique_ptr<UserNodeSystem> system;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(UserNodeData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<UserNodeData>::Begin()
        .Field(drawingEnabledPropertyName, &UserNodeData::IsDrawingEnabled, &UserNodeData::SetDrawingEnabled)
        .End();
    }
};
