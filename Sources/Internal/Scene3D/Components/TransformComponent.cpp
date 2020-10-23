#include "Scene3D/Components/TransformComponent.h"

#include "Math/TransformUtils.h"
#include "Reflection/ReflectedMeta.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scene3D/Components/SingleComponents/TransformSingleComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(TransformComponent)
{
    ReflectionRegistrator<TransformComponent>::Begin()[M::CantBeCreatedManualyComponent(), M::CantBeDeletedManualyComponent(), M::DeveloperModeOnly()]
    .ConstructorByPointer()
    .Field("localMatrix", &TransformComponent::localTransform)[M::ReadOnly(), M::DisplayName("Local Transform")]
    .Field("worldMatrix", &TransformComponent::worldTransform)[M::ReadOnly(), M::DisplayName("World Transform")]
    .Field("parentMatrix", &TransformComponent::parentTransform)[M::ReadOnly(), M::HiddenField()]
    .End();
}

Component* TransformComponent::Clone(Entity* toEntity)
{
    TransformComponent* newTransform = new TransformComponent();
    newTransform->SetEntity(toEntity);
    newTransform->localTransform = localTransform;
    newTransform->worldTransform = worldTransform;
    newTransform->worldMatrix = worldMatrix;
    newTransform->parent = this->parent;

    return newTransform;
}

void TransformComponent::SetLocalMatrix(const Matrix4& transform)
{
    localTransform = Transform(transform);
    UpdateWorldTransformForEmptyParent();
    MarkLocalChanged();
}

void TransformComponent::SetLocalTransform(const Transform& transform)
{
    localTransform = transform;
    UpdateWorldTransformForEmptyParent();
    MarkLocalChanged();
}

void TransformComponent::SetLocalTranslation(const Vector3& translation)
{
    localTransform.SetTranslation(translation);
    UpdateWorldTransformForEmptyParent();
    MarkLocalChanged();
}

void TransformComponent::SetLocalScale(const Vector3& scale)
{
    localTransform.SetScale(scale);
    UpdateWorldTransformForEmptyParent();
    MarkLocalChanged();
}

void TransformComponent::SetLocalRotation(const Quaternion& rotation)
{
    localTransform.SetRotation(rotation);
    UpdateWorldTransformForEmptyParent();
    MarkLocalChanged();
}

void TransformComponent::SetWorldMatrix(const Matrix4& transform)
{
    worldTransform = Transform(transform);
    MarkWorldChanged();
}

void TransformComponent::UpdateWorldTransformForEmptyParent()
{
    if (parent == nullptr)
    {
        worldTransform = localTransform;
        worldMatrix = TransformUtils::ToMatrix(worldTransform);
    }
}

void TransformComponent::SetParent(Entity* node)
{
    parent = node;

    if (node)
    {
        parentTransform = &node->GetComponent<TransformComponent>()->worldTransform;
    }
    else
    {
        parentTransform = nullptr;
    }

    MarkParentChanged();
}

void TransformComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (nullptr != archive)
    {
        archive->SetVector3("tc.localTranslation", localTransform.GetTranslation());
        archive->SetVector3("tc.localScale", localTransform.GetScale());
        archive->SetVector4("tc.localRotation", localTransform.GetRotation().data);
        archive->SetVector3("tc.worldTranslation", worldTransform.GetTranslation());
        archive->SetVector3("tc.worldScale", worldTransform.GetScale());
        archive->SetVector4("tc.worldRotation", worldTransform.GetRotation().data);
    }
}

void TransformComponent::Deserialize(KeyedArchive* archive, SerializationContext* sceneFile)
{
    if (nullptr != archive)
    {
        if (archive->IsKeyExists("tc.localMatrix"))
        {
            localTransform = Transform(archive->GetMatrix4("tc.localMatrix", Matrix4::IDENTITY));
            worldTransform = Transform(archive->GetMatrix4("tc.worldMatrix", Matrix4::IDENTITY));
        }
        else
        {
            localTransform.SetTranslation(archive->GetVector3("tc.localTranslation", Vector3::Zero));
            localTransform.SetScale(archive->GetVector3("tc.localScale", Vector3(1.f, 1.f, 1.f)));
            localTransform.SetRotation(archive->GetVector4("tc.localRotation", Quaternion::Identity.data).data);
            worldTransform.SetTranslation(archive->GetVector3("tc.worldTranslation", Vector3::Zero));
            worldTransform.SetScale(archive->GetVector3("tc.worldScale", Vector3(1.f, 1.f, 1.f)));
            worldTransform.SetRotation(archive->GetVector4("tc.worldRotation", Quaternion::Identity.data).data);
        }
        worldMatrix = TransformUtils::ToMatrix(worldTransform);
    }

    Component::Deserialize(archive, sceneFile);
}

void TransformComponent::MarkLocalChanged()
{
    if (entity && entity->GetScene() && entity->GetScene()->transformSingleComponent)
    {
        TransformSingleComponent* tsc = entity->GetScene()->transformSingleComponent;
        tsc->localTransformChanged.push_back(entity);
    }
}

void TransformComponent::MarkWorldChanged()
{
    if (entity && entity->GetScene() && entity->GetScene()->transformSingleComponent)
    {
        TransformSingleComponent* tsc = entity->GetScene()->transformSingleComponent;
        tsc->worldTransformChanged.Push(entity);
    }
    worldMatrix = TransformUtils::ToMatrix(worldTransform);
}

void TransformComponent::MarkParentChanged()
{
    if (entity && entity->GetScene() && entity->GetScene()->transformSingleComponent)
    {
        TransformSingleComponent* tsc = entity->GetScene()->transformSingleComponent;
        tsc->transformParentChanged.push_back(entity);
    }
}
}
