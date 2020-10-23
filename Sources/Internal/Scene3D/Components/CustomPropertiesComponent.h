#pragma once

#include "Entity/Component.h"
#include "FileSystem/KeyedArchive.h"
#include "Reflection/Reflection.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class CustomPropertiesComponent : public Component
{
protected:
    virtual ~CustomPropertiesComponent();

public:
    CustomPropertiesComponent();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    KeyedArchive* GetArchive();

    //this method helps to load data for older scene file version
    void LoadFromArchive(const KeyedArchive& srcProperties, SerializationContext* serializationContext);

    DAVA_VIRTUAL_REFLECTION(CustomPropertiesComponent, Component);

private:
    CustomPropertiesComponent(const KeyedArchive& srcProperties);

private:
    KeyedArchive* properties;
};
};
