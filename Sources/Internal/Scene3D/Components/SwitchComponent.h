#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"
#include "Debug/DVAssert.h"
#include "Base/Introspection.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class SwitchComponent : public Component
{
protected:
    ~SwitchComponent(){};

public:
    SwitchComponent();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    void SetSwitchIndex(const int32& switchIndex);
    int32 GetSwitchIndex() const;

private:
    int32 oldSwitchIndex;
    int32 newSwitchIndex;

    friend class SwitchSystem;
    DAVA_VIRTUAL_REFLECTION(SwitchComponent, Component);
};
}
