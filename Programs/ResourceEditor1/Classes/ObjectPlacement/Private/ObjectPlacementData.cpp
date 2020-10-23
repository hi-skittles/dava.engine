#include "Classes/ObjectPlacement/Private/ObjectPlacementData.h"
#include "Classes/ObjectPlacement/Private/ObjectPlacementSystem.h"

const char* ObjectPlacementData::snapToLandscapePropertyName = "snapToLandscape";

bool ObjectPlacementData::GetSnapToLandscape() const
{
    DVASSERT(objectPlacementSystem != nullptr);
    return objectPlacementSystem->GetSnapToLandscape();
}

void ObjectPlacementData::SetSnapToLandscape(bool newSnapToLandscape)
{
    DVASSERT(objectPlacementSystem != nullptr);
    objectPlacementSystem->SetSnapToLandscape(newSnapToLandscape);
}

DAVA_VIRTUAL_REFLECTION_IMPL(ObjectPlacementData)
{
    DAVA::ReflectionRegistrator<ObjectPlacementData>::Begin()
    .Field(snapToLandscapePropertyName, &ObjectPlacementData::GetSnapToLandscape, nullptr)
    .End();
}
