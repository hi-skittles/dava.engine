#include "SkeletonSystem.h"

#include "Animation/AnimationTrack.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Render/Highlevel/SkinnedMesh.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/SkeletonComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/SkeletonAnimation/JointTransform.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/EventSystem.h"

#define RE_DEBUG_PROCESS_TEST_SKINNED_MESHES 0

namespace DAVA
{
SkeletonSystem::SkeletonSystem(Scene* scene)
    : SceneSystem(scene)
{
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::SKELETON_CONFIG_CHANGED);
}

SkeletonSystem::~SkeletonSystem()
{
    GetScene()->GetEventSystem()->UnregisterSystemForEvent(this, EventSystem::SKELETON_CONFIG_CHANGED);
}

void SkeletonSystem::AddEntity(Entity* entity)
{
    entities.push_back(entity);

    SkeletonComponent* component = GetSkeletonComponent(entity);
    DVASSERT(component);

    if (component->configUpdated)
        RebuildSkeleton(component);
}

void SkeletonSystem::RemoveEntity(Entity* entity)
{
    uint32 size = static_cast<uint32>(entities.size());
    for (uint32 i = 0; i < size; ++i)
    {
        if (entities[i] == entity)
        {
            entities[i] = entities[size - 1];
            entities.pop_back();
            return;
        }
    }
    DVASSERT(0);
}

void SkeletonSystem::PrepareForRemove()
{
    entities.clear();
}

void SkeletonSystem::ImmediateEvent(Component* component, uint32 event)
{
    if (event == EventSystem::SKELETON_CONFIG_CHANGED)
        RebuildSkeleton(static_cast<SkeletonComponent*>(component));
}

void SkeletonSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_SKELETON_SYSTEM);

#if RE_DEBUG_PROCESS_TEST_SKINNED_MESHES
    UpdateTestSkeletons();
#endif

    for (int32 i = 0, sz = static_cast<int32>(entities.size()); i < sz; ++i)
    {
        SkeletonComponent* component = GetSkeletonComponent(entities[i]);

        if (component != nullptr)
        {
            if (component->configUpdated)
            {
                RebuildSkeleton(GetSkeletonComponent(entities[i]));
            }

            if (component->startJoint != SkeletonComponent::INVALID_JOINT_INDEX)
            {
                UpdateJointTransforms(component);
                RenderObject* ro = GetRenderObject(entities[i]);
                if (ro != nullptr && (RenderObject::TYPE_SKINNED_MESH == ro->GetType()))
                {
                    UpdateSkinnedMesh(component, static_cast<SkinnedMesh*>(ro));
                }
            }
        }
    }

    DrawSkeletons(GetScene()->renderSystem->GetDebugDrawer());
}

void SkeletonSystem::DrawSkeletons(RenderHelper* drawer)
{
    for (Entity* entity : entities)
    {
        SkeletonComponent* component = GetSkeletonComponent(entity);
        if (component->drawSkeleton)
        {
            const Matrix4& worldTransform = GetTransformComponent(entity)->GetWorldMatrix();

            Vector<Vector3> positions(component->GetJointsCount());
            for (uint32 i = 0; i < component->GetJointsCount(); ++i)
            {
                positions[i] = component->objectSpaceTransforms[i].GetPosition() * worldTransform;
            }

            const Vector<SkeletonComponent::Joint>& joints = component->jointsArray;
            for (uint32 i = 0; i < component->GetJointsCount(); ++i)
            {
                const SkeletonComponent::Joint& cfg = joints[i];
                if (cfg.parentIndex != SkeletonComponent::INVALID_JOINT_INDEX)
                {
                    float32 arrowLength = (positions[cfg.parentIndex] - positions[i]).Length() * 0.25f;
                    drawer->DrawArrow(positions[cfg.parentIndex], positions[i], arrowLength, Color(1.0f, 0.5f, 0.0f, 1.0), RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);
                }

                Vector3 xAxis = component->objectSpaceTransforms[i].ApplyToPoint(Vector3(1.f, 0.f, 0.f)) * worldTransform;
                Vector3 yAxis = component->objectSpaceTransforms[i].ApplyToPoint(Vector3(0.f, 1.f, 0.f)) * worldTransform;
                Vector3 zAxis = component->objectSpaceTransforms[i].ApplyToPoint(Vector3(0.f, 0.f, 1.f)) * worldTransform;

                drawer->DrawLine(positions[i], xAxis, Color::Red, RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);
                drawer->DrawLine(positions[i], yAxis, Color::Green, RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);
                drawer->DrawLine(positions[i], zAxis, Color::Blue, RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);

                //drawer->DrawAABoxTransformed(component->objectSpaceBoxes[i], worldTransform, DAVA::Color::Red, RenderHelper::eDrawType::DRAW_WIRE_NO_DEPTH);
            }
        }
    }
}

void SkeletonSystem::UpdateJointTransforms(SkeletonComponent* skeleton)
{
    DVASSERT(!skeleton->configUpdated);

    uint32 count = skeleton->GetJointsCount();
    for (uint32 currJoint = skeleton->startJoint; currJoint < count; ++currJoint)
    {
        uint32 parentJoint = skeleton->jointInfo[currJoint] & SkeletonComponent::INFO_PARENT_MASK;
        if ((skeleton->jointInfo[currJoint] & SkeletonComponent::FLAG_MARKED_FOR_UPDATED) || ((parentJoint != SkeletonComponent::INVALID_JOINT_INDEX) && (skeleton->jointInfo[parentJoint] & SkeletonComponent::FLAG_UPDATED_THIS_FRAME)))
        {
            //calculate object space transforms
            if (parentJoint == SkeletonComponent::INVALID_JOINT_INDEX) //root
            {
                skeleton->objectSpaceTransforms[currJoint] = skeleton->localSpaceTransforms[currJoint]; //just copy
            }
            else
            {
                skeleton->objectSpaceTransforms[currJoint] = skeleton->objectSpaceTransforms[parentJoint].AppendTransform(skeleton->localSpaceTransforms[currJoint]);
            }

            //calculate final transform including bindTransform
            skeleton->finalTransforms[currJoint] = skeleton->objectSpaceTransforms[currJoint].AppendTransform(skeleton->inverseBindTransforms[currJoint]);

            if (!skeleton->jointsArray[currJoint].bbox.IsEmpty())
            {
                skeleton->objectSpaceBoxes[currJoint] = skeleton->objectSpaceTransforms[currJoint].ApplyToAABBox(skeleton->jointsArray[currJoint].bbox);
            }
            else
            {
                skeleton->objectSpaceBoxes[currJoint].Empty();
            }

            //  add [was updated]  remove [marked for update]
            skeleton->jointInfo[currJoint] &= ~SkeletonComponent::FLAG_MARKED_FOR_UPDATED;
            skeleton->jointInfo[currJoint] |= SkeletonComponent::FLAG_UPDATED_THIS_FRAME;
        }
        else
        {
            /*  remove was updated  - note that as bones come in descending order we do not care that was updated flag would be cared to next frame*/
            skeleton->jointInfo[currJoint] &= ~SkeletonComponent::FLAG_UPDATED_THIS_FRAME;
        }
    }
    skeleton->startJoint = SkeletonComponent::INVALID_JOINT_INDEX;
}

void SkeletonSystem::UpdateSkinnedMesh(SkeletonComponent* skeleton, SkinnedMesh* skinnedMeshObject)
{
    DVASSERT(!skeleton->configUpdated);

    //recalculate object box
    uint32 count = skeleton->GetJointsCount();
    AABBox3 resBox;
    for (uint32 currJoint = 0; currJoint < count; ++currJoint)
    {
        if (!skeleton->objectSpaceBoxes[currJoint].IsEmpty())
        {
            resBox.AddAABBox(skeleton->objectSpaceBoxes[currJoint]);
        }
    }

    skinnedMeshObject->UpdateJointTransforms(skeleton->finalTransforms);
    skinnedMeshObject->SetBoundingBox(resBox); //TODO: *Skinning* decide on bbox calculation

    GetScene()->GetRenderSystem()->MarkForUpdate(skinnedMeshObject);
}

void SkeletonSystem::RebuildSkeleton(SkeletonComponent* skeleton)
{
    skeleton->configUpdated = false;

    size_t jointsCount = skeleton->jointsArray.size();
    skeleton->jointInfo.resize(jointsCount);
    skeleton->localSpaceTransforms.resize(jointsCount);
    skeleton->objectSpaceTransforms.resize(jointsCount);
    skeleton->finalTransforms.resize(jointsCount);
    skeleton->inverseBindTransforms.resize(jointsCount);
    skeleton->objectSpaceBoxes.resize(jointsCount);

    DVASSERT(skeleton->jointsArray.size() < SkeletonComponent::INFO_PARENT_MASK);
    for (uint32 i = 0, sz = static_cast<int32>(skeleton->jointsArray.size()); i < sz; ++i)
    {
        DVASSERT((skeleton->jointsArray[i].parentIndex == SkeletonComponent::INVALID_JOINT_INDEX) || (skeleton->jointsArray[i].parentIndex < i)); //order
        DVASSERT((skeleton->jointsArray[i].parentIndex == SkeletonComponent::INVALID_JOINT_INDEX) || ((skeleton->jointsArray[i].parentIndex & SkeletonComponent::INFO_PARENT_MASK) == skeleton->jointsArray[i].parentIndex)); //parent fits mask

        skeleton->jointInfo[i] = skeleton->jointsArray[i].parentIndex | SkeletonComponent::FLAG_MARKED_FOR_UPDATED;

        JointTransform localTransform;
        localTransform.Construct(skeleton->jointsArray[i].bindTransform);

        skeleton->localSpaceTransforms[i] = localTransform;
        if (skeleton->jointsArray[i].parentIndex == SkeletonComponent::INVALID_JOINT_INDEX)
        {
            skeleton->objectSpaceTransforms[i] = localTransform;
        }
        else
        {
            skeleton->objectSpaceTransforms[i] = skeleton->objectSpaceTransforms[skeleton->jointsArray[i].parentIndex].AppendTransform(localTransform);
        }

        skeleton->inverseBindTransforms[i].Construct(skeleton->jointsArray[i].bindTransformInv);
    }

    skeleton->startJoint = 0;
}

void SkeletonSystem::UpdateTestSkeletons(float32 timeElapsed)
{
    static float32 t = 0;
    t += timeElapsed;

    for (Entity* entity : entities)
    {
        SkeletonComponent* component = GetSkeletonComponent(entity);
        if (component != nullptr)
        {
            static const FastName SOFT_SKINNED_ENTITY_NAME("TestSoftSkinned");

            if (entity->GetName() == SOFT_SKINNED_ENTITY_NAME)
            {
                //Manipulate test soft skinned mesh in 'Debug Functions' in RE
                uint32 jointCount = component->GetJointsCount();
                for (uint32 j = 1; j < jointCount; ++j)
                {
                    component->GetJoint(j).bindTransform.GetTranslationVector();

                    Vector3 position = component->GetJoint(j).bindTransform.GetTranslationVector();
                    position.z += 5.f * sinf(float32(j + t));

                    JointTransform transform;
                    transform.SetPosition(position);

                    component->SetJointTransform(j, transform);
                }
            }
            else
            {
                for (uint32 i = 0, sz = component->GetJointsCount(); i < sz; ++i)
                {
                    component->SetJointOrientation(i, Quaternion::MakeRotationFastY(t));
                }
            }
        }
    }
}
}
