#include "JointTransform.h"

namespace DAVA
{
JointTransform::JointTransform(const Matrix4& transform)
{
    Construct(transform);
}

Matrix4 JointTransform::GetMatrix() const
{
    Matrix4 result = HasOrientation() ? orientation.GetMatrix() : Matrix4::IDENTITY;

    if (HasScale())
        result *= Matrix4::MakeScale(Vector3(scale, scale, scale));

    if (HasPosition())
        result.SetTranslationVector(position);

    return result;
}

JointTransform JointTransform::AppendTransform(const JointTransform& transform) const
{
    JointTransform res;
    res.position = ApplyToPoint(transform.position);
    res.scale = scale * transform.scale;

    if (HasOrientation() && transform.HasOrientation())
        res.orientation = orientation * transform.orientation;
    else if (HasOrientation())
        res.orientation = orientation;
    else if (transform.HasOrientation())
        res.orientation = transform.orientation;

    res.flags = flags | transform.flags;

    return res;
}

JointTransform JointTransform::GetInverse() const
{
    JointTransform res;
    res.orientation = HasOrientation() ? orientation.GetInverse() : orientation;
    res.position = HasOrientation() ? -res.orientation.ApplyToVectorFast(position) : -position;
    if (HasScale())
    {
        res.scale = 1.f / scale;
        res.position *= res.scale;
    }
    res.flags = flags;

    return res;
}

AABBox3 JointTransform::ApplyToAABBox(const AABBox3& bbox) const
{
    const Vector3& min = bbox.min;
    const Vector3& max = bbox.max;

    AABBox3 res;
    res.AddPoint(ApplyToPoint(min));
    res.AddPoint(ApplyToPoint(max));
    res.AddPoint(ApplyToPoint(Vector3(min.x, min.y, max.z)));
    res.AddPoint(ApplyToPoint(Vector3(min.x, max.y, min.z)));
    res.AddPoint(ApplyToPoint(Vector3(min.x, max.y, max.z)));
    res.AddPoint(ApplyToPoint(Vector3(max.x, min.y, min.z)));
    res.AddPoint(ApplyToPoint(Vector3(max.x, min.y, max.z)));
    res.AddPoint(ApplyToPoint(Vector3(max.x, max.y, min.z)));

    return res;
}

JointTransform JointTransform::Lerp(const JointTransform& t0, const JointTransform& t1, float32 factor)
{
    JointTransform result;

    if (t0.HasPosition() && t1.HasPosition())
    {
        result.SetPosition(DAVA::Lerp<Vector3>(t0.position, t1.position, factor));
    }
    else if (t0.HasPosition())
    {
        result.SetPosition(t0.position);
    }
    else if (t1.HasPosition())
    {
        result.SetPosition(t1.position);
    }

    if (t0.HasOrientation() && t1.HasOrientation())
    {
        Quaternion sLerp;
        sLerp.Slerp(t0.orientation, t1.orientation, factor);
        sLerp.Normalize();
        result.SetOrientation(sLerp);
    }
    else if (t0.HasOrientation())
    {
        result.SetOrientation(t0.orientation);
    }
    else if (t1.HasOrientation())
    {
        result.SetOrientation(t1.orientation);
    }

    if (t0.HasScale() && t1.HasScale())
    {
        result.SetScale(DAVA::Lerp(t0.scale, t1.scale, factor));
    }
    else if (t0.HasScale())
    {
        result.SetScale(t0.scale);
    }
    else if (t1.HasScale())
    {
        result.SetScale(t1.scale);
    }

    return result;
}

JointTransform JointTransform::Override(const JointTransform& t0, const JointTransform& t1)
{
    JointTransform result = t0;

    if (t1.HasPosition())
        result.SetPosition(t1.GetPosition());

    if (t1.HasOrientation())
        result.SetOrientation(t1.GetOrientation());

    if (t1.HasScale())
        result.SetScale(t1.GetScale());

    return result;
}

} //ns