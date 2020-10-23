#pragma once

#include "Math/Vector.h"
#include "Math/Matrix4.h"
#include "Math/Transform.h"
#include "Math/Quaternion.h"

namespace DAVA
{
class TransformUtils final
{
public:
    static const Transform IDENTITY;

    static Transform MakeTranslation(const Vector3& translation);
    static Transform MakeRotation(const Vector3& axis, float32 angleInRadians);
    static Transform MakeScale(const Vector3& scaleVector);

    static Vector3 TransformVector(const Transform& transform, const Vector3& vector);
    static Transform Inverse(const Transform& transform);
    static Matrix4 ToMatrix(const Transform& transform);

    static Transform BuildOriented(const Vector3& direction, const Vector3& translation);
};

inline Transform TransformUtils::MakeTranslation(const Vector3& translationVector)
{
    return Transform(translationVector);
}

inline Transform TransformUtils::MakeRotation(const Vector3& axis, float32 angleInRadians)
{
    return Transform(Quaternion::MakeRotation(axis, angleInRadians));
}

inline Transform TransformUtils::MakeScale(const Vector3& scaleVector)
{
    return Transform(Vector3::Zero, scaleVector, Quaternion());
}

} // DAVA
