#include "Classes/UserNodeModule/Private/UserNodeSystem.h"
#include "Classes/Qt/Scene/SceneEditor2.h"

#include "Classes/Qt/Scene/SceneEditor2.h"

#include <FileSystem/KeyedArchive.h>
#include <FileSystem/FilePath.h>
#include <Debug/DVAssert.h>
#include <Logger/Logger.h>
#include <Math/MathConstants.h>
#include <Render/Material/NMaterialNames.h>
#include <Render/Material/NMaterial.h>
#include <Render/Highlevel/RenderObject.h>
#include <Render/Highlevel/RenderSystem.h>
#include <Scene3D/Components/TransformComponent.h>
#include <Scene3D/Components/SingleComponents/TransformSingleComponent.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/SceneFileV2.h>
#include <Scene3D/Scene.h>
#include <Utils/Utils.h>

namespace UserNodeSystemDetails
{
DAVA::RenderObject* CreateRenderObject(const DAVA::FilePath& scenePath)
{
    using namespace DAVA;

    ScopedPtr<Scene> scene(new Scene());
    SceneFileV2::eError result = scene->LoadScene(scenePath);
    if (result == SceneFileV2::ERROR_NO_ERROR)
    {
        Vector<Entity*> entities;
        scene->GetChildEntitiesWithComponent(entities, DAVA::Type::Instance<DAVA::RenderComponent>());
        if (entities.size() == 1)
        {
            RenderObject* ro = GetRenderObject(entities[0]);
            if (ro != nullptr)
            {
                uint32 count = ro->GetRenderBatchCount();
                for (uint32 i = 0; i < count; ++i)
                {
                    NMaterial* mat = ro->GetRenderBatch(i)->GetMaterial();
                    if (mat != nullptr)
                    {
                        if (mat->HasLocalFlag(NMaterialFlagName::FLAG_FLATCOLOR) == false)
                        {
                            mat->AddFlag(NMaterialFlagName::FLAG_FLATCOLOR, 1);
                        }

                        if (mat->HasLocalProperty(NMaterialParamName::PARAM_FLAT_COLOR) == false)
                        {
                            mat->AddProperty(NMaterialParamName::PARAM_FLAT_COLOR, Color().color, rhi::ShaderProp::TYPE_FLOAT4);
                        }
                    }
                }
            }

            return SafeRetain(ro);
        }
    }

    Logger::Error("[%s] Can't open scene %s properly", __FUNCTION__, scenePath.GetStringValue().c_str());
    return nullptr;
}
}

UserNodeSystem::UserNodeSystem(DAVA::Scene* scene, const DAVA::FilePath& scenePath)
    : SceneSystem(scene)
{
    sourceObject = UserNodeSystemDetails::CreateRenderObject(scenePath);
    if (sourceObject != nullptr)
    {
        using namespace DAVA;

        Vector3 size = sourceObject->GetBoundingBox().GetSize();
        nodeMatrix = Matrix4::MakeRotation(Vector3::UnitZ, DAVA::PI) * Matrix4::MakeScale(Vector3(6.f / size.x, 13.f / size.y, 6.f / size.z));
    }
}

UserNodeSystem::~UserNodeSystem()
{
    SafeRelease(sourceObject);

    DVASSERT(userNodes.empty());
    DVASSERT(spawnNodes.empty());
}

void UserNodeSystem::AddEntity(DAVA::Entity* entity)
{
    userNodes.push_back(entity);
}

void UserNodeSystem::RemoveEntity(DAVA::Entity* entity)
{
    auto it = spawnNodes.find(entity);
    if (it != spawnNodes.end())
    {
        RemoveObject(it->second.ro);
        spawnNodes.erase(it);
    }

    DAVA::FindAndRemoveExchangingWithLast(userNodes, entity);
}

void UserNodeSystem::PrepareForRemove()
{
    for (const auto& node : spawnNodes)
    {
        RemoveObject(node.second.ro);
    }
    spawnNodes.clear();
    userNodes.clear();
}

void UserNodeSystem::Process(DAVA::float32 timeElapsed)
{
    using namespace DAVA;

    if (userNodes.empty() == true || sourceObject == nullptr)
        return;

    //remove old spawns: somebody changed type value
    RemoveOldSpawns();

    //update transform of entity
    UpdateTransformedEntities();

    //find new spawns: somebody changed type
    Vector<Entity*> newSpawns;
    FindNewSpawns(newSpawns);

    SceneEditor2* editorScene = static_cast<SceneEditor2*>(GetScene());
    RenderSystem* renderSystem = editorScene->GetRenderSystem();
    for (Entity* entity : newSpawns)
    {
        //add RO
        NodeDescription description;
        description.ro = sourceObject->Clone(nullptr);
        spawnNodes[entity] = description;

        TransformObject(&spawnNodes[entity], *GetWorldTransformPtr(entity));

        renderSystem->RenderPermanent(description.ro);
    }

    UpdateSpawnVisibility();
}

void UserNodeSystem::Draw()
{
    using namespace DAVA;

    SceneEditor2* editorScene = static_cast<SceneEditor2*>(GetScene());
    RenderHelper* drawer = editorScene->GetRenderSystem()->GetDebugDrawer();

    for (Entity* entity : userNodes)
    {
        bool isBot = IsSpawnNode(entity);
        if ((isBot == false || IsSystemEnabled() == false) && entity->GetVisible())
        {
            AABBox3 worldBox = editorScene->collisionSystem->GetUntransformedBoundingBox(entity);
            DVASSERT(!worldBox.IsEmpty());

            const Matrix4& worldTransform = entity->GetWorldTransform();
            drawer->DrawAABoxTransformed(worldBox, worldTransform, Color(0.5f, 0.5f, 1.0f, 0.3f), RenderHelper::DRAW_SOLID_DEPTH);
            drawer->DrawAABoxTransformed(worldBox, worldTransform, Color(0.2f, 0.2f, 0.8f, 1.0f), RenderHelper::DRAW_WIRE_DEPTH);

            float32 delta = worldBox.GetSize().Length() / 4;
            const Vector3 center = worldTransform.GetTranslationVector();
            const Vector3 xAxis = MultiplyVectorMat3x3(Vector3(delta, 0.f, 0.f), worldTransform);
            const Vector3 yAxis = MultiplyVectorMat3x3(Vector3(0.f, delta, 0.f), worldTransform);
            const Vector3 zAxis = MultiplyVectorMat3x3(Vector3(0.f, 0.f, delta), worldTransform);

            // axises
            drawer->DrawLine(center, center + xAxis, Color(0.7f, 0, 0, 1.0f));
            drawer->DrawLine(center, center + yAxis, Color(0, 0.7f, 0, 1.0f));
            drawer->DrawLine(center, center + zAxis, Color(0, 0, 0.7f, 1.0f));
        }
    }
}

bool UserNodeSystem::IsSpawnNode(DAVA::Entity* entity) const
{
    using namespace DAVA;

    KeyedArchive* options = GetCustomPropertiesArchieve(entity);
    if (options != nullptr && options->IsKeyExists("type"))
    {
        String type = options->GetString("type");
        return type == "spawnpoint" || (type == "botspawn");
    }

    return false;
}

const DAVA::Color& UserNodeSystem::GetSpawnColor(DAVA::Entity* entity) const
{
    using namespace DAVA;

    KeyedArchive* options = GetCustomPropertiesArchieve(entity);
    if (options != nullptr && options->IsKeyExists("SpawnPreferredVehicleType"))
    {
        int32 type = options->GetInt32("SpawnPreferredVehicleType");
        switch (type)
        {
        case 1: //light tank
            return Color::Yellow;
        case 2: //medium tank
            return Color::Blue;
        case 3: //heavy tank
            return Color::Red;
        case 4: //at-spg
            return Color::Green;

        default: //any
            return Color::White;
        }
    }

    return Color::White;
}

DAVA::Matrix4* UserNodeSystem::GetWorldTransformPtr(DAVA::Entity* entity) const
{
    using namespace DAVA;
    return entity->GetComponent<TransformComponent>()->GetWorldTransformPtr();
}

void UserNodeSystem::RemoveOldSpawns()
{
    using namespace DAVA;

    RenderSystem* renderSystem = GetScene()->GetRenderSystem();
    for (auto it = spawnNodes.begin(); it != spawnNodes.end();)
    {
        bool isBot = IsSpawnNode(it->first);
        if (isBot == false)
        {
            RemoveObject(it->second.ro);
            it = spawnNodes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void UserNodeSystem::FindNewSpawns(DAVA::Vector<DAVA::Entity*>& newSpawns)
{
    for (DAVA::Entity* e : userNodes)
    {
        bool isBot = IsSpawnNode(e);
        if (isBot == true)
        {
            if (spawnNodes.count(e) == 0)
            {
                newSpawns.push_back(e);
            }
        }
    }
}

void UserNodeSystem::UpdateSpawnVisibility()
{
    using namespace DAVA;
    for (auto it = spawnNodes.begin(); it != spawnNodes.end(); ++it)
    {
        RenderObject* ro = it->second.ro;
        if (it->first->GetVisible() && IsSystemEnabled() && isSystemVisible)
        {
            const Color& color = GetSpawnColor(it->first);

            ro->AddFlag(RenderObject::VISIBLE);

            uint32 count = ro->GetRenderBatchCount();
            for (uint32 i = 0; i < count; ++i)
            {
                NMaterial* mat = ro->GetRenderBatch(i)->GetMaterial();
                if (mat != nullptr)
                {
                    mat->SetPropertyValue(NMaterialParamName::PARAM_FLAT_COLOR, color.color);
                }
            }
        }
        else
        {
            ro->RemoveFlag(RenderObject::VISIBLE);
        }
    }
}

void UserNodeSystem::UpdateTransformedEntities()
{
    using namespace DAVA;

    TransformSingleComponent* trSingle = GetScene()->transformSingleComponent;

    if (trSingle != nullptr)
    {
        RenderSystem* renderSystem = GetScene()->GetRenderSystem();
        for (auto& pair : trSingle->worldTransformChanged.map)
        {
            if (pair.first->GetComponentsCount(Type::Instance<UserComponent>()) > 0)
            {
                for (Entity* entity : pair.second)
                {
                    auto it = spawnNodes.find(entity);
                    if (it != spawnNodes.end())
                    {
                        TransformObject(&it->second, *GetWorldTransformPtr(entity));
                    }
                }
            }
        }
    }
}

void UserNodeSystem::TransformObject(UserNodeSystem::NodeDescription* description, const DAVA::Matrix4& entityTransform)
{
    description->transform = nodeMatrix * entityTransform;
    description->ro->SetWorldTransformPtr(&description->transform);
    GetScene()->GetRenderSystem()->MarkForUpdate(description->ro);
}

void UserNodeSystem::RemoveObject(DAVA::RenderObject* renderObject)
{
    GetScene()->GetRenderSystem()->RemoveFromRender(renderObject);
    DAVA::SafeRelease(renderObject);
}
void UserNodeSystem::SetVisible(bool visible)
{
    isSystemVisible = visible;
}
