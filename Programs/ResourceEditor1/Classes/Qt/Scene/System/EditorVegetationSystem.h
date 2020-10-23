#pragma once

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class Entity;
class VegetationRenderObject;
}

class SceneEditor2;
class EditorVegetationSystem : public DAVA::SceneSystem
{
public:
    EditorVegetationSystem(DAVA::Scene* scene);

    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    void GetActiveVegetation(DAVA::Vector<DAVA::VegetationRenderObject*>& activeVegetationObjects);

    void ReloadVegetation();

private:
    DAVA::Vector<DAVA::VegetationRenderObject*> vegetationObjects;
};
