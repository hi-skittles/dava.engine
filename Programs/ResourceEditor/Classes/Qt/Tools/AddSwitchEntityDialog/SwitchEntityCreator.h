#pragma once

#include <Base/BaseTypes.h>
#include <Render/Highlevel/RenderObject.h>

namespace DAVA
{
class Entity;
class RenderObject;
}

using RenderPair = std::pair<DAVA::Entity*, DAVA::RenderObject*>;

class SwitchEntityCreator
{
    static const size_t MAX_SWITCH_COUNT = 3u;

public:
    DAVA::RefPtr<DAVA::Entity> CreateSwitchEntity(const DAVA::Vector<DAVA::Entity*>& fromEntities);
    bool HasSwitchComponentsRecursive(DAVA::Entity* fromEntity);
    bool HasMeshRenderObjectsRecursive(DAVA::Entity* fromEntity);
    size_t GetRenderObjectsCountRecursive(DAVA::Entity* entity, DAVA::RenderObject::eType objectType);

private:
    void CreateSingleObjectData(DAVA::Entity* switchEntity);
    void CreateMultipleObjectsData();

    void FindRenderObjectsRecursive(DAVA::Entity* fromEntity, DAVA::Vector<RenderPair>& entityAndObjectPairs);

    DAVA::Vector<DAVA::Entity*> clonedEntities;
    DAVA::Vector<DAVA::Entity*> realChildren;

    DAVA::Array<DAVA::Vector<RenderPair>, MAX_SWITCH_COUNT> renderPairs;
    bool hasSkinnedMesh = false;
};
