#pragma once

#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
class JointTransform
{
public:
    JointTransform() = default;
    explicit JointTransform(const Matrix4& transform);

    bool operator==(const JointTransform& other) const;
    bool operator!=(const JointTransform& other) const;

    void Reset();

    void SetPosition(const Vector3& position);
    void SetOrientation(const Quaternion& orientation);
    void SetScale(float32 scale);

    const Vector3& GetPosition() const;
    const Quaternion& GetOrientation() const;
    float32 GetScale() const;
    Matrix4 GetMatrix() const;

    bool IsEmpty() const;
    bool HasPosition() const;
    bool HasOrientation() const;
    bool HasScale() const;

    void Construct(const Matrix4& transform);

    JointTransform AppendTransform(const JointTransform& transform) const;
    JointTransform GetInverse() const;

    Vector3 ApplyToPoint(const Vector3& point) const;
    AABBox3 ApplyToAABBox(const AABBox3& bbox) const;

    static JointTransform Lerp(const JointTransform& t0, const JointTransform& t1, float32 factor);
    static JointTransform Override(const JointTransform& t0, const JointTransform& t1);

private:
    enum eTransformFlag
    {
        FLAG_POSITION = 1 << 0,
        FLAG_ORIENTATION = 1 << 1,
        FLAG_SCALE = 1 << 2,
    };

    Quaternion orientation;
    Vector3 position;
    float32 scale = 1.f;
    uint8 flags = 0;
};

inline void JointTransform::Reset()
{
    flags = 0;
    position = Vector3();
    orientation = Quaternion();
    scale = 1.f;
}

inline void JointTransform::SetPosition(const Vector3& _position)
{
    position = _position;
    flags |= FLAG_POSITION;
}

inline void JointTransform::SetOrientation(const Quaternion& _orientation)
{
    orientation = _orientation;
    flags |= FLAG_ORIENTATION;
}

inline void JointTransform::SetScale(float32 _scale)
{
    scale = _scale;
    flags |= FLAG_SCALE;
}

inline const Vector3& JointTransform::GetPosition() const
{
    return position;
}

inline const Quaternion& JointTransform::GetOrientation() const
{
    return orientation;
}

inline float32 JointTransform::GetScale() const
{
    return scale;
}

inline bool JointTransform::IsEmpty() const
{
    return (flags == 0);
}

inline bool JointTransform::HasPosition() const
{
    return (flags & FLAG_POSITION) == FLAG_POSITION;
}

inline bool JointTransform::HasOrientation() const
{
    return (flags & FLAG_ORIENTATION) == FLAG_ORIENTATION;
}

inline bool JointTransform::HasScale() const
{
    return (flags & FLAG_SCALE) == FLAG_SCALE;
}

inline Vector3 JointTransform::ApplyToPoint(const Vector3& inVec) const
{
    return position + orientation.ApplyToVectorFast(inVec) * scale;
}

inline bool JointTransform::operator==(const JointTransform& other) const
{
    return (flags == other.flags) && (orientation == other.orientation) && (position == other.position) && (scale == other.scale);
}

inline bool JointTransform::operator!=(const JointTransform& other) const
{
    return !(*this == other);
}

inline void JointTransform::Construct(const Matrix4& transform)
{
    Vector3 scale3;
    transform.Decomposition(position, scale3, orientation);
    scale = scale3.x;

    flags = FLAG_POSITION | FLAG_ORIENTATION | FLAG_SCALE;
}

} //ns