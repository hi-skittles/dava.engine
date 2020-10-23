#pragma once

#include "Math/Vector.h"
#include "Math/Matrix4.h"
#include "Math/Quaternion.h"

namespace DAVA
{
class Transform final
{
public:
    Transform();
    Transform(const Vector3& translation);
    Transform(const Quaternion& rotation);
    Transform(const Matrix4& matrix);
    Transform(const Vector3& translation, const Vector3& scale, const Quaternion& rotation);

    void SetTranslation(const Vector3& translation);
    const Vector3& GetTranslation() const;

    void SetScale(const Vector3& scale);
    const Vector3& GetScale() const;

    void SetRotation(const Quaternion& rotation);
    const Quaternion& GetRotation() const;

    void Inverse();

    Transform operator*(const Transform& transform) const;
    const Transform& operator*=(const Transform& transform);

    bool operator==(const Transform& transform) const;
    bool operator!=(const Transform& transform) const;

    friend Vector3 operator*(const Vector3& vector, const Transform& transform);

private:
    Vector3 translation;
    Vector3 scale;
    Quaternion rotation;
};

Vector3 operator*(const Vector3& vector, const Transform& transform);

inline void Transform::SetTranslation(const Vector3& translation_)
{
    translation = translation_;
}

inline const Vector3& Transform::GetTranslation() const
{
    return translation;
}

inline void Transform::SetScale(const Vector3& scale_)
{
    scale = scale_;
}

inline const Vector3& Transform::GetScale() const
{
    return scale;
}

inline void Transform::SetRotation(const Quaternion& rotation_)
{
    rotation = rotation_;
}

inline const Quaternion& Transform::GetRotation() const
{
    return rotation;
}

template <>
bool AnyCompare<Transform>::IsEqual(const Any& v1, const Any& v2);
extern template struct AnyCompare<Transform>;

} //DAVA
