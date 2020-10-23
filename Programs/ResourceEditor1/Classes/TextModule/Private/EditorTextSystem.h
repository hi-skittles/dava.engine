#pragma once

#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

#include <Entity/SceneSystem.h>
#include <Base/BaseTypes.h>

class EditorTextSystem : public DAVA::SceneSystem, public EditorSceneSystem
{
public:
    EditorTextSystem(DAVA::Scene* scene);

    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

protected:
    void Draw() override;

private:
    DAVA::Vector<DAVA::Entity*> textEntities;
};
