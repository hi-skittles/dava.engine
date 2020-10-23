#include "Math/Transform.h"
#include "Math/TransformUtils.h"

namespace DAVA
{
namespace TransformDetails
{
void SafeInverse(float32& component)
{
    if (Abs(component) < EPSILON)
    {
        component = 0;
    }
    else
    {
        component = 1.f / component;
    }
}
} // TransformDetails

template <>
bool AnyCompare<Transform>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<Transform>() == v2.Get<Transform>();
}

Transform::Transform()
    : scale(1.f, 1.f, 1.f)
{
}

Transform::Transform(const Vector3& translation_)
    : scale(1.f, 1.f, 1.f)
    , translation(translation_)
{
}

Transform::Transform(const Quaternion& rotation_)
    : scale(1.f, 1.f, 1.f)
    , rotation(rotation_)
{
}

Transform::Transform(const Matrix4& matrix)
{
    matrix.Decomposition(translation, scale, rotation);
}

Transform::Transform(const Vector3& translation_, const Vector3& scale_, const Quaternion& rotation_)
    : translation(translation_)
    , scale(scale_)
    , rotation(rotation_)
{
}

void Transform::Inverse()
{
    rotation.Inverse();

    TransformDetails::SafeInverse(scale.x);
    TransformDetails::SafeInverse(scale.y);
    TransformDetails::SafeInverse(scale.z);

    Vector3 scaledTranslation = scale * -translation;
    translation = rotation.ApplyToVectorFast(scaledTranslation);
}

Transform Transform::operator*(const Transform& transform) const
{
    Transform result;
    result.rotation = transform.rotation * rotation;
    result.scale = transform.scale * scale;
    result.translation = transform.rotation.ApplyToVectorFast(translation) * transform.scale + transform.translation;

    return result;
}

bool Transform::operator!=(const Transform& transform) const
{
    return translation != transform.translation || rotation != transform.rotation || scale != transform.scale;
}

bool Transform::operator==(const Transform& transform) const
{
    return translation == transform.translation && rotation == transform.rotation && scale == transform.scale;
}

const Transform& Transform::operator*=(const Transform& transform)
{
    *this = this->operator*(transform);
    return *this;
}

Vector3 operator*(const Vector3& vector, const Transform& transform)
{
    return TransformUtils::TransformVector(transform, vector);
}
}
