#pragma once

#include "Base/BaseTypes.h"
#include "Entity/Component.h"
#include "Math/Transform.h"
#include "Math/TransformUtils.h"
#include "Reflection/Reflection.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/Systems/TransformSystem.h"

namespace DAVA
{
class Entity;
class Transform;

class TransformComponent : public Component
{
public:
    DAVA_DEPRECATED(inline Matrix4* GetWorldMatrixPtr()); //TODO: delete it
    DAVA_DEPRECATED(inline const Matrix4& GetWorldMatrix()); //TODO: delete it
    DAVA_DEPRECATED(inline Matrix4 GetLocalMatrix()); //TODO: delete it

    DAVA_DEPRECATED(void SetWorldMatrix(const Matrix4& transform)); //TODO: delete it
    DAVA_DEPRECATED(void SetLocalMatrix(const Matrix4& transform)); //TODO: delete it

    void SetLocalTranslation(const Vector3& translation);
    void SetLocalScale(const Vector3& scale);
    void SetLocalRotation(const Quaternion& rotation);

    void SetLocalTransform(const Transform& transform);
    const Transform& GetLocalTransform() const;
    const Transform& GetWorldTransform() const;

    void SetParent(Entity* node);

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

private:
    void MarkLocalChanged();
    void MarkWorldChanged();
    void MarkParentChanged();

    void UpdateWorldTransformForEmptyParent();

    Transform localTransform;
    Transform worldTransform;

    Matrix4 worldMatrix = Matrix4::IDENTITY;
    Transform* parentTransform = nullptr;
    Entity* parent = nullptr; //Entity::parent should be removed

    friend class TransformSystem;
    friend class FTransformComponent;

    DAVA_VIRTUAL_REFLECTION(TransformComponent, Component);
};

inline const Matrix4& TransformComponent::GetWorldMatrix()
{
    return worldMatrix;
}

inline Matrix4 TransformComponent::GetLocalMatrix()
{
    return TransformUtils::ToMatrix(localTransform);
}

inline Matrix4* TransformComponent::GetWorldMatrixPtr()
{
    return &worldMatrix;
}

inline const Transform& TransformComponent::GetLocalTransform() const
{
    return localTransform;
}

inline const DAVA::Transform& TransformComponent::GetWorldTransform() const
{
    return worldTransform;
}
}
