#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Debug/DVAssert.h"
#include "Entity/Component.h"
#include "Base/Introspection.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class UserComponent : public Component
{
protected:
    ~UserComponent(){};

public:
    UserComponent();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

public:
    DAVA_VIRTUAL_REFLECTION(UserComponent, Component);
};
}
