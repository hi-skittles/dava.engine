#pragma once

#include "EditorPhysics/Private/EditorPhysicsSystem.h"

#include <TArc/DataProcessing/DataNode.h>

class EditorPhysicsData : public DAVA::TArc::DataNode
{
public:
    EditorPhysicsSystem* system = nullptr;

private:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(EditorPhysicsData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<EditorPhysicsData>::Begin()
        .End();
    }
};
