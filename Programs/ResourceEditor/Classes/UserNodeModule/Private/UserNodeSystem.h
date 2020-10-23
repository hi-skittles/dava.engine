#pragma once

#include <REPlatform/Scene/Systems/EditorSceneSystem.h>

#include <Entity/SceneSystem.h>
#include <Math/Matrix4.h>
#include <Math/Color.h>
#include <Base/BaseTypes.h>

namespace DAVA
{
class Entity;
class RenderObject;
class FilePath;
}

class UserNodeSystem : public DAVA::SceneSystem, public DAVA::EditorSceneSystem
{
public:
    UserNodeSystem(DAVA::Scene* scene, const DAVA::FilePath& scenePath);
    ~UserNodeSystem() override;

    void Process(DAVA::float32 timeElapsed) override;

    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    void SetVisible(bool visible);

protected:
    void Draw() override;

private:
    struct NodeDescription
    {
        DAVA::RenderObject* ro = nullptr;
        DAVA::Matrix4 transform;
    };

    bool IsSpawnNode(DAVA::Entity* entity) const;
    const DAVA::Color& GetSpawnColor(DAVA::Entity* entity) const;

    DAVA::Matrix4* GetWorldMatrixPtr(DAVA::Entity* entity) const;

    void RemoveOldSpawns();
    void FindNewSpawns(DAVA::Vector<DAVA::Entity*>& newSpawns);

    void UpdateSpawnVisibility();
    void UpdateTransformedEntities();

    void RemoveObject(DAVA::RenderObject* renderObject);
    void TransformObject(NodeDescription* description, const DAVA::Matrix4& entityTransform);

    DAVA::RenderObject* sourceObject = nullptr;
    DAVA::Matrix4 nodeMatrix;

    DAVA::UnorderedMap<DAVA::Entity*, NodeDescription> spawnNodes;
    DAVA::Vector<DAVA::Entity*> userNodes;

    bool isSystemVisible = true;
};
