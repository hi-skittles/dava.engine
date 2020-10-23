#ifndef __DAVAENGINE_PLANE_H__
#define __DAVAENGINE_PLANE_H__

#include "Math/Matrix3.h"

namespace DAVA
{
/**	
	\ingroup math
	\brief Class to work plane in 3D space. 
 */
class Plane
{
public:
    Vector3 n;
    float32 d;

    //! Basic constructor
    inline Plane();
    //! Construct plane from 3 points
    inline Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3);
    //! Construct plane from normal & distance
    inline Plane(const Vector3& n, float d);
    //! Construct plane from normal & distance
    inline Plane(const Vector4& nd);
    //! Construct plane from normal & point on plane
    inline Plane(const Vector3& n, const Vector3& pOnPlane);

    //! Change plane normal positive direction
    inline void Inverse();

    //! Find distance from plane to point
    inline float32 DistanceToPoint(const Vector3& point) const;
    inline float32 DistanceToPoint(float px, float py, float pz) const;

    //! Find intersection with ray.
    inline bool IntersectByRay(const Vector3& from, const Vector3& to, float32& intrsectDistance) const;

    //! Normalize plane equation
    inline void Normalize();

    /// Projects point onto this plane
    void ProjectPoint(const DAVA::Vector3& p, DAVA::Vector3& projected) const
    {
        float dist = DistanceToPoint(p);
        projected = p - dist * n;
    }
};

inline Vector3 Plane3Intersection(const Plane& p1, const Plane& p2, const Plane& p3);

// Plane class implementation
//! Basic constructor
inline Plane::Plane()
    : n(0.0f, 0.0f, 0.0f)
    , d(0.0f)
{
}

//! Construct plane from 3 points
inline Plane::Plane(const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
    n = CrossProduct(p2 - p0, p1 - p0);
    n.Normalize();
    d = -n.DotProduct(p0);
}

//! Construct plane from normal & distance
inline Plane::Plane(const Vector3& _n, float _d)
    : n(_n)
    , d(_d)
{
}
//! Construct plane from normal & distance
inline Plane::Plane(const Vector4& _nd)
{
    n.x = _nd.x;
    n.y = _nd.y;
    n.z = _nd.z;
    d = _nd.w;
}

//! Construct plane from normal & point on plane
inline Plane::Plane(const Vector3& _n, const Vector3& pOnPlane)
    : n(_n)
{
    d = -n.DotProduct(pOnPlane);
}

//! Change plane normal positive direction
inline void Plane::Inverse()
{
    //TODO: add operator-() to Vector3 & 4 classes
    n = -n;
}

//! Find distance from plane to point
inline float32 Plane::DistanceToPoint(const Vector3& point) const
{
    return (n.x * point.x + n.y * point.y + n.z * point.z + d);
    //return (n.DotProduct(point) + d);
}

inline float32 Plane::DistanceToPoint(float px, float py, float pz) const
{
    return (n.x * px + n.y * py + n.z * pz + d);
}

inline bool Plane::IntersectByRay(const Vector3& from, const Vector3& to, float32& intrsectDistance) const
{
    float numer = -(d + (n.DotProduct(from)));
    float denom = n.DotProduct(to);

    if (Abs(denom) < EPSILON)
    {
        return false;
    }

    intrsectDistance = numer / denom;

    return true;
}

inline void Plane::Normalize()
{
    float invLen = 1.0f / n.Length();
    n *= invLen;
    d *= invLen;
}

inline Vector3 Plane3Intersection(const Plane& plane1, const Plane& plane2, const Plane& plane3)
{
    Vector3 p1, p2, p3;

    p1 = -plane1.d * plane1.n;
    p2 = -plane2.d * plane2.n;
    p3 = -plane3.d * plane3.n;

    Matrix3 mat(plane1.n.x, plane2.n.x, plane3.n.x,
                plane1.n.y, plane2.n.y, plane3.n.y,
                plane1.n.z, plane2.n.z, plane3.n.z);

    Vector3 resPoint;

    resPoint = (DotProduct(p1, plane1.n) * CrossProduct(plane2.n, plane3.n) +
                DotProduct(p2, plane2.n) * CrossProduct(plane3.n, plane1.n) +
                DotProduct(p3, plane3.n) * CrossProduct(plane1.n, plane2.n))
    / (mat.Det());

    return resPoint;
}
};

#endif // __DAVAENGINE_PLANE_H__
