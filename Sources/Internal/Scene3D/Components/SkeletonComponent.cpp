#include "Scene3D/Components/SkeletonComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
REGISTER_CLASS(SkeletonComponent)

DAVA_VIRTUAL_REFLECTION_IMPL(SkeletonComponent::Joint)
{
    ReflectionRegistrator<SkeletonComponent::Joint>::Begin()
    .Field("name", &SkeletonComponent::Joint::name)[M::DisplayName("Name")]
    .Field("uid", &SkeletonComponent::Joint::uid)[M::DisplayName("UID")]
    .Field("bbox", &SkeletonComponent::Joint::bbox)[M::DisplayName("Bounding box")]
    .Field("bindTransformInv", &SkeletonComponent::Joint::bbox)[M::DisplayName("Bind Transform Inverse")]
    .End();
}

DAVA_VIRTUAL_REFLECTION_IMPL(SkeletonComponent)
{
    ReflectionRegistrator<SkeletonComponent>::Begin()[M::CantBeCreatedManualyComponent(), M::CantBeDeletedManualyComponent()]
    .Field("joints", &SkeletonComponent::jointsArray)[M::DisplayName("Joints")]
    .Field("drawSkeleton", &SkeletonComponent::drawSkeleton)[M::DisplayName("Draw Skeleton")]
    .End();
}

bool SkeletonComponent::Joint::operator==(const Joint& other) const
{
    return parentIndex == other.parentIndex &&
    name == other.name &&
    uid == other.uid &&
    bbox == other.bbox &&
    bindTransform == other.bindTransform &&
    bindTransformInv == other.bindTransformInv;
}

void SkeletonComponent::SetJoints(const Vector<Joint>& config)
{
    jointsArray = config;

    GlobalEventSystem::Instance()->Event(this, EventSystem::SKELETON_CONFIG_CHANGED);

    UpdateJointsMap();
    UpdateDefaultPose();
}

void SkeletonComponent::UpdateDefaultPose()
{
    uint32 jointCount = uint32(jointsArray.size());

    defaultPose.SetJointCount(jointCount);
    for (uint32 j = 0; j < jointCount; ++j)
    {
        defaultPose.SetTransform(j, JointTransform(jointsArray[j].bindTransform));
    }
}

const SkeletonPose& SkeletonComponent::GetDefaultPose() const
{
    return defaultPose;
}

void SkeletonComponent::ApplyPose(const SkeletonPose& pose)
{
    uint32 minJoints = Min(GetJointsCount(), pose.GetJointsCount());
    for (uint32 j = 0; j < minJoints; ++j)
    {
        const JointTransform& poseTransform = pose.GetJointTransform(j);

        if (poseTransform.IsEmpty())
            continue;

        JointTransform& transform = localSpaceTransforms[j];

        if (poseTransform.HasPosition())
            transform.SetPosition(poseTransform.GetPosition());
        if (poseTransform.HasOrientation())
            transform.SetOrientation(poseTransform.GetOrientation());
        if (poseTransform.HasScale())
            transform.SetScale(poseTransform.GetScale());

        SetJointTransform(j, transform);
    }
}

Component* SkeletonComponent::Clone(Entity* toEntity)
{
    SkeletonComponent* newComponent = new SkeletonComponent();
    newComponent->SetEntity(toEntity);
    newComponent->SetJoints(jointsArray);
    return newComponent;
}

void SkeletonComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);
    archive->SetUInt32("jointsCount", static_cast<uint32>(jointsArray.size()));
    ScopedPtr<KeyedArchive> jointsArch(new KeyedArchive());
    for (size_t i = 0, sz = jointsArray.size(); i < sz; ++i)
    {
        const Joint& joint = jointsArray[i];
        ScopedPtr<KeyedArchive> jointArch(new KeyedArchive());
        jointArch->SetFastName("joint.name", joint.name);
        jointArch->SetFastName("joint.uid", joint.uid);
        jointArch->SetUInt32("joint.parentIndex", joint.parentIndex);
        jointArch->SetVector3("joint.bbox.min", joint.bbox.min);
        jointArch->SetVector3("joint.bbox.max", joint.bbox.max);
        jointArch->SetMatrix4("joint.bindPose", joint.bindTransform);
        jointArch->SetMatrix4("joint.invBindPose", joint.bindTransformInv);

        jointsArch->SetArchive(KeyedArchive::GenKeyFromIndex(static_cast<uint32>(i)), jointArch);
    }

    archive->SetArchive("joints", jointsArch);
}

void SkeletonComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Deserialize(archive, serializationContext);

    uint32 jointsCount = archive->GetUInt32("jointsCount", static_cast<uint32>(jointsArray.size()));
    jointsArray.resize(jointsCount);

    KeyedArchive* jointsArch = archive->GetArchive("joints");
    for (uint32 i = 0; i < jointsCount; ++i)
    {
        Joint& joint = jointsArray[i];
        KeyedArchive* jointArch = jointsArch->GetArchive(KeyedArchive::GenKeyFromIndex(i));
        joint.name = jointArch->GetFastName("joint.name");
        joint.uid = jointArch->GetFastName("joint.uid");
        joint.parentIndex = jointArch->GetUInt32("joint.parentIndex", INVALID_JOINT_INDEX);
        joint.bbox.min = jointArch->GetVector3("joint.bbox.min");
        joint.bbox.max = jointArch->GetVector3("joint.bbox.max");
        joint.bindTransform = jointArch->GetMatrix4("joint.bindPose");
        joint.bindTransformInv = jointArch->GetMatrix4("joint.invBindPose");
    }

    UpdateJointsMap();
    UpdateDefaultPose();
}

void SkeletonComponent::UpdateJointsMap()
{
    jointMap.clear();
    uint32 jointCount = uint32(jointsArray.size());
    for (uint32 j = 0; j < jointCount; ++j)
    {
        const FastName& jointUID = jointsArray[j].uid;
        DVASSERT(jointMap.count(jointUID) == 0); //duplicate bone name

        jointMap[jointUID] = j;
    }
}

template <>
bool AnyCompare<SkeletonComponent::Joint>::IsEqual(const Any& v1, const Any& v2)
{
    return v1.Get<SkeletonComponent::Joint>() == v2.Get<SkeletonComponent::Joint>();
}
}