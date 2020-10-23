#pragma once

#include <TArc/DataProcessing/TArcDataNode.h>
#include <Reflection/ReflectionRegistrator.h>

#include <memory>

class ObjectPlacementSystem;

class ObjectPlacementData : public DAVA::TArcDataNode
{
public:
    friend class ObjectPlacementModule;

    static const char* snapToLandscapePropertyName;
    bool GetSnapToLandscape() const;
    void SetSnapToLandscape(bool newSnapToLandscape);

private:
    std::unique_ptr<ObjectPlacementSystem> objectPlacementSystem;

    DAVA_VIRTUAL_REFLECTION(ObjectPlacementData, DAVA::TArcDataNode);
};
