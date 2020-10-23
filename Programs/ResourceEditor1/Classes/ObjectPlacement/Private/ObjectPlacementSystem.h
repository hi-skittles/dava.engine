#pragma once

#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

#include <Base/BaseTypes.h>
#include <Entity/SceneSystem.h>

class EntityModificationSystem;

namespace DAVA
{
class Entity;
class LandscapeSystem;
class RayTraceCollision;
class RenderSystem;
struct Matrix4;
class Vector3;
}

class ObjectPlacementSystem : public DAVA::SceneSystem, public EditorSceneSystem
{
public:
    ObjectPlacementSystem(DAVA::Scene* scene);
    void SetSnapToLandscape(bool newSnapToLandscape);
    bool GetSnapToLandscape() const;
    void PlaceOnLandscape() const;
    void PlaceAndAlign() const;

    void RemoveEntity(DAVA::Entity* entity) override;
    void Process(DAVA::float32 time) override;

    void PrepareForRemove() override;

private:
    void GetObjectCollisionMatrixAndNormal(DAVA::RayTraceCollision& collision,
                                           DAVA::Vector3& translationVector, DAVA::Vector3& normal) const;

    // FIXME: pointer to scene modification system in ObjectPlacementSystem
    // As for now, Modification System is not reflective, doesn't
    // have its own TArc module or DataNode. So, to be able
    // to set Modification System's fields via Object Placement module's
    // reflective controls, we need to store it here.
    EntityModificationSystem* modificationSystem = nullptr;
    DAVA::RenderSystem* renderSystem = nullptr;
    DAVA::LandscapeSystem* landscapeSystem = nullptr;

    bool snapToLandscape = false;
    bool needCheckLandscapes = true;
};
