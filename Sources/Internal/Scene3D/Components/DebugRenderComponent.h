#pragma once

#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Entity/Component.h"
#include "Reflection/Reflection.h"
#include "Render/Highlevel/RenderObject.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
class DebugRenderComponent : public Component
{
public:
    enum eDebugDrawFlags : uint32
    {
        DEBUG_DRAW_NONE = 0x0,
        DEBUG_DRAW_AABBOX = 0x1,
        DEBUG_DRAW_LOCAL_AXIS = 0x2,
        DEBUG_DRAW_AABOX_CORNERS = 0x4,
        DEBUG_DRAW_LIGHT_NODE = 0x8,
        DEBUG_DRAW_NORMALS = 0x10,
        DEBUG_DRAW_GRID = 0x20,
        DEBUG_DRAW_USERNODE = 0x40,
        DEBUG_DRAW_RED_AABBOX = 0x80,
        DEBUG_DRAW_CAMERA = 0x100,

        DEBUG_AUTOCREATED = 0x80000000,
        DEBUG_DRAW_ALL = 0xFFFFFFFF,
    };

protected:
    virtual ~DebugRenderComponent();

public:
    DebugRenderComponent();

    void SetDebugFlags(uint32 debugFlags);
    uint32 GetDebugFlags() const;

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

private:
    uint32 curDebugFlags;

    DAVA_VIRTUAL_REFLECTION(DebugRenderComponent, Component);
};
};
