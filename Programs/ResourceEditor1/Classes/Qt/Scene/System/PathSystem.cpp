#include "Classes/Qt/Scene/System/PathSystem.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Application/REGlobal.h"

#include "Classes/Commands2/Base/RECommandNotificationObject.h"
#include "Classes/Commands2/InspMemberModifyCommand.h"
#include "Classes/Commands2/WayEditCommands.h"
#include "Classes/Qt/Scene/SceneEditor2.h"

#include "Classes/Selection/Selection.h"

#include <Scene3D/Components/Waypoint/PathComponent.h>
#include <Scene3D/Components/Waypoint/WaypointComponent.h>
#include <Scene3D/Components/Waypoint/EdgeComponent.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Entity.h>
#include <FileSystem/KeyedArchive.h>
#include <Utils/Utils.h>

namespace PathSystemDetail
{
const DAVA::Array<DAVA::Color, 16> PathColorPallete =
{ { DAVA::Color(0x00ffffff),
    DAVA::Color(0x000000ff),
    DAVA::Color(0x0000ffff),
    DAVA::Color(0xff00ffff),

    DAVA::Color(0x808080ff),
    DAVA::Color(0x008000ff),
    DAVA::Color(0x00ff00ff),
    DAVA::Color(0x80000ff),

    DAVA::Color(0x000080ff),
    DAVA::Color(0x808000ff),
    DAVA::Color(0x800080ff),
    DAVA::Color(0xff0000ff),

    DAVA::Color(0xc0c0c0ff),
    DAVA::Color(0x008080ff),
    DAVA::Color(0xffffffff),
    DAVA::Color(0xffff00ff) } };

const DAVA::String PATH_COLOR_PROP_NAME = "pathColor";

struct WaypointKey
{
    DAVA::PathComponent* path = nullptr;
    DAVA::PathComponent::Waypoint* waypoint = nullptr;

    bool operator==(const WaypointKey& other) const
    {
        return path == other.path && waypoint == other.waypoint;
    }
};

class WaypointHash
{
public:
    size_t operator()(const WaypointKey& s) const
    {
        size_t h1 = std::hash<const DAVA::PathComponent*>()(s.path);
        size_t h2 = std::hash<const DAVA::PathComponent::Waypoint*>()(s.waypoint);
        return h1 ^ (h2 << 1);
    }
};

struct EdgeKey
{
    DAVA::PathComponent* path = nullptr;
    DAVA::PathComponent::Edge* edge = nullptr;

    bool operator==(const EdgeKey& other) const
    {
        return path == other.path && edge == other.edge;
    }
};

class EdgeHash
{
public:
    size_t operator()(const EdgeKey& s) const
    {
        size_t h1 = std::hash<const DAVA::PathComponent*>()(s.path);
        size_t h2 = std::hash<const DAVA::PathComponent::Edge*>()(s.edge);
        return h1 ^ (h2 << 1);
    }
};

struct MappingValue
{
    DAVA::RefPtr<DAVA::Entity> entity;
    bool existsInSourceData = false;
};

struct EdgeMappingValue : MappingValue
{
    DAVA::EdgeComponent* component = nullptr;
};
}

PathSystem::PathSystem(DAVA::Scene* scene)
    : DAVA::SceneSystem(scene)
    , currentPath(NULL)
    , isEditingEnabled(false)
{
}

PathSystem::~PathSystem()
{
    currentPath = NULL;

    pathes.clear();
    currentSelection.Clear();

    for (const auto& iter : edgeComponentCache)
    {
        delete iter.second;
    }
}

void PathSystem::AddEntity(DAVA::Entity* entity)
{
    if (isEditingEnabled == false && !currentPath)
    {
        currentPath = entity;
    }

    // extract color data from custom properties for old scenes
    DAVA::PathComponent* pc = GetPathComponent(entity);
    DVASSERT(pc != nullptr);
    if (pc->GetColor() == DAVA::Color())
    {
        DAVA::KeyedArchive* props = GetCustomPropertiesArchieve(entity);
        if (props && props->IsKeyExists(PathSystemDetail::PATH_COLOR_PROP_NAME))
        {
            pc->SetColor(DAVA::Color(props->GetVector4(PathSystemDetail::PATH_COLOR_PROP_NAME)));
            props->DeleteKey(PathSystemDetail::PATH_COLOR_PROP_NAME);
        }
    }

    if (isEditingEnabled == true)
    {
        entitiesForExpand.insert(entity);
    }

    InitPathComponent(pc);
    pathes.push_back(entity);
}

void PathSystem::RemoveEntity(DAVA::Entity* entity)
{
    DAVA::PathComponent* pc = GetPathComponent(entity);
    if (isEditingEnabled == true && pc != nullptr)
    {
        entitiesForCollapse.emplace(entity, pc->GetName());
    }

    DAVA::FindAndRemoveExchangingWithLast(pathes, entity);

    if (pathes.size())
    {
        if (entity == currentPath)
        {
            currentPath = pathes[0];
        }
    }
    else
    {
        currentPath = nullptr;
    }
}

void PathSystem::PrepareForRemove()
{
    pathes.clear();
    currentPath = nullptr;
    entitiesForCollapse.clear();
}

void PathSystem::WillClone(DAVA::Entity* originalEntity)
{
    DAVA::PathComponent* pc = DAVA::GetPathComponent(originalEntity);
    if (isEditingEnabled && pc != nullptr)
    {
        CollapsePathEntity(originalEntity, pc->GetName());
    }
}

void PathSystem::DidCloned(DAVA::Entity* originalEntity, DAVA::Entity* newEntity)
{
    if (isEditingEnabled)
    {
        if (GetPathComponent(originalEntity) != nullptr)
        {
            entitiesForExpand.insert(originalEntity);
        }

        if (GetPathComponent(newEntity) != nullptr)
        {
            entitiesForExpand.insert(newEntity);
        }
    }
}

void PathSystem::OnWaypointAdded(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint)
{
    entitiesForExpand.insert(path->GetEntity());
}

void PathSystem::OnWaypointRemoved(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint)
{
    entitiesForExpand.insert(path->GetEntity());
}

void PathSystem::OnEdgeAdded(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge)
{
    entitiesForExpand.insert(path->GetEntity());
}

void PathSystem::OnEdgeRemoved(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge)
{
    entitiesForExpand.insert(path->GetEntity());
}

void PathSystem::OnWaypointDeleted(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint)
{
    size_t erasedCount = entityCache.erase(waypoint);
    DVASSERT(erasedCount > 0);
}

void PathSystem::OnEdgeDeleted(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge)
{
    auto iter = edgeComponentCache.find(edge);
    if (iter != edgeComponentCache.end())
    {
        SafeDelete(iter->second);
        edgeComponentCache.erase(iter);
    }
}

void PathSystem::Draw()
{
    const DAVA::uint32 count = static_cast<DAVA::uint32>(pathes.size());
    if (!count)
        return;

    if (isEditingEnabled)
    {
        DrawInEditableMode();
    }
    else
    {
        DrawInViewOnlyMode();
    }
}

void PathSystem::DrawInEditableMode()
{
    for (DAVA::Entity* path : pathes)
    {
        DAVA::PathComponent* pc = GetPathComponent(path);
        if (!path->GetVisible() || !pc)
        {
            continue;
        }

        const DAVA::uint32 childrenCount = path->GetChildrenCount();
        for (DAVA::uint32 c = 0; c < childrenCount; ++c)
        {
            DAVA::Entity* waypoint = path->GetChild(c);

            const DAVA::uint32 edgesCount = waypoint->GetComponentCount<DAVA::EdgeComponent>();
            if (edgesCount)
            {
                DAVA::Vector3 startPosition = GetTransformComponent(waypoint)->GetWorldTransform().GetTranslationVector();
                startPosition.z += WAYPOINTS_DRAW_LIFTING;
                for (DAVA::uint32 e = 0; e < edgesCount; ++e)
                {
                    DAVA::EdgeComponent* edge = waypoint->GetComponent<DAVA::EdgeComponent>(e);
                    DAVA::Entity* nextEntity = edge->GetNextEntity();
                    if (nextEntity && nextEntity->GetParent())
                    {
                        DAVA::Vector3 finishPosition = GetTransformComponent(nextEntity)->GetWorldTransform().GetTranslationVector();
                        finishPosition.z += WAYPOINTS_DRAW_LIFTING;
                        DrawArrow(startPosition, finishPosition, pc->GetColor());
                    }
                }
            }
        }
    }
}

void PathSystem::DrawInViewOnlyMode()
{
    GlobalSceneSettings* settings = REGlobal::GetGlobalContext()->GetData<GlobalSceneSettings>();
    const DAVA::float32 boxScale = settings->debugBoxWaypointScale;

    const SelectableGroup& selection = Selection::GetSelection();
    for (auto entity : selection.ObjectsOfType<DAVA::Entity>())
    {
        DAVA::PathComponent* pathComponent = DAVA::GetPathComponent(entity);
        if (entity->GetVisible() == false || !pathComponent)
        {
            continue;
        }

        const DAVA::Vector<DAVA::PathComponent::Waypoint*>& waypoints = pathComponent->GetPoints();
        for (auto waypoint : waypoints)
        {
            DAVA::Vector3 startPosition = waypoint->position;
            const DAVA::AABBox3 wpBoundingBox(startPosition, boxScale);
            const auto& transform = entity->GetWorldTransform();
            bool isStarting = waypoint->IsStarting();

            GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawAABoxTransformed(wpBoundingBox, transform, DAVA::Color(0.3f, 0.3f, isStarting ? 1.0f : 0.0f, 0.3f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
            GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawAABoxTransformed(wpBoundingBox, transform, DAVA::Color(0.7f, 0.7f, isStarting ? 0.7f : 0.0f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);

            //draw edges
            if (!waypoint->edges.empty())
            {
                startPosition.z += WAYPOINTS_DRAW_LIFTING;
                for (auto edge : waypoint->edges)
                {
                    DAVA::Vector3 finishPosition = edge->destination->position;
                    finishPosition.z += WAYPOINTS_DRAW_LIFTING;
                    DrawArrow(startPosition * transform, finishPosition * transform, pathComponent->GetColor());
                }
            }
        }
    }
}

void PathSystem::DrawArrow(const DAVA::Vector3& start, const DAVA::Vector3& finish, const DAVA::Color& color)
{
    GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawArrow(start, finish, DAVA::Min((finish - start).Length() / 4.f, 4.f), DAVA::ClampToUnityRange(color), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
}

void PathSystem::InitPathComponent(DAVA::PathComponent* component)
{
    if (component->GetName().IsValid() == false)
    {
        component->SetName(GeneratePathName());
    }

    if (component->GetColor() == DAVA::Color())
    {
        component->SetColor(GetNextPathColor());
    }

    const DAVA::Vector<DAVA::PathComponent::Waypoint*>& points = component->GetPoints();
    for (DAVA::PathComponent::Waypoint* waypoint : points)
    {
        if (waypoint->GetProperties() == nullptr)
        {
            DAVA::RefPtr<DAVA::KeyedArchive> props(new DAVA::KeyedArchive());
            waypoint->SetProperties(props.Get());
        }

        for (DAVA::PathComponent::Edge* edge : waypoint->edges)
        {
            if (edge->GetProperties() == nullptr)
            {
                DAVA::RefPtr<DAVA::KeyedArchive> props(new DAVA::KeyedArchive());
                edge->SetProperties(props.Get());
            }
        }
    }
}

void PathSystem::Process(DAVA::float32 timeElapsed)
{
    for (const auto& node : entitiesForCollapse)
    {
        CollapsePathEntity(node.first, node.second);
    }
    entitiesForCollapse.clear();

    for (DAVA::Entity* entityToExpand : entitiesForExpand)
    {
        ExpandPathEntity(entityToExpand);
    }
    entitiesForExpand.clear();

    if (isEditingEnabled == false)
    {
        return;
    }

    const SelectableGroup& selection = Selection::GetSelection();
    if (currentSelection != selection)
    {
        currentSelection.Clear();
        currentSelection.Join(selection);

        for (auto entity : currentSelection.ObjectsOfType<DAVA::Entity>())
        {
            if (GetPathComponent(entity) != nullptr)
            {
                currentPath = entity;
                break;
            }

            if (GetWaypointComponent(entity) && GetPathComponent(entity->GetParent()))
            {
                currentPath = entity->GetParent();
                break;
            }
        }
    }
}

DAVA::FastName PathSystem::GeneratePathName() const
{
    const DAVA::uint32 count = static_cast<DAVA::uint32>(pathes.size());
    for (DAVA::uint32 i = 0; i <= count; ++i)
    {
        DAVA::FastName generatedName(DAVA::Format("path_%02d", i));

        bool found = false;

        for (DAVA::uint32 p = 0; p < count; ++p)
        {
            const DAVA::PathComponent* pc = DAVA::GetPathComponent(pathes[p]);
            if (generatedName == pc->GetName())
            {
                found = true;
                break;
            }
        }

        if (!found)
            return generatedName;
    }

    return DAVA::FastName();
}

const DAVA::Color& PathSystem::GetNextPathColor() const
{
    const DAVA::uint32 count = static_cast<DAVA::uint32>(pathes.size());
    const DAVA::uint32 index = count % static_cast<DAVA::uint32>(PathSystemDetail::PathColorPallete.size());

    return PathSystemDetail::PathColorPallete[index];
}

void PathSystem::EnablePathEdit(bool enable)
{
    SceneEditor2* sceneEditor = static_cast<SceneEditor2*>(GetScene());
    DVASSERT(isEditingEnabled != enable);
    isEditingEnabled = enable;
    if (enable)
    {
        for (DAVA::Entity* pathEntity : pathes)
        {
            entitiesForExpand.insert(pathEntity);
        }
    }
    else
    {
        for (DAVA::Entity* pathEntity : pathes)
        {
            DAVA::PathComponent* path = DAVA::GetPathComponent(pathEntity);
            DVASSERT(path != nullptr);
            entitiesForCollapse.emplace(pathEntity, path->GetName());
        }
    }
}

void PathSystem::ExpandPathEntity(DAVA::Entity* pathEntity)
{
    using namespace DAVA;
    using namespace PathSystemDetail;

    UnorderedMap<WaypointKey, MappingValue, WaypointHash> waypointToEntity;
    UnorderedMap<EdgeKey, EdgeMappingValue, EdgeHash> edgeToEntity;

    auto lookUpWaypointEntity = [this, &waypointToEntity, pathEntity](PathComponent* path, PathComponent::Waypoint* waypoint) -> RefPtr<Entity>
    {
        WaypointKey key;
        key.path = path;
        key.waypoint = waypoint;

        MappingValue& value = waypointToEntity[key];
        value.existsInSourceData = true;
        if (value.entity.Get() == nullptr)
        {
            auto iter = entityCache.find(waypoint);
            if (iter != entityCache.end())
            {
                value.entity = iter->second;
                WaypointComponent* component = GetWaypointComponent(value.entity.Get());
                DVASSERT(component != nullptr);
                DVASSERT(component->GetPath() == path);
                DVASSERT(component->GetWaypoint() == waypoint);
            }
            else
            {
                value.entity.ConstructInplace();
                value.entity->SetName(waypoint->name);
                if (waypoint->IsStarting())
                {
                    value.entity->SetNotRemovable(true);
                }

                WaypointComponent* wpComponent = new WaypointComponent();
                wpComponent->Init(path, waypoint);
                value.entity->AddComponent(wpComponent);

                entityCache.emplace(waypoint, value.entity);
            }

            Matrix4 m;
            m.SetTranslationVector(waypoint->position);
            value.entity->SetLocalTransform(m);
            pathEntity->AddNode(value.entity.Get());
        }

        return value.entity;
    };

    for (int32 i = 0; i < pathEntity->GetChildrenCount(); ++i)
    {
        Entity* child = pathEntity->GetChild(i);
        WaypointComponent* component = GetWaypointComponent(child);
        if (component == nullptr)
        {
            continue;
        }

        WaypointKey key;
        key.path = component->GetPath();
        key.waypoint = component->GetWaypoint();

        MappingValue value;
        value.entity = RefPtr<Entity>::ConstructWithRetain(child);

        waypointToEntity.emplace(key, value);
        if (entityCache.count(key.waypoint) == 0)
        {
            entityCache[key.waypoint] = value.entity;
        }

        for (uint32 edgeIndex = 0; edgeIndex < child->GetComponentCount<EdgeComponent>(); ++edgeIndex)
        {
            EdgeComponent* component = child->GetComponent<EdgeComponent>(edgeIndex);

            EdgeKey key;
            key.path = component->GetPath();
            key.edge = component->GetEdge();

            EdgeMappingValue value;
            value.entity = RefPtr<Entity>::ConstructWithRetain(child);
            value.component = component;

            edgeToEntity.emplace(key, value);
        }
    }

    PathComponent* path = GetPathComponent(pathEntity);
    DVASSERT(path != nullptr);
    for (PathComponent::Waypoint* waypoint : path->GetPoints())
    {
        lookUpWaypointEntity(path, waypoint);

        for (PathComponent::Edge* edge : waypoint->edges)
        {
            EdgeKey edgeKey;
            edgeKey.path = path;
            edgeKey.edge = edge;

            EdgeMappingValue& value = edgeToEntity[edgeKey];
            value.existsInSourceData = true;
            if (value.component == nullptr)
            {
                RefPtr<Entity> srcWaypointEntity = lookUpWaypointEntity(path, waypoint);
                value.entity = srcWaypointEntity;
                auto iter = edgeComponentCache.find(edge);
                if (iter != edgeComponentCache.end())
                {
                    value.component = iter->second;
                    DVASSERT(value.component->GetPath() == path);
                    DVASSERT(value.component->GetEdge() == edge);
                    edgeComponentCache.erase(edge);
                }
                else
                {
                    value.component = new EdgeComponent();
                    value.component->Init(path, edge);
                }
                value.entity->AddComponent(value.component);
            }

            RefPtr<Entity> destinationEntity = lookUpWaypointEntity(path, edge->destination);
            value.component->SetNextEntity(destinationEntity.Get());
        }
    }

    Set<RefPtr<Entity>> entitiesToRemove;
    for (const auto& node : waypointToEntity)
    {
        if (node.second.existsInSourceData == false)
        {
            DVASSERT(entityCache.count(node.first.waypoint) > 0);
            entitiesToRemove.insert(node.second.entity);
        }
    }
    waypointToEntity.clear();

    for (const auto& node : edgeToEntity)
    {
        if (node.second.existsInSourceData == false)
        {
            node.second.component->SetNextEntity(nullptr);
            node.second.entity->DetachComponent(node.second.component);
            edgeComponentCache.emplace(node.first.edge, node.second.component);
        }
    }
    edgeToEntity.clear();

    for (const RefPtr<Entity>& e : entitiesToRemove)
    {
        pathEntity->RemoveNode(e.Get());
    }
    entitiesToRemove.clear();
}

void PathSystem::CollapsePathEntity(DAVA::Entity* pathEntity, DAVA::FastName pathName)
{
    using namespace DAVA;

    SceneEditor2* sceneEditor = static_cast<SceneEditor2*>(GetScene());

    Vector<Entity*> edgeChildren;
    pathEntity->GetChildEntitiesWithComponent(edgeChildren, Type::Instance<EdgeComponent>());
    for (Entity* edgeEntity : edgeChildren)
    {
        uint32 count = edgeEntity->GetComponentCount<EdgeComponent>();
        Vector<EdgeComponent*> edgeComponents;
        edgeComponents.reserve(count);
        for (uint32 i = 0; i < count; ++i)
        {
            EdgeComponent* edgeComponent = edgeEntity->GetComponent<EdgeComponent>(i);
            edgeComponents.push_back(edgeComponent);
            DVASSERT(edgeComponent->GetEdge() != nullptr);
            bool added = edgeComponentCache.emplace(edgeComponent->GetEdge(), edgeComponent).second;
            DVASSERT(added == true);
        }

        for (EdgeComponent* edgeComponent : edgeComponents)
        {
            edgeEntity->DetachComponent(edgeComponent);
        }
    }

    Vector<Entity*> children;
    pathEntity->GetChildEntitiesWithComponent(children, Type::Instance<WaypointComponent>());
    for (Entity* wpEntity : children)
    {
        WaypointComponent* wpComponent = GetWaypointComponent(wpEntity);
        if (pathName == wpComponent->GetPathName())
        {
            DVASSERT(entityCache.count(wpComponent->GetWaypoint()) > 0);
            pathEntity->RemoveNode(wpEntity);
        }
    }
}
