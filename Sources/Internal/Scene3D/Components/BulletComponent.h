#pragma once

#include "Base/BaseTypes.h"
#include "Entity/Component.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class BaseObject;
class BulletComponent : public Component
{
protected:
    virtual ~BulletComponent();

public:
    BulletComponent();

    void SetBulletObject(BaseObject* bulletObject);
    BaseObject* GetBulletObject();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

private:
    BaseObject* bulletObject;

    DAVA_VIRTUAL_REFLECTION(BulletComponent, Component);
};
}
