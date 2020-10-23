#pragma once

#include "Entity/Component.h"
#include "Scene3D/Components/Waypoint/PathComponent.h"
#include "Reflection/Reflection.h"
#include "Base/Introspection.h"

namespace DAVA
{
class SerializationContext;
class KeyedArchive;
class Entity;

class WaypointComponent : public Component
{
protected:
    ~WaypointComponent();

public:
    WaypointComponent();

    void Init(PathComponent* path, PathComponent::Waypoint* waypoint);
    PathComponent* GetPath() const;
    PathComponent::Waypoint* GetWaypoint() const;

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    const FastName& GetPathName() const;
    void SetPathName(const FastName& name);

    KeyedArchive* GetProperties() const;
    void SetProperties(KeyedArchive* arc);

    bool IsStartingPoint() const;

private:
    PathComponent* path = nullptr;
    PathComponent::Waypoint* waypoint = nullptr;

    DAVA_VIRTUAL_REFLECTION(WaypointComponent, Component);
};

inline const FastName& WaypointComponent::GetPathName() const
{
    DVASSERT(path != nullptr);
    DVASSERT(waypoint != nullptr);
    return path->GetName();
}

inline void WaypointComponent::SetPathName(const FastName& name)
{
    DVASSERT(false);
}

inline KeyedArchive* WaypointComponent::GetProperties() const
{
    DVASSERT(path != nullptr);
    DVASSERT(waypoint != nullptr);
    return waypoint->GetProperties();
}

inline void WaypointComponent::SetProperties(KeyedArchive* arc)
{
    DVASSERT(false);
}

inline bool WaypointComponent::IsStartingPoint() const
{
    DVASSERT(path != nullptr);
    DVASSERT(waypoint != nullptr);
    return waypoint->IsStarting();
}

} // namespace DAVA
