#include "Scene3D/Components/DebugRenderComponent.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Base/GlobalEnum.h"

ENUM_DECLARE(DAVA::DebugRenderComponent::eDebugDrawFlags)
{
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_NONE, "DEBUG_DRAW_NONE");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_AABBOX, "DEBUG_DRAW_AABBOX");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_LOCAL_AXIS, "DEBUG_DRAW_LOCAL_AXIS");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_AABOX_CORNERS, "DEBUG_DRAW_AABOX_CORNERS");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_LIGHT_NODE, "DEBUG_DRAW_LIGHT_NODE");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_NORMALS, "DEBUG_DRAW_NORMALS");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_GRID, "DEBUG_DRAW_GRID");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_USERNODE, "DEBUG_DRAW_USERNODE");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_RED_AABBOX, "DEBUG_DRAW_RED_AABBOX");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_DRAW_CAMERA, "DEBUG_DRAW_CAMERA");
    ENUM_ADD_DESCR(DAVA::DebugRenderComponent::DEBUG_AUTOCREATED, "DEBUG_AUTOCREATED");
}

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(DebugRenderComponent)
{
    ReflectionRegistrator<DebugRenderComponent>::Begin()[M::CantBeCreatedManualyComponent()]
    .ConstructorByPointer()
    .Field("debugFlags", &DebugRenderComponent::GetDebugFlags, &DebugRenderComponent::SetDebugFlags)[M::DisplayName("Debug Flags"), M::FlagsT<eDebugDrawFlags>()]
    .End();
}

DebugRenderComponent::DebugRenderComponent()
    : curDebugFlags(DEBUG_DRAW_NONE)
{
}

DebugRenderComponent::~DebugRenderComponent()
{
}

void DebugRenderComponent::SetDebugFlags(uint32 debugFlags)
{
    curDebugFlags = debugFlags;
}

uint32 DebugRenderComponent::GetDebugFlags() const
{
    return curDebugFlags;
}

Component* DebugRenderComponent::Clone(Entity* toEntity)
{
    DebugRenderComponent* component = new DebugRenderComponent();
    component->SetEntity(toEntity);
    component->curDebugFlags = curDebugFlags;
    return component;
}

void DebugRenderComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    // Don't need to save
}

void DebugRenderComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    // Don't need to load
}
};
