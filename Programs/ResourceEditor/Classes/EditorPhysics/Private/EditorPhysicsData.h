#pragma once

#include "Classes/EditorPhysics/Private/EditorPhysicsSystem.h"

#include <TArc/DataProcessing/TArcDataNode.h>

class EditorPhysicsData : public DAVA::TArcDataNode
{
public:
    EditorPhysicsSystem* system = nullptr;

private:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(EditorPhysicsData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<EditorPhysicsData>::Begin()
        .End();
    }
};
