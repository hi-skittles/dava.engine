#include "Tests/Utils/TankUtils.h"

using namespace DAVA;

const FastName TankUtils::TankNode::TURRET = FastName("turret");
const FastName TankUtils::TankNode::L_WHEELS = FastName("l_wheels");
const FastName TankUtils::TankNode::R_WHEELS = FastName("r_wheels");
const FastName TankUtils::TankNode::SKINNED_TANK = FastName("SKINNED_TANK");

void TankUtils::MakeSkinnedTank(Entity* sourceTank, Vector<uint32>& outJointIndexes)
{
    ScopedPtr<Entity> skinnedTank(new Entity());
    skinnedTank->SetName(TankUtils::TankNode::SKINNED_TANK);

    Entity* lWheelsRoot = sourceTank->FindByName(TankNode::L_WHEELS);
    Entity* rWheelsRoot = sourceTank->FindByName(TankNode::R_WHEELS);

    Vector<Entity*> wheels;
    lWheelsRoot->GetChildNodes(wheels);
    rWheelsRoot->GetChildNodes(wheels);

    for (Entity* wheel : wheels)
    {
        RenderObject* wheelObject = GetRenderObject(wheel);
        DVASSERT(wheelObject);

        const Vector3& centerPos = wheelObject->GetBoundingBox().GetCenter();

        TransformComponent* tc = wheel->GetComponent<TransformComponent>();
        tc->SetLocalTranslation(centerPos);
    }

    Vector<SkeletonComponent::Joint> tankJoints;
    ScopedPtr<RenderObject> skinnedRo(MeshUtils::CreateHardSkinnedMesh(sourceTank, tankJoints));
    skinnedRo->AddFlag(RenderObject::VISIBLE_REFLECTION | RenderObject::VISIBLE_REFRACTION);

    RenderComponent* renderComponent = skinnedTank->GetOrCreateComponent<RenderComponent>();
    renderComponent->SetRenderObject(skinnedRo);

    uint32 jointsCount = static_cast<uint32>(tankJoints.size());

    for (Entity* wheel : wheels)
    {
        RenderComponent* renderComponent = wheel->GetComponent<RenderComponent>();
        const Vector3& centerPos = renderComponent->GetRenderObject()->GetBoundingBox().GetCenter();

        for (uint32 i = 0; i < jointsCount; i++)
        {
            if (tankJoints[i].name == wheel->GetName())
                outJointIndexes.push_back(i);
        }
    }

    SkeletonComponent* conquerorSkeleton = new SkeletonComponent();
    conquerorSkeleton->SetJoints(tankJoints);

    skinnedTank->AddComponent(conquerorSkeleton);

    Vector<Entity*> sourceTankChildren;
    sourceTank->GetChildEntitiesWithComponent(sourceTankChildren, Type::Instance<RenderComponent>());

    for (auto* child : sourceTankChildren)
    {
        child->RemoveComponent(Type::Instance<RenderComponent>());
    }

    LodComponent* toLod = skinnedTank->GetOrCreateComponent<LodComponent>();
    toLod->EnableRecursiveUpdate();

    sourceTank->AddNode(skinnedTank);
}

void TankUtils::Animate(Entity* tank, const Vector<uint32>& jointIndexes, float32 angle)
{
    Entity* skinnedTank = tank->FindByName(TankUtils::TankNode::SKINNED_TANK);
    Entity* turret = tank->FindByName(TankUtils::TankNode::TURRET);

    SkeletonComponent* skeleton = skinnedTank->GetComponent<SkeletonComponent>();

    const Quaternion& wheelsRotation = Quaternion::MakeRotation(Vector3::UnitX, angle);
    const Quaternion& turrentRotation = Quaternion::MakeRotation(Vector3::UnitZ, angle);

    for (uint32 i = 0; i < jointIndexes.size(); i++)
    {
        skeleton->SetJointOrientation(jointIndexes[i], wheelsRotation);
    }

    // rotate gun shot effect
    TransformComponent* tc = turret->GetComponent<TransformComponent>();
    tc->SetLocalMatrix(turrentRotation.GetMatrix());

    uint32 turretJoint = skeleton->GetJointIndex(TankUtils::TankNode::TURRET);
    JointTransform transform = skeleton->GetJointTransform(turretJoint);
    skeleton->SetJointOrientation(turretJoint, turrentRotation);
}
