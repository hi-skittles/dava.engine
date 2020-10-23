#include "Classes/DebugDraw/Private/DebugDrawSystem.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/DebugDraw/Private/DebugDrawSystem.h"
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Qt/Scene/System/LandscapeEditorDrawSystem/LandscapeProxy.h"
#include "Classes/Qt/Scene/System/BeastSystem.h"
#include "Classes/Qt/Scene/System/LandscapeEditorDrawSystem/LandscapeProxy.h"
#include "Classes/Qt/Scene/System/LandscapeEditorDrawSystem.h"
#include "Classes/Selection/Selection.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Deprecated/EditorConfig.h"
#include "Classes/Deprecated/SceneValidator.h"

#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/GeoDecalComponent.h>
#include <Render/Highlevel/GeometryOctTree.h>

#include <Entity/ComponentManager.h>
#include <Engine/Engine.h>
#include <Base/Type.h>

#define DAVA_ALLOW_OCTREE_DEBUG 0

const DAVA::float32 DebugDrawSystem::HANGING_OBJECTS_DEFAULT_HEIGHT = 0.001f;

DebugDrawSystem::DebugDrawSystem(DAVA::Scene* scene)
    : DAVA::SceneSystem(scene)
{
    SceneEditor2* sc = (SceneEditor2*)GetScene();

    collSystem = sc->collisionSystem;

    DVASSERT(NULL != collSystem);

    drawComponentFunctionsMap[DAVA::Type::Instance<DAVA::SoundComponent>()] = MakeFunction(this, &DebugDrawSystem::DrawSoundNode);
    drawComponentFunctionsMap[DAVA::Type::Instance<DAVA::WindComponent>()] = MakeFunction(this, &DebugDrawSystem::DrawWindNode);
    drawComponentFunctionsMap[DAVA::Type::Instance<DAVA::GeoDecalComponent>()] = MakeFunction(this, &DebugDrawSystem::DrawDecals);
    drawComponentFunctionsMap[DAVA::Type::Instance<DAVA::LightComponent>()] = DAVA::Bind(&DebugDrawSystem::DrawLightNode, this, DAVA::_1, false);
}

DebugDrawSystem::~DebugDrawSystem()
{
}

void DebugDrawSystem::SetRequestedObjectType(ResourceEditor::eSceneObjectType _objectType)
{
    objectType = _objectType;

    if (ResourceEditor::ESOT_NONE != objectType)
    {
        ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
        DVASSERT(data);
        const DAVA::Vector<DAVA::Color>& colors = data->GetEditorConfig()->GetColorPropertyValues("CollisionTypeColor");
        if ((DAVA::uint32)objectType < (DAVA::uint32)colors.size())
        {
            objectTypeColor = colors[objectType];
        }
        else
        {
            objectTypeColor = DAVA::Color(1.f, 0, 0, 1.f);
        }
    }
}

ResourceEditor::eSceneObjectType DebugDrawSystem::GetRequestedObjectType() const
{
    return objectType;
}

void DebugDrawSystem::AddEntity(DAVA::Entity* entity)
{
    entities.push_back(entity);

    DAVA::ComponentManager* cm = DAVA::GetEngineContext()->componentManager;
    const DAVA::Vector<const DAVA::Type*> componentsTypes = cm->GetRegisteredSceneComponents();

    for (const DAVA::Type* type : componentsTypes)
    {
        for (DAVA::uint32 index = 0, count = entity->GetComponentCount(type); index < count; ++index)
        {
            DAVA::Component* component = entity->GetComponent(type, index);
            RegisterComponent(entity, component);
        }
    }
}

void DebugDrawSystem::RemoveEntity(DAVA::Entity* entity)
{
    DAVA::FindAndRemoveExchangingWithLast(entities, entity);

    for (auto& it : entitiesComponentMap)
    {
        DAVA::FindAndRemoveExchangingWithLast(it.second, entity);
    }
}

void DebugDrawSystem::RegisterComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    const DAVA::Type* type = component->GetType();

    auto it = drawComponentFunctionsMap.find(type);

    if (it != drawComponentFunctionsMap.end())
    {
        DAVA::Vector<DAVA::Entity*>& array = entitiesComponentMap[type];

        auto it = find_if(array.begin(), array.end(), [entity](const DAVA::Entity* obj) { return entity == obj; });

        if (it == array.end())
        {
            array.push_back(entity);
        }
    }
}

void DebugDrawSystem::UnregisterComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    const DAVA::Type* type = component->GetType();

    auto it = entitiesComponentMap.find(type);

    if (it != entitiesComponentMap.end() && entity->GetComponentCount(type) == 1)
    {
        DAVA::FindAndRemoveExchangingWithLast(it->second, entity);
    }
}

void DebugDrawSystem::PrepareForRemove()
{
    entities.clear();
    entitiesComponentMap.clear();
}

void DebugDrawSystem::DrawComponent(const DAVA::Type* type, const DAVA::Function<void(DAVA::Entity*)>& func)
{
    auto it = entitiesComponentMap.find(type);

    if (it != entitiesComponentMap.end())
    {
        DAVA::Vector<DAVA::Entity*>& array = it->second;

        for (DAVA::Entity* entity : array)
        {
            func(entity);
        }
    }
}

void DebugDrawSystem::Draw()
{
    for (auto& it : drawComponentFunctionsMap)
    {
        DrawComponent(it.first, it.second);
    }

    for (DAVA::Entity* entity : entities)
    { //drawing methods do not use data from components
        DrawObjectBoxesByType(entity);
        DrawHangingObjects(entity);
        DrawSwitchesWithDifferentLods(entity);
        DrawDebugOctTree(entity);
    }

    //draw selected objects
    const SelectableGroup& selection = Selection::GetSelection();
    for (auto entity : selection.ObjectsOfType<DAVA::Entity>())
    {
        DrawLightNode(entity, true);
        DrawSelectedSoundNode(entity);
    }
}

void DebugDrawSystem::DrawObjectBoxesByType(DAVA::Entity* entity)
{
    bool drawBox = false;

    DAVA::KeyedArchive* customProperties = GetCustomPropertiesArchieve(entity);
    if (customProperties)
    {
        if (customProperties->IsKeyExists("CollisionType"))
        {
            drawBox = customProperties->GetInt32("CollisionType", 0) == objectType;
        }
        else if (objectType == ResourceEditor::ESOT_UNDEFINED_COLLISION && entity->GetParent() == GetScene())
        {
            const bool skip =
            GetLight(entity) == NULL &&
            GetCamera(entity) == NULL &&
            GetLandscape(entity) == NULL;

            drawBox = skip;
        }
    }

    if (drawBox)
    {
        DrawEntityBox(entity, objectTypeColor);
    }
}

void DebugDrawSystem::DrawDebugOctTree(DAVA::Entity* entity)
{
#if (DAVA_ALLOW_OCTREE_DEBUG)
    SceneEditor2* editorScene = static_cast<SceneEditor2*>(GetScene());
    DAVA::RenderHelper* drawer = editorScene->GetRenderSystem()->GetDebugDrawer();

    DAVA::RenderObject* renderObject = GetRenderObject(entity);
    if (renderObject == nullptr)
        return;

    for (DAVA::uint32 k = 0; k < renderObject->GetActiveRenderBatchCount(); ++k)
    {
        DAVA::RenderBatch* renderBatch = renderObject->GetActiveRenderBatch(k);

        DAVA::PolygonGroup* pg = renderBatch->GetPolygonGroup();
        if (pg == nullptr)
            continue;

        DAVA::GeometryOctTree* octTree = pg->GetGeometryOctTree();
        if (octTree == nullptr)
            continue;

        const DAVA::Matrix4& wt = entity->GetWorldTransform();
        if (renderBatch->debugDrawOctree)
            octTree->DebugDraw(wt, 0, drawer);

        for (const DAVA::GeometryOctTree::Triangle& tri : octTree->GetDebugTriangles())
        {
            DAVA::Vector3 v1 = tri.v1 * entity->GetWorldTransform();
            DAVA::Vector3 v2 = tri.v2 * entity->GetWorldTransform();
            DAVA::Vector3 v3 = tri.v3 * entity->GetWorldTransform();
            GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawLine(v1, v2, DAVA::Color(1.0f, 0.0f, 0.0f, 1.0f), DAVA::RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);
            GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawLine(v2, v3, DAVA::Color(1.0f, 0.0f, 0.0f, 1.0f), DAVA::RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);
            GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawLine(v3, v1, DAVA::Color(1.0f, 0.0f, 0.0f, 1.0f), DAVA::RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);
        }

        for (const DAVA::AABBox3& box : octTree->GetDebugBoxes())
        {
            GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawAABoxTransformed(box, wt, DAVA::Color(0.0f, 1.0f, 0.0f, 1.0f), DAVA::RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);
        }
    }
#endif
}

void DebugDrawSystem::DrawLightNode(DAVA::Entity* entity, bool isSelected)
{
    DAVA::Light* light = GetLight(entity);
    if (NULL != light)
    {
        SceneEditor2* editorScene = static_cast<SceneEditor2*>(GetScene());
        DAVA::RenderHelper* drawer = editorScene->GetRenderSystem()->GetDebugDrawer();

        DAVA::AABBox3 worldBox;
        DAVA::AABBox3 localBox = editorScene->collisionSystem->GetUntransformedBoundingBox(entity);
        DVASSERT(!localBox.IsEmpty());
        localBox.GetTransformedBox(entity->GetWorldTransform(), worldBox);

        if (light->GetType() == DAVA::Light::TYPE_DIRECTIONAL)
        {
            DAVA::Vector3 center = worldBox.GetCenter();
            DAVA::Vector3 direction = -light->GetDirection();
            direction.Normalize();
            direction = direction * worldBox.GetSize().x;
            center -= (direction / 2);
            drawer->DrawArrow(center + direction, center, direction.Length() / 2, DAVA::Color(1.0f, 1.0f, 0, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
        }
        else if (light->GetType() == DAVA::Light::TYPE_POINT)
        {
            DAVA::Vector3 worldCenter = worldBox.GetCenter();
            drawer->DrawIcosahedron(worldCenter, worldBox.GetSize().x / 2, DAVA::Color(1.0f, 1.0f, 0, 0.3f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
            drawer->DrawIcosahedron(worldCenter, worldBox.GetSize().x / 2, DAVA::Color(1.0f, 1.0f, 0, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
            DAVA::KeyedArchive* properties = GetCustomPropertiesArchieve(entity);
            DAVA::VariantType* value = properties->GetVariant("editor.staticlight.falloffcutoff");
            if (value != nullptr && value->GetType() == DAVA::VariantType::TYPE_FLOAT && isSelected)
            {
                DAVA::float32 distance = value->AsFloat();
                if (distance < BeastSystem::DEFAULT_FALLOFFCUTOFF_VALUE)
                {
                    DAVA::uint32 segmentCount = 32;
                    drawer->DrawCircle(worldCenter, DAVA::Vector3(1.0f, 0.0f, 0.0f), distance, segmentCount, DAVA::Color(1.0f, 1.0f, 0.0f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
                    drawer->DrawCircle(worldCenter, DAVA::Vector3(0.0f, 1.0f, 0.0f), distance, segmentCount, DAVA::Color(1.0f, 1.0f, 0.0f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
                    drawer->DrawCircle(worldCenter, DAVA::Vector3(0.0f, 0.0f, 1.0f), distance, segmentCount, DAVA::Color(1.0f, 1.0f, 0.0f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
                }
            }
        }
        else if (light->GetType() == DAVA::Light::TYPE_SPOT)
        {
            float coneAngle = DAVA::PI / 2.0f;
            float penumbraAngle = 0.0f;
            DAVA::KeyedArchive* properties = GetCustomPropertiesArchieve(entity);
            if (properties != nullptr)
            {
                DAVA::VariantType* value = properties->GetVariant("editor.staticlight.cone.angle");
                if (value->GetType() == DAVA::VariantType::TYPE_FLOAT)
                {
                    coneAngle = value->AsFloat() * DAVA::PI / 180.0f;
                }

                value = properties->GetVariant("editor.staticlight.cone.penumbra.angle");
                if (value->GetType() == DAVA::VariantType::TYPE_FLOAT)
                {
                    penumbraAngle = value->AsFloat() * DAVA::PI / 180.0f;
                }
            }

            DAVA::Vector3 center = worldBox.GetCenter();
            DAVA::Vector3 direction = light->GetDirection();
            direction.Normalize();

            center -= 0.5f * direction;
            drawer->DrawArrow(center, center + direction, 0.5f * direction.Length(), DAVA::Color(1.0f, 1.0f, 1.0f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);

            DAVA::float32 innerRadius = std::sin(0.5f * coneAngle);
            DAVA::float32 outerRadius = std::sin(0.5f * std::max(coneAngle, coneAngle + penumbraAngle));
            DAVA::uint32 sgm = 16;
            DAVA::float32 dt = 0.2f;
            DAVA::float32 t = dt;
            while (t <= 1.0f)
            {
                drawer->DrawCircle(center + t * direction, direction, t * innerRadius, sgm, DAVA::Color(1.0f, 1.0f, 0.5f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
                if (outerRadius > innerRadius)
                {
                    drawer->DrawCircle(center + t * direction, direction, t * outerRadius, sgm, DAVA::Color(1.0f, 1.0f, 0.0f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
                }
                t += dt;
            }
            drawer->DrawCircle(center + direction, direction, outerRadius, sgm, DAVA::Color(1.0f, 1.0f, 0.0f, 0.3f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
            drawer->DrawCircle(center + direction, direction, innerRadius, sgm, DAVA::Color(1.0f, 1.0f, 0.5f, 0.3f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
        }
        else
        {
            drawer->DrawAABox(worldBox, DAVA::Color(1.0f, 1.0f, 0, 0.3f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
            drawer->DrawAABox(worldBox, DAVA::Color(1.0f, 1.0f, 0, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
        }
    }
}

void DebugDrawSystem::DrawSoundNode(DAVA::Entity* entity)
{
    GlobalSceneSettings* settings = REGlobal::GetGlobalContext()->GetData<GlobalSceneSettings>();

    if (settings->drawSoundObjects == false)
        return;

    DAVA::SoundComponent* sc = GetSoundComponent(entity);
    if (sc)
    {
        SceneEditor2* editorScene = static_cast<SceneEditor2*>(GetScene());

        DAVA::AABBox3 worldBox;
        DAVA::AABBox3 localBox = editorScene->collisionSystem->GetUntransformedBoundingBox(entity);
        if (!localBox.IsEmpty())
        {
            localBox.GetTransformedBox(entity->GetWorldTransform(), worldBox);

            DAVA::Color soundColor = settings->soundObjectBoxColor;
            GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawAABox(worldBox, ClampToUnityRange(soundColor), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
        }
    }
}

void DebugDrawSystem::DrawSelectedSoundNode(DAVA::Entity* entity)
{
    GlobalSceneSettings* settings = REGlobal::GetGlobalContext()->GetData<GlobalSceneSettings>();
    if (settings->drawSoundObjects == false)
        return;

    DAVA::SoundComponent* sc = GetSoundComponent(entity);
    if (sc)
    {
        SceneEditor2* sceneEditor = ((SceneEditor2*)GetScene());

        DAVA::Vector3 position = entity->GetWorldTransform().GetTranslationVector();

        DAVA::uint32 fontHeight = 0;
        DAVA::GraphicFont* debugTextFont = sceneEditor->textDrawSystem->GetFont();
        if (debugTextFont)
            fontHeight = debugTextFont->GetFontHeight();

        DAVA::uint32 eventsCount = sc->GetEventsCount();
        for (DAVA::uint32 i = 0; i < eventsCount; ++i)
        {
            DAVA::SoundEvent* sEvent = sc->GetSoundEvent(i);
            DAVA::float32 distance = sEvent->GetMaxDistance();

            DAVA::Color soundColor = settings->soundObjectSphereColor;
            sceneEditor->GetRenderSystem()->GetDebugDrawer()->DrawIcosahedron(position, distance,
                                                                              ClampToUnityRange(soundColor), DAVA::RenderHelper::DRAW_SOLID_DEPTH);

            sceneEditor->textDrawSystem->DrawText(sceneEditor->textDrawSystem->ToPos2d(position) - DAVA::Vector2(0.f, fontHeight - 2.f) * i,
                                                  sEvent->GetEventName(), DAVA::Color::White, TextDrawSystem::Align::Center);

            if (sEvent->IsDirectional())
            {
                sceneEditor->GetRenderSystem()->GetDebugDrawer()->DrawArrow(position, sc->GetLocalDirection(i), .25f,
                                                                            DAVA::Color(0.0f, 1.0f, 0.3f, 1.0f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
            }
        }
    }
}

void DebugDrawSystem::DrawWindNode(DAVA::Entity* entity)
{
    DAVA::WindComponent* wind = GetWindComponent(entity);
    if (wind)
    {
        const DAVA::Matrix4& worldMx = entity->GetWorldTransform();
        DAVA::Vector3 worldPosition = worldMx.GetTranslationVector();

        GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawArrow(worldPosition, worldPosition + wind->GetDirection() * 3.f, .75f,
                                                                   DAVA::Color(1.0f, 0.5f, 0.2f, 1.0f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
    }
}

void DebugDrawSystem::DrawEntityBox(DAVA::Entity* entity, const DAVA::Color& color)
{
    SceneEditor2* editorScene = static_cast<SceneEditor2*>(GetScene());
    DAVA::AABBox3 localBox = editorScene->collisionSystem->GetUntransformedBoundingBox(entity);
    if (localBox.IsEmpty() == false)
    {
        DAVA::AABBox3 worldBox;
        localBox.GetTransformedBox(entity->GetWorldTransform(), worldBox);
        GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawAABox(worldBox, color, DAVA::RenderHelper::DRAW_WIRE_DEPTH);
    }
}

void DebugDrawSystem::DrawHangingObjects(DAVA::Entity* entity)
{
    if (hangingObjectsModeEnabled && (entity->GetParent() == GetScene()) && IsObjectHanging(entity))
    {
        DrawEntityBox(entity, DAVA::Color(1.0f, 0.0f, 0.0f, 1.0f));
    }
}

void DebugDrawSystem::CollectRenderBatchesRecursively(DAVA::Entity* entity, RenderBatchesWithTransforms& batches) const
{
    auto ro = GetRenderObject(entity);
    if (ro != nullptr)
    {
        auto roType = ro->GetType();
        if ((roType == DAVA::RenderObject::TYPE_MESH) || (roType == DAVA::RenderObject::TYPE_RENDEROBJECT) || (roType == DAVA::RenderObject::TYPE_SPEED_TREE))
        {
            const DAVA::Matrix4& wt = entity->GetWorldTransform();
            for (DAVA::uint32 i = 0, e = ro->GetActiveRenderBatchCount(); i < e; ++i)
            {
                DAVA::RenderBatch* batch = ro->GetActiveRenderBatch(i);
                if (batch != nullptr)
                {
                    DAVA::PolygonGroup* pg = batch->GetPolygonGroup();
                    if (pg != nullptr)
                    {
                        batches.emplace_back(batch, wt);
                    }
                }
            }
        }
    }

    for (DAVA::int32 i = 0, e = entity->GetChildrenCount(); i < e; ++i)
    {
        CollectRenderBatchesRecursively(entity->GetChild(i), batches);
    }
}

DAVA::float32 DebugDrawSystem::GetMinimalZ(const RenderBatchesWithTransforms& batches) const
{
    DAVA::float32 minZ = AABBOX_INFINITY;
    for (const auto& batch : batches)
    {
        DAVA::PolygonGroup* polygonGroup = batch.first->GetPolygonGroup();
        for (DAVA::uint32 v = 0, e = polygonGroup->GetVertexCount(); v < e; ++v)
        {
            DAVA::Vector3 pos;
            polygonGroup->GetCoord(v, pos);
            minZ = DAVA::Min(minZ, pos.z);
        }
    }
    return minZ;
}

void DebugDrawSystem::GetLowestVertexes(const RenderBatchesWithTransforms& batches, DAVA::Vector<DAVA::Vector3>& vertexes) const
{
    const DAVA::float32 minZ = GetMinimalZ(batches);
    for (const auto& batch : batches)
    {
        DAVA::float32 scale = std::sqrt(batch.second._20 * batch.second._20 + batch.second._21 * batch.second._21 + batch.second._22 * batch.second._22);
        DAVA::PolygonGroup* polygonGroup = batch.first->GetPolygonGroup();
        for (DAVA::uint32 v = 0, e = polygonGroup->GetVertexCount(); v < e; ++v)
        {
            DAVA::Vector3 pos;
            polygonGroup->GetCoord(v, pos);
            if (scale * (pos.z - minZ) <= hangingObjectsHeight)
            {
                vertexes.push_back(pos * batch.second);
            }
        }
    }
}

bool DebugDrawSystem::IsObjectHanging(DAVA::Entity* entity) const
{
    DAVA::Vector<DAVA::Vector3> lowestVertexes;
    RenderBatchesWithTransforms batches;
    CollectRenderBatchesRecursively(entity, batches);
    GetLowestVertexes(batches, lowestVertexes);

    for (const auto& vertex : lowestVertexes)
    {
        DAVA::Vector3 landscapePoint = GetLandscapePointAtCoordinates(DAVA::Vector2(vertex.x, vertex.y));
        if ((vertex.z - landscapePoint.z) > DAVA::EPSILON)
        {
            return true;
        }
    }

    return false;
}

DAVA::Vector3 DebugDrawSystem::GetLandscapePointAtCoordinates(const DAVA::Vector2& centerXY) const
{
    LandscapeEditorDrawSystem* landSystem = ((SceneEditor2*)GetScene())->landscapeEditorDrawSystem;
    LandscapeProxy* landscape = landSystem->GetLandscapeProxy();

    if (landscape)
    {
        return landscape->PlacePoint(DAVA::Vector3(centerXY));
    }

    return DAVA::Vector3();
}

void DebugDrawSystem::DrawSwitchesWithDifferentLods(DAVA::Entity* entity)
{
    if (switchesWithDifferentLodsEnabled && SceneValidator::IsEntityHasDifferentLODsCount(entity))
    {
        SceneEditor2* editorScene = static_cast<SceneEditor2*>(GetScene());

        DAVA::AABBox3 worldBox;
        DAVA::AABBox3 localBox = editorScene->collisionSystem->GetUntransformedBoundingBox(entity);
        DVASSERT(!localBox.IsEmpty());
        localBox.GetTransformedBox(entity->GetWorldTransform(), worldBox);
        GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawAABox(worldBox, DAVA::Color(1.0f, 0.f, 0.f, 1.f), DAVA::RenderHelper::DRAW_WIRE_DEPTH);
    }
}

void DebugDrawSystem::DrawDecals(DAVA::Entity* entity)
{
    DAVA::RenderHelper* drawer = GetScene()->GetRenderSystem()->GetDebugDrawer();
    DAVA::uint32 componentsCount = entity->GetComponentCount<DAVA::GeoDecalComponent>();
    for (DAVA::uint32 i = 0; i < componentsCount; ++i)
    {
        DAVA::GeoDecalComponent* decal = entity->GetComponent<DAVA::GeoDecalComponent>(i);

        DVASSERT(decal != nullptr);

        DAVA::Matrix4 transform = entity->GetWorldTransform();

        DAVA::RenderHelper::eDrawType dt = DAVA::RenderHelper::eDrawType::DRAW_WIRE_DEPTH;
        DAVA::Color baseColor(1.0f, 0.5f, 0.25f, 1.0f);
        DAVA::Color accentColor(1.0f, 1.0f, 0.5f, 1.0f);

        DAVA::AABBox3 box = decal->GetBoundingBox();
        DAVA::Vector3 boxCenter = box.GetCenter();
        DAVA::Vector3 boxHalfSize = 0.5f * box.GetSize();

        DAVA::Vector3 farPoint = DAVA::Vector3(boxCenter.x, boxCenter.y, box.min.z) * transform;
        DAVA::Vector3 nearPoint = DAVA::Vector3(boxCenter.x, boxCenter.y, box.max.z) * transform;

        DAVA::Vector3 direction = farPoint - nearPoint;
        direction.Normalize();

        drawer->DrawAABoxTransformed(box, transform, baseColor, dt);

        if (decal->GetConfig().mapping == DAVA::GeoDecalManager::Mapping::CYLINDRICAL)
        {
            DAVA::Vector3 side = DAVA::Vector3(boxCenter.x - boxHalfSize.x, 0.0f, box.max.z) * transform;

            float radius = (side - nearPoint).Length();
            drawer->DrawCircle(nearPoint, direction, radius, 32, accentColor, dt);
            drawer->DrawCircle(farPoint, -direction, radius, 32, accentColor, dt);
            drawer->DrawLine(nearPoint, side, accentColor);
        }
        else if (decal->GetConfig().mapping == DAVA::GeoDecalManager::Mapping::SPHERICAL)
        {
            // no extra debug visualization
        }
        else /* planar assumed */
        {
            drawer->DrawArrow(nearPoint - direction, nearPoint, 0.25f * direction.Length(), accentColor, dt);
        }
    }
}
