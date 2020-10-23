#include "Classes/SceneManager/SceneData.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Commands2/SetFieldValueCommand.h"
#include "Classes/Commands2/Base/RECommandNotificationObject.h"
#include "Classes/Commands2/TransformCommand.h"
#include "Classes/Commands2/ParticleEditorCommands.h"
#include "Classes/Commands2/ConvertToBillboardCommand.h"
#include "Classes/Commands2/CreatePlaneLODCommand.h"
#include "Classes/Commands2/DeleteLODCommand.h"
#include "Classes/Commands2/EntityParentChangeCommand.h"
#include "Classes/Commands2/InspMemberModifyCommand.h"
#include "Classes/Commands2/BakeTransformCommand.h"

#include "Classes/Selection/Selection.h"

#include "Classes/Qt/Scene/System/CollisionSystem.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/System/CameraSystem.h"

#include <Physics/PhysicsGeometryCache.h>
#include <Physics/PhysicsModule.h>
#include <Physics/Private/PhysicsMath.h>

#include <Base/AlignedAllocator.h>
#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <Functional/Function.h>
#include <Math/AABBox3.h>
#include <Math/Matrix4.h>
#include <ModuleManager/ModuleManager.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/TransformComponent.h>
#include <Scene3D/Components/LightComponent.h>
#include <Scene3D/Components/SoundComponent.h>
#include <Scene3D/Components/TextComponent.h>
#include <Scene3D/Components/UserComponent.h>
#include <Scene3D/Components/WindComponent.h>
#include <Scene3D/Components/SingleComponents/TransformSingleComponent.h>

#include <Scene3D/Scene.h>

#include <physx/PxScene.h>
#include <physx/PxQueryReport.h>

#define SIMPLE_COLLISION_BOX_SIZE 1.0f

namespace SceneCollisionSystemDetail
{
using namespace physx;
using namespace DAVA;

const PxFilterData landscapeFilterData = PxFilterData(1, 0, 0, 0);
const PxFilterData objectFilterData = PxFilterData(0, 1, 0, 0);
const PxU32 maxQueryTouchCount = 1024;
const PxU32 maxOverlapTouchCount = 30000;

void UpdateActorTransform(const Matrix4& tranform, PxRigidActor* actor)
{
    Vector3 position;
    Vector3 scale;
    Quaternion rotation;
    tranform.Decomposition(position, scale, rotation);

    actor->setGlobalPose(PxTransform(PhysicsMath::Vector3ToPxVec3(position), PhysicsMath::QuaternionToPxQuat(rotation)));
    DVASSERT(actor->getNbShapes() == 1);
    PxShape* shape = nullptr;
    actor->getShapes(&shape, 1, 0);
    if (shape->getGeometryType() == PxGeometryType::eTRIANGLEMESH)
    {
        PxTriangleMeshGeometry geom;
        shape->getTriangleMeshGeometry(geom);
        geom.scale.scale = PhysicsMath::Vector3ToPxVec3(scale);
        shape->setGeometry(geom);
    }
}

void InitBounds(PxShape* shape, AABBox3 box)
{
    DVASSERT(shape->userData == nullptr);
    float32 inflateHalfSize = 0.01f;
    box.min = box.min - inflateHalfSize;
    box.max = box.max + inflateHalfSize;

    shape->userData = new AABBox3(box);
}

void InitBounds(PxRigidActor* actor, PxShape* shape)
{
    PxBounds3 bounds = actor->getWorldBounds();
    InitBounds(shape, AABBox3(PhysicsMath::PxBounds3ToAABox3(bounds)));
}

struct CollisionObj
{
    PxRigidActor* collisionObject = nullptr;
    bool isValid = false;
    bool shouldRecreate = false;
};

CollisionObj CreateBox(bool createCollision, bool recreate, const Matrix4& transform, const Vector3& halfSize, void* userData)
{
    CollisionObj result;
    result.isValid = true;
    result.shouldRecreate = recreate;
    if (createCollision)
    {
        PhysicsModule* module = GetEngineContext()->moduleManager->GetModule<PhysicsModule>();
        PxActor* actor = module->CreateStaticActor();
        DVASSERT(actor != nullptr);
        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        DVASSERT(rigidActor != nullptr);
        rigidActor->userData = userData;

        PxShape* shape = module->CreateBoxShape(halfSize, DAVA::FastName());
        shape->setQueryFilterData(objectFilterData);
        rigidActor->attachShape(*shape);

        InitBounds(rigidActor, shape);
        UpdateActorTransform(transform, rigidActor);

        result.collisionObject = rigidActor;
    }
    return result;
}

CollisionObj CreateMesh(bool createCollision, const Matrix4& transform, RenderObject* ro, PhysicsGeometryCache* cache, void* userData)
{
    CollisionObj result;
    result.isValid = true;
    if (createCollision)
    {
        int32 maxVertexCount = 0;
        int32 bestLodIndex = 0;
        int32 curSwitchIndex = ro->GetSwitchIndex();
        Vector<PolygonGroup*> polygons;
        polygons.reserve(8);

        // search for best lod index
        for (uint32 i = 0; i < ro->GetRenderBatchCount(); ++i)
        {
            int32 batchLodIndex = 0;
            int32 batchSwitchIndex = 0;
            RenderBatch* batch = ro->GetRenderBatch(i, batchLodIndex, batchSwitchIndex);
            int32 vertexCount = (batch->GetPolygonGroup() != nullptr) ? batch->GetPolygonGroup()->GetVertexCount() : 0;
            if ((vertexCount > maxVertexCount) && (curSwitchIndex == batchSwitchIndex))
            {
                bestLodIndex = batchLodIndex;
                maxVertexCount = vertexCount;
            }
        }

        for (uint32 i = 0; i < ro->GetRenderBatchCount(); ++i)
        {
            int32 batchLodIndex = 0;
            int32 batchSwitchIndex = 0;
            RenderBatch* batch = ro->GetRenderBatch(i, batchLodIndex, batchSwitchIndex);

            if ((batchLodIndex == bestLodIndex) && (batchSwitchIndex == curSwitchIndex))
            {
                polygons.push_back(batch->GetPolygonGroup());
            }
        }

        if (polygons.empty() == false)
        {
            PhysicsModule* module = GetEngineContext()->moduleManager->GetModule<PhysicsModule>();
            PxActor* actor = module->CreateStaticActor();
            DVASSERT(actor != nullptr);
            PxRigidActor* rigidActor = actor->is<PxRigidActor>();
            DVASSERT(rigidActor);
            rigidActor->userData = userData;

            PxShape* shape = module->CreateMeshShape(std::move(polygons), Vector3(1.0, 1.0, 1.0), DAVA::FastName(), cache);
            shape->setQueryFilterData(objectFilterData);
            rigidActor->attachShape(*shape);

            AABBox3 bbox;
            for (PolygonGroup* polygon : polygons)
            {
                bbox.AddAABBox(polygon->GetBoundingBox());
            }

            InitBounds(shape, bbox);
            UpdateActorTransform(transform, rigidActor);

            result.collisionObject = rigidActor;
        }
        else
        {
            result.isValid = false;
        }
    }
    return result;
}

CollisionObj CreateLandscape(bool createCollision, Landscape* landscape, void* userData)
{
    CollisionObj result;
    result.isValid = true;
    result.shouldRecreate = true;
    if (landscape->GetHeightmap() != nullptr)
    {
        if (createCollision)
        {
            PhysicsModule* module = GetEngineContext()->moduleManager->GetModule<PhysicsModule>();
            PxActor* actor = module->CreateStaticActor();
            DVASSERT(actor != nullptr);
            PxRigidActor* rigidActor = actor->is<PxRigidActor>();
            DVASSERT(rigidActor);
            rigidActor->userData = userData;

            Matrix4 localPose;
            PxShape* shape = module->CreateHeightField(landscape, DAVA::FastName(), localPose);
            rigidActor->attachShape(*shape);

            shape->setLocalPose(PxTransform(PhysicsMath::Matrix4ToPxMat44(localPose)));
            InitBounds(rigidActor, shape);
            shape->setQueryFilterData(landscapeFilterData);

            result.collisionObject = rigidActor;
        }
    }
    else
    {
        result = CreateBox(createCollision, true, DAVA::Matrix4::IDENTITY, DAVA::Vector3(0.5f, 0.5f, 0.5f), userData);
    }
    return result;
}

AABBox3* GetBounds(PxShape* shape)
{
    DVASSERT(shape->userData != nullptr);
    return reinterpret_cast<AABBox3*>(shape->userData);
}

AABBox3* GetBounds(PxRigidActor* actor)
{
    DVASSERT(actor != nullptr);
    DVASSERT(actor->getNbShapes() == 1);

    PxShape* shape = nullptr;
    actor->getShapes(&shape, 1, 0);
    return GetBounds(shape);
}

enum class ClassifyPlaneResult
{
    InFront,
    Intersects,
    Behind
};

enum class ClassifyPlanesResult
{
    ContainsOrIntersects,
    Outside
};

inline Plane TransformPlaneToLocalSpace(const Selectable& object, const Plane& plane)
{
    Matrix4 transform = object.GetWorldTransform();
    transform.Transpose();
    return Plane(Vector4(plane.n.x, plane.n.y, plane.n.z, plane.d) * transform);
}

inline ClassifyPlaneResult ClassifyBoundingBoxToPlane(const AABBox3& bbox, const Plane& plane)
{
    uint8 cornersData[8 * sizeof(Vector3)];
    Vector3* corners = reinterpret_cast<Vector3*>(cornersData);
    bbox.GetCorners(corners);

    float32 minDistance = std::numeric_limits<float>::max();
    float32 maxDistance = -minDistance;
    for (uint32 i = 0; i < 8; ++i)
    {
        float32 d = plane.DistanceToPoint(corners[i]);
        minDistance = std::min(minDistance, d);
        maxDistance = std::max(maxDistance, d);
    }

    if ((minDistance > 0.0f) && (maxDistance > 0.0f))
        return ClassifyPlaneResult::InFront;

    if ((minDistance < 0.0f) && (maxDistance < 0.0f))
        return ClassifyPlaneResult::Behind;

    return ClassifyPlaneResult::Intersects;
}

ClassifyPlanesResult ClassifyObjectBoundingBox(const Selectable& object, const AABBox3& bounds, const Vector<Plane>& planes)
{
    for (const Plane& globalPlane : planes)
    {
        Plane localPlane = TransformPlaneToLocalSpace(object, globalPlane);
        if (ClassifyBoundingBoxToPlane(bounds, localPlane) == ClassifyPlaneResult::Behind)
        {
            return ClassifyPlanesResult::Outside;
        }
    }
    return ClassifyPlanesResult::ContainsOrIntersects;
}

ClassifyPlanesResult ClassifyBoxToPlanes(const Selectable& object, PxShape* shape, const Vector<Plane>& planes)
{
    AABBox3 bounds = *GetBounds(shape);
    return ClassifyObjectBoundingBox(object, bounds, planes);
}

inline bool IsBothNegative(float32 v1, float32 v2)
{
    return (((reinterpret_cast<uint32_t&>(v1) & 0x80000000) & (reinterpret_cast<uint32_t&>(v2) & 0x80000000)) >> 31) != 0;
}

inline void SortDistances(float32 values[3])
{
    if (values[1] > values[0])
        std::swap(values[1], values[0]);
    if (values[2] > values[1])
        std::swap(values[2], values[1]);
    if (values[1] > values[0])
        std::swap(values[1], values[0]);
}

ClassifyPlanesResult ClassifyMeshToPlanes(const Selectable& object, PxShape* shape, const Vector<Plane>& planes)
{
    AABBox3 bounds = *GetBounds(shape);
    if (ClassifyObjectBoundingBox(object, bounds, planes) == ClassifyPlanesResult::Outside)
    {
        return ClassifyPlanesResult::Outside;
    }

    PxTriangleMeshGeometry geomHolder;
    bool extractSuccessed = shape->getTriangleMeshGeometry(geomHolder);
    DVASSERT(extractSuccessed == true);

    const PxTriangleMesh* mesh = geomHolder.triangleMesh;
    const PxU32 triangleCount = mesh->getNbTriangles();
    const PxVec3* verticesPtr = mesh->getVertices();
    const void* indicesPtr = mesh->getTriangles();

    const bool indices16Bits = mesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES;

    for (PxU32 i = 0; i < triangleCount; ++i)
    {
        PxU32 i0 = indices16Bits ? static_cast<const PxU16*>(indicesPtr)[3 * i] : static_cast<const PxU32*>(indicesPtr)[3 * i];
        PxU32 i1 = indices16Bits ? static_cast<const PxU16*>(indicesPtr)[3 * i + 1] : static_cast<const PxU32*>(indicesPtr)[3 * i + 1];
        PxU32 i2 = indices16Bits ? static_cast<const PxU16*>(indicesPtr)[3 * i + 2] : static_cast<const PxU32*>(indicesPtr)[3 * i + 2];

        PxVec3 v0 = verticesPtr[i0];
        PxVec3 v1 = verticesPtr[i1];
        PxVec3 v2 = verticesPtr[i2];

        bool isOutSideFound = false;
        for (const Plane& globalPlane : planes)
        {
            Plane localPlane = TransformPlaneToLocalSpace(object, globalPlane);

            float32 distances[3] = {
                localPlane.DistanceToPoint(v0.x, v0.y, v0.z),
                localPlane.DistanceToPoint(v1.x, v1.y, v1.z),
                localPlane.DistanceToPoint(v2.x, v2.y, v2.z)
            };

            SortDistances(distances);
            if (IsBothNegative(distances[0], distances[2]) == true)
            {
                isOutSideFound = true;
                break;
            }
        }

        if (isOutSideFound == false)
        {
            return ClassifyPlanesResult::ContainsOrIntersects;
        }
    }

    return ClassifyPlanesResult::Outside;
}

ClassifyPlanesResult ClassifyToPlanes(const Selectable& object, PxShape* shape, const Vector<Plane>& planes)
{
    PxGeometryType::Enum type = shape->getGeometryType();
    switch (type)
    {
    case PxGeometryType::eBOX:
        return ClassifyBoxToPlanes(object, shape, planes);
    case PxGeometryType::eTRIANGLEMESH:
        return ClassifyMeshToPlanes(object, shape, planes);
    case PxGeometryType::eHEIGHTFIELD:
        return ClassifyPlanesResult::Outside;
    default:
        DVASSERT(false);
        break;
    }

    return ClassifyPlanesResult::Outside;
}
} // namespace SceneCollisionSystemDetail

SceneCollisionSystem::SceneCollisionSystem(DAVA::Scene* scene)
    : DAVA::SceneSystem(scene)
{
    scene->GetEventSystem()->RegisterSystemForEvent(this, DAVA::EventSystem::SWITCH_CHANGED);
    scene->GetEventSystem()->RegisterSystemForEvent(this, DAVA::EventSystem::GEO_DECAL_CHANGED);
    DAVA::PhysicsModule* module = DAVA::GetEngineContext()->moduleManager->GetModule<DAVA::PhysicsModule>();
    DVASSERT(module != nullptr);

    DAVA::PhysicsSceneConfig config;
    config.threadCount = 0;
    physicsScene = module->CreateScene(config, physx::PxDefaultSimulationFilterShader, nullptr);
    geometryCache = new DAVA::PhysicsGeometryCache();
}

SceneCollisionSystem::~SceneCollisionSystem()
{
}

void SceneCollisionSystem::ObjectsRayTest(const DAVA::Vector3& from, const DAVA::Vector3& to, SelectableGroup::CollectionType& result) const
{
    using namespace physx;

    DAVA::Vector3 direction = to - from;
    DAVA::float32 distance = direction.Length();
    direction.Normalize();

    PxRaycastHit hit[SceneCollisionSystemDetail::maxQueryTouchCount];
    PxRaycastBuffer buffer(hit, SceneCollisionSystemDetail::maxQueryTouchCount);

    PxQueryFilterData filterData;
    filterData.data = SceneCollisionSystemDetail::objectFilterData;
    filterData.flags = PxQueryFlag::eSTATIC;
    DAVA::Map<physx::PxReal, physx::PxRigidActor*> sortedHits;
    if (physicsScene->raycast(DAVA::PhysicsMath::Vector3ToPxVec3(from), DAVA::PhysicsMath::Vector3ToPxVec3(direction), distance, buffer, PxHitFlag::eDEFAULT, filterData) == true)
    {
        result.reserve(buffer.nbTouches);
        for (PxU32 i = 0; i < buffer.nbTouches; i++)
        {
            PxRaycastHit& currentHit = hit[i];
            DVASSERT(currentHit.actor != nullptr);
            DVASSERT(currentHit.flags.isSet(PxHitFlag::eDISTANCE));
            sortedHits.emplace(currentHit.distance, currentHit.actor);
        }
    }

    GlobalSceneSettings* settings = REGlobal::GetGlobalContext()->GetData<GlobalSceneSettings>();
    DAVA::float32 debugBoxScale = SIMPLE_COLLISION_BOX_SIZE * settings->debugBoxScale;
    for (const auto& node : sortedHits)
    {
        if (node.first < debugBoxScale)
        {
            continue;
        }

        DAVA::Any objPtr(node.second->userData);
        auto iter = objToPhysx.find(objPtr);
        DVASSERT(iter != objToPhysx.end());
        Selectable hitObject(iter->first);
        hitObject.SetBoundingBox(GetBoundingBox(iter->first));
        result.push_back(hitObject);
    }
}

void SceneCollisionSystem::ObjectsRayTestFromCamera(SelectableGroup::CollectionType& result) const
{
    DAVA::Vector3 traceFrom;
    DAVA::Vector3 traceTo;
    SceneEditor2* sceneEditor = static_cast<SceneEditor2*>(GetScene());

    sceneEditor->cameraSystem->GetRayTo2dPoint(lastMousePos, 1000.0f, traceFrom, traceTo);

    return ObjectsRayTest(traceFrom, traceTo, result);
}

bool SceneCollisionSystem::LandRayTest(const DAVA::Vector3& from, const DAVA::Vector3& to, DAVA::Vector3& intersectionPoint) const
{
    using namespace physx;
    bool result = false;

    DAVA::Vector3 direction = to - from;
    DAVA::float32 distance = direction.Length();
    direction.Normalize();
    PxRaycastBuffer buffer;

    PxQueryFilterData filterData;
    filterData.data = SceneCollisionSystemDetail::landscapeFilterData;
    filterData.flags = PxQueryFlag::eSTATIC;
    if (physicsScene->raycast(DAVA::PhysicsMath::Vector3ToPxVec3(from), DAVA::PhysicsMath::Vector3ToPxVec3(direction), distance, buffer, PxHitFlag::ePOSITION, filterData) == true)
    {
        DVASSERT(buffer.hasBlock);
#ifdef __DAVAENGINE_DEBUG__
        DAVA::Any objPtr(buffer.block.actor->userData);
        auto iter = objToPhysx.find(objPtr);
        DVASSERT(iter != objToPhysx.end());
        Selectable object = Selectable(iter->first);
        DVASSERT(object.CanBeCastedTo<DAVA::Entity>() == true);
        DAVA::Entity* entity = object.Cast<DAVA::Entity>();
        DVASSERT(entity == curLandscapeEntity);
#endif

        DVASSERT(buffer.block.flags.isSet(PxHitFlag::ePOSITION) == true);
        intersectionPoint = DAVA::PhysicsMath::PxVec3ToVector3(buffer.block.position);
        result = true;
    }

    return result;
}

bool SceneCollisionSystem::LandRayTestFromCamera(DAVA::Vector3& intersectionPoint) const
{
    SceneEditor2* sceneEditor = static_cast<SceneEditor2*>(GetScene());
    SceneCameraSystem* cameraSystem = sceneEditor->cameraSystem;

    DAVA::Vector3 camPos = cameraSystem->GetCameraPosition();
    DAVA::Vector3 camDir = cameraSystem->GetPointDirection(lastMousePos);

    DAVA::Vector3 traceFrom = camPos;
    DAVA::Vector3 traceTo = traceFrom + camDir * 1000.0f;

    return LandRayTest(traceFrom, traceTo, intersectionPoint);
}

void SceneCollisionSystem::ClipObjectsToPlanes(const DAVA::Vector<DAVA::Plane>& planes, SelectableGroup::CollectionType& result)
{
    using namespace SceneCollisionSystemDetail;

    result.reserve(64);
    for (const auto& objToPhysxNode : objToPhysx)
    {
        if (objToPhysxNode.first.IsEmpty() == true || objToPhysxNode.second == nullptr)
        {
            continue;
        }

        Selectable object(objToPhysxNode.first);
        physx::PxShape* shape = nullptr;
        objToPhysxNode.second->getShapes(&shape, 1, 0);
        DVASSERT(shape != nullptr);
        if (ClassifyToPlanes(object, shape, planes) == ClassifyPlanesResult::ContainsOrIntersects)
        {
            result.push_back(object);
        }
    }
}

DAVA::Landscape* SceneCollisionSystem::GetLandscape() const
{
    return DAVA::GetLandscape(curLandscapeEntity);
}

void SceneCollisionSystem::UpdateCollisionObject(const Selectable& object, bool forceRecreate)
{
    bool shouldBeRecreated = false;
    EnumerateObjectHierarchy(object, false, [&shouldBeRecreated](const DAVA::Any&, physx::PxRigidActor*, bool recreate) {
        shouldBeRecreated |= recreate;
    });

    shouldBeRecreated |= forceRecreate;
    if (shouldBeRecreated == false)
    {
        objectsToUpdateTransform.insert(object.GetContainedObject());
        return;
    }

    if (object.CanBeCastedTo<DAVA::Entity>() && shouldBeRecreated == false)
    {
        auto entity = object.AsEntity();
        RemoveEntity(entity);
        AddEntity(entity);
    }
    else
    {
        objectsToRemove.insert(object.GetContainedObject());
        objectsToAdd.insert(object.GetContainedObject());
    }
}

DAVA::AABBox3 SceneCollisionSystem::GetBoundingBox(const DAVA::Any& object) const
{
    DVASSERT(object.IsEmpty() == false);

    DAVA::AABBox3 aabox;
    auto iter = objToPhysx.find(object);
    if (iter != objToPhysx.end())
    {
        physx::PxRigidActor* actor = iter->second;
        aabox = *SceneCollisionSystemDetail::GetBounds(actor);

        Selectable wrapper(object);
        if (wrapper.CanBeCastedTo<DAVA::Entity>())
        {
            auto entity = wrapper.AsEntity();
            for (DAVA::int32 i = 0, e = entity->GetChildrenCount(); i < e; ++i)
            {
                DAVA::Entity* childEntity = entity->GetChild(i);
                DAVA::AABBox3 entityBox = GetBoundingBox(childEntity);
                DAVA::AABBox3 entityTransformedBox;
                entityBox.GetTransformedBox(childEntity->GetLocalTransform(), entityTransformedBox);
                aabox.AddAABBox(entityTransformedBox);
            }
        }
    }

    return aabox;
}

void SceneCollisionSystem::Process(DAVA::float32 timeElapsed)
{
    if (!IsSystemEnabled())
    {
        return;
    }

    DAVA::TransformSingleComponent* tsc = GetScene()->transformSingleComponent;
    for (DAVA::Entity* entity : tsc->localTransformChanged)
    {
        UpdateCollisionObject(Selectable(entity));
    }

    for (DAVA::Entity* entity : tsc->transformParentChanged)
    {
        UpdateCollisionObject(Selectable(entity));
    }

    // check in there are entities that should be added or removed
    if (!(objectsToAdd.empty() && objectsToRemove.empty()))
    {
        for (auto obj : objectsToRemove)
        {
            Selectable wrapper(obj);
            EnumerateObjectHierarchy(wrapper, false, [this](const DAVA::Any& object, physx::PxRigidActor*, bool) {
                DVASSERT(object.IsEmpty() == false);
                auto iter = objToPhysx.find(object);
                if (iter != objToPhysx.end())
                {
                    physx::PxRigidActor* rigidActor = iter->second;
                    DVASSERT(rigidActor != nullptr);
                    DAVA::AABBox3* bounds = SceneCollisionSystemDetail::GetBounds(rigidActor);
                    DAVA::SafeDelete(bounds);

                    physicsScene->removeActor(*rigidActor);
                    rigidActor->release();
                    size_t removedCount = objToPhysx.erase(object);
                    DVASSERT(removedCount == 1);
                }
            });
        }

        for (auto obj : objectsToAdd)
        {
            Selectable wrapper(obj);
            if (wrapper.CanBeCastedTo<DAVA::Entity>() || wrapper.SupportsTransformType(Selectable::TransformType::Disabled))
            {
                EnumerateObjectHierarchy(wrapper, true, [this](const DAVA::Any& object, physx::PxRigidActor* actor, bool) {
                    DVASSERT(actor != nullptr);
                    DVASSERT(object.IsEmpty() == false);
                    DVASSERT(objToPhysx.count(object) == 0);
                    objToPhysx[object] = actor;
                    physicsScene->addActor(*actor);
                });
            }
        }

        objectsToAdd.clear();
        objectsToRemove.clear();
    }

    for (const DAVA::Any& object : objectsToUpdateTransform)
    {
        auto iter = objToPhysx.find(object);
        if (iter != objToPhysx.end())
        {
            physx::PxRigidActor* actor = iter->second;

            Selectable wrapper(object);
            SceneCollisionSystemDetail::UpdateActorTransform(wrapper.GetWorldTransform(), actor);
        }
    }
    objectsToUpdateTransform.clear();
}

bool SceneCollisionSystem::Input(DAVA::UIEvent* event)
{
    // don't have to update last mouse pos when event is not from the mouse
    if (DAVA::eInputDevices::MOUSE == event->device)
    {
        lastMousePos = event->point;
    }
    return false;
}

void SceneCollisionSystem::SetScene(DAVA::Scene* scene)
{
    if (scene == GetScene())
    {
        SceneSystem::SetScene(scene);
        return;
    }

    {
        DAVA::Scene* currentScene = GetScene();
        if (currentScene != nullptr)
        {
            currentScene->GetEventSystem()->UnregisterSystemForEvent(this, DAVA::EventSystem::SWITCH_CHANGED);
            currentScene->GetEventSystem()->UnregisterSystemForEvent(this, DAVA::EventSystem::GEO_DECAL_CHANGED);
            DVASSERT(physicsScene != nullptr);
            physicsScene->release();
            DVASSERT(geometryCache != nullptr);
            SafeDelete(geometryCache);
        }
    }

    SceneSystem::SetScene(scene);

    {
        DAVA::Scene* currentScene = GetScene();
        if (currentScene != nullptr)
        {
            currentScene->GetEventSystem()->RegisterSystemForEvent(this, DAVA::EventSystem::SWITCH_CHANGED);
            currentScene->GetEventSystem()->RegisterSystemForEvent(this, DAVA::EventSystem::GEO_DECAL_CHANGED);

            DAVA::PhysicsModule* module = DAVA::GetEngineContext()->moduleManager->GetModule<DAVA::PhysicsModule>();
            DVASSERT(module != nullptr);

            DAVA::PhysicsSceneConfig config;
            config.threadCount = 0;
            physicsScene = module->CreateScene(config, physx::PxDefaultSimulationFilterShader, nullptr);
            geometryCache = new DAVA::PhysicsGeometryCache();
        }
    }
}

void SceneCollisionSystem::ProcessCommand(const RECommandNotificationObject& commandNotification)
{
    if (commandNotification.MatchCommandIDs({ CMDID_HEIGHTMAP_MODIFY }))
    {
        UpdateCollisionObject(Selectable(curLandscapeEntity), true);
    }

    static const DAVA::Vector<DAVA::uint32> acceptableCommands =
    {
      CMDID_LOD_CREATE_PLANE,
      CMDID_LOD_DELETE,
      CMDID_INSP_MEMBER_MODIFY,
      CMDID_REFLECTED_FIELD_MODIFY,
      CMDID_PARTICLE_EFFECT_EMITTER_REMOVE,
      CMDID_TRANSFORM,
      CMDID_CONVERT_TO_BILLBOARD
    };

    if (commandNotification.MatchCommandIDs(acceptableCommands) == false)
        return;

    auto processSingleCommand = [this](const RECommand* command, bool redo) {
        if (command->MatchCommandID(CMDID_REFLECTED_FIELD_MODIFY))
        {
            const DAVA::FastName HEIGHTMAP_PATH("heightmapPath");
            const DAVA::FastName HEIGHTMAP_SIZE("size");
            const SetFieldValueCommand* cmd = static_cast<const SetFieldValueCommand*>(command);
            const DAVA::Reflection::Field& field = cmd->GetField();
            DAVA::ReflectedObject obj = field.ref.GetDirectObject();
            bool isLandscape = obj.GetReflectedType() == DAVA::ReflectedTypeDB::Get<DAVA::Landscape>();
            DAVA::FastName fieldKey = field.key.Cast<DAVA::FastName>(DAVA::FastName(""));
            if (isLandscape == true && (fieldKey == HEIGHTMAP_PATH || fieldKey == HEIGHTMAP_SIZE))
            {
                UpdateCollisionObject(Selectable(curLandscapeEntity), true);
            }
        }
        else if (command->MatchCommandID(CMDID_LOD_CREATE_PLANE))
        {
            const CreatePlaneLODCommand* createPlaneLODCommand = static_cast<const CreatePlaneLODCommand*>(command);
            UpdateCollisionObject(Selectable(createPlaneLODCommand->GetEntity()), true);
        }
        else if (command->MatchCommandIDs({ CMDID_LOD_DELETE }))
        {
            const DeleteLODCommand* deleteLODCommand = static_cast<const DeleteLODCommand*>(command);
            UpdateCollisionObject(Selectable(deleteLODCommand->GetEntity()), true);
        }
        else if (command->MatchCommandID(CMDID_PARTICLE_EFFECT_EMITTER_REMOVE))
        {
            auto cmd = static_cast<const CommandRemoveParticleEmitter*>(command);
            (redo ? objectsToRemove : objectsToAdd).insert(cmd->GetEmitterInstance());
        }
        else if (command->MatchCommandID(CMDID_TRANSFORM))
        {
            auto cmd = static_cast<const TransformCommand*>(command);
            UpdateCollisionObject(cmd->GetTransformedObject());
        }
        else if (command->MatchCommandID(CMDID_CONVERT_TO_BILLBOARD))
        {
            auto cmd = static_cast<const ConvertToBillboardCommand*>(command);
            UpdateCollisionObject(Selectable(cmd->GetEntity()), true);
        }
        else if (command->MatchCommandID(CMDID_BAKE_GEOMERTY))
        {
            auto cmd = static_cast<const BakeGeometryCommand*>(command);
            UpdateCollisionObject(Selectable(cmd->GetEntity()), true);
        }
    };

    commandNotification.ExecuteForAllCommands(processSingleCommand);
}

void SceneCollisionSystem::ImmediateEvent(DAVA::Component* component, DAVA::uint32 event)
{
    if (!IsSystemEnabled())
    {
        return;
    }

    switch (event)
    {
    case DAVA::EventSystem::SWITCH_CHANGED:
    {
        UpdateCollisionObject(Selectable(component->GetEntity()), true);
        break;
    }
    case DAVA::EventSystem::GEO_DECAL_CHANGED:
    {
        UpdateCollisionObject(Selectable(component->GetEntity()), true);
        break;
    }
    default:
        break;
    }
}

void SceneCollisionSystem::AddEntity(DAVA::Entity* entity)
{
    if (!IsSystemEnabled() || entity == nullptr)
        return;

    if (entity == GetScene())
    {
        return;
    }

    if (DAVA::GetLandscape(entity) != nullptr)
    {
        curLandscapeEntity = entity;
    }

    objectsToRemove.erase(entity);
    if (objToPhysx.count(entity) > 0)
    {
        objectsToUpdateTransform.insert(entity);
    }
    else
    {
        objectsToAdd.insert(entity);
    }

    // build collision object for entity childs
    for (int i = 0; i < entity->GetChildrenCount(); ++i)
    {
        AddEntity(entity->GetChild(i));
    }
}

void SceneCollisionSystem::RemoveEntity(DAVA::Entity* entity)
{
    if (!IsSystemEnabled() || entity == nullptr)
        return;

    if (curLandscapeEntity == entity)
    {
        curLandscapeEntity = nullptr;
    }

    objectsToAdd.erase(entity);
    objectsToRemove.insert(entity);

    // destroy collision object for entities childs
    for (int i = 0; i < entity->GetChildrenCount(); ++i)
    {
        RemoveEntity(entity->GetChild(i));
    }
}

void SceneCollisionSystem::PrepareForRemove()
{
    objectsToAdd.clear();
    objectsToRemove.clear();
    objectsToUpdateTransform.clear();
    for (const auto& node : objToPhysx)
    {
        DAVA::Vector<physx::PxShape*> shapes;
        shapes.resize(node.second->getNbShapes());
        node.second->getShapes(shapes.data(), static_cast<physx::PxU32>(shapes.size()), 0);
        for (physx::PxShape* shape : shapes)
        {
            shape->release();
        }
        node.second->release();
    }
    objToPhysx.clear();
    curLandscapeEntity = nullptr;
}

void SceneCollisionSystem::EnableSystem()
{
    EditorSceneSystem::EnableSystem();
    DAVA::Scene* scene = GetScene();
    for (DAVA::int32 i = 0; i < scene->GetChildrenCount(); ++i)
    {
        AddEntity(scene->GetChild(i));
    }
}

void SceneCollisionSystem::EnumerateObjectHierarchy(const Selectable& object, bool createCollision, const TCallBack& callback)
{
    using namespace SceneCollisionSystemDetail;
    GlobalSceneSettings* settings = REGlobal::GetGlobalContext()->GetData<GlobalSceneSettings>();

    void* userData = object.GetContainedObject().Get<void*>();

    auto toVec3Fn = [](DAVA::float32 v) { return DAVA::Vector3(v, v, v); };

    DAVA::float32 debugBoxScale = SIMPLE_COLLISION_BOX_SIZE * settings->debugBoxScale;
    DAVA::float32 debugBoxParticleScale = SIMPLE_COLLISION_BOX_SIZE * settings->debugBoxParticleScale;
    if (object.CanBeCastedTo<DAVA::Entity>())
    {
        CollisionObj result;
        DAVA::Entity* entity = object.AsEntity();

        DAVA::float32 debugBoxUserScale = SIMPLE_COLLISION_BOX_SIZE * settings->debugBoxUserScale;
        DAVA::float32 debugBoxWaypointScale = SIMPLE_COLLISION_BOX_SIZE * settings->debugBoxWaypointScale;

        DAVA::Landscape* landscape = DAVA::GetLandscape(entity);
        if (landscape != nullptr)
        {
            result = CreateLandscape(createCollision, landscape, userData);
        }

        DAVA::ParticleEffectComponent* particleEffect = DAVA::GetEffectComponent(entity);
        if ((result.isValid == false) && (particleEffect != nullptr))
        {
            for (DAVA::int32 i = 0, e = particleEffect->GetEmittersCount(); i < e; ++i)
            {
                EnumerateObjectHierarchy(Selectable(particleEffect->GetEmitterInstance(i)), createCollision, callback);
            }
            result = CreateBox(createCollision, false, entity->GetWorldTransform(), toVec3Fn(debugBoxParticleScale), userData);
        }

        DAVA::GeoDecalComponent* geoDecalComponent = DAVA::GetGeoDecalComponent(entity);
        if ((result.isValid == false) && (geoDecalComponent != nullptr))
        {
            result = CreateBox(createCollision, true, entity->GetWorldTransform(), geoDecalComponent->GetDimensions(), userData);
        }

        DAVA::RenderObject* renderObject = DAVA::GetRenderObject(entity);
        if ((result.isValid == false) && (renderObject != nullptr))
        {
            DAVA::RenderObject::eType objType = renderObject->GetType();
            if (objType == DAVA::RenderObject::TYPE_BILLBOARD)
            {
                const DAVA::AABBox3& box = renderObject->GetBoundingBox();
                Matrix4 transform = Matrix4::MakeTranslation(box.GetCenter());
                result = CreateBox(createCollision, true, transform, toVec3Fn(box.GetSize().x), userData);
            }
            else if ((objType != DAVA::RenderObject::TYPE_SPRITE) && (objType != DAVA::RenderObject::TYPE_VEGETATION))
            {
                result = CreateMesh(createCollision, entity->GetWorldTransform(), renderObject, geometryCache, userData);
            }
        }

        DAVA::Camera* camera = DAVA::GetCamera(entity);
        if ((result.isValid == false) && (camera != nullptr))
        {
            Matrix4 transform = Matrix4::MakeTranslation(camera->GetPosition());
            result = CreateBox(createCollision, true, transform, toVec3Fn(debugBoxScale), userData);
        }

        // build simple collision box for all other entities, that has more than two components
        if ((result.isValid == false) && (entity != nullptr))
        {
            if ((entity->GetComponent<DAVA::SoundComponent>() != nullptr) ||
                (entity->GetComponent<DAVA::LightComponent>() != nullptr) ||
                (entity->GetComponent<DAVA::TextComponent>() != nullptr) ||
                (entity->GetComponent<DAVA::WindComponent>() != nullptr))
            {
                result = CreateBox(createCollision, false, entity->GetWorldTransform(), toVec3Fn(debugBoxScale), userData);
            }
            else if (entity->GetComponent<DAVA::UserComponent>() != nullptr)
            {
                result = CreateBox(createCollision, false, entity->GetWorldTransform(), toVec3Fn(debugBoxUserScale), userData);
            }
            else if (GetWaypointComponent(entity) != nullptr)
            {
                result = CreateBox(createCollision, false, entity->GetWorldTransform(), toVec3Fn(debugBoxWaypointScale), userData);
            }
            else
            {
                result = CreateBox(createCollision, false, entity->GetWorldTransform(), toVec3Fn(debugBoxScale), userData);
            }
        }

        if (result.isValid == true)
        {
            callback(entity, result.collisionObject, result.shouldRecreate);
        }
    }
    else
    {
        DAVA::float32 scale = object.CanBeCastedTo<DAVA::ParticleEmitterInstance>() ? debugBoxParticleScale : debugBoxScale;
        const DAVA::Any& containedObject = object.GetContainedObject();
        CollisionObj result = CreateBox(createCollision, false, object.GetWorldTransform(), toVec3Fn(scale), userData);
        callback(containedObject, result.collisionObject, result.shouldRecreate);
    }
}

DAVA::AABBox3 SceneCollisionSystem::GetUntransformedBoundingBox(const DAVA::Any& entity) const
{
    return GetTransformedBoundingBox(Selectable(entity), DAVA::Matrix4::IDENTITY);
}

DAVA::AABBox3 SceneCollisionSystem::GetTransformedBoundingBox(const Selectable& object, const DAVA::Matrix4& transform) const
{
    DAVA::AABBox3 entityBox = GetBoundingBox(object.GetContainedObject());
    if (object.CanBeCastedTo<DAVA::Entity>())
    {
        // add childs boxes into entity box
        DAVA::Entity* entity = object.AsEntity();
        for (DAVA::int32 i = 0; i < entity->GetChildrenCount(); i++)
        {
            Selectable childEntity(entity->GetChild(i));
            DAVA::AABBox3 childBox = GetTransformedBoundingBox(childEntity, childEntity.GetLocalTransform());
            if (childBox.IsEmpty() == false)
            {
                if (entityBox.IsEmpty())
                {
                    entityBox = childBox;
                }
                else
                {
                    entityBox.AddAABBox(childBox);
                }
            }
        }
    }

    DAVA::AABBox3 ret;
    if (entityBox.IsEmpty() == false)
    {
        entityBox.GetTransformedBox(transform, ret);
    }
    return ret;
}
