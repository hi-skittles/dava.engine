#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"
#include "Render/Highlevel/RenderObject.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class RenderComponent : public Component
{
protected:
    virtual ~RenderComponent();

public:
    RenderComponent(RenderObject* _object = nullptr);

    void SetRenderObject(RenderObject* object);
    RenderObject* GetRenderObject();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void GetDataNodes(Set<DataNode*>& dataNodes) override;
    void OptimizeBeforeExport() override;

private:
    RenderObject* renderObject = nullptr;

    DAVA_VIRTUAL_REFLECTION(RenderComponent, Component);
};
}
