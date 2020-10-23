#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Math/Ray.h"
#include "Math/MathConstants.h"

namespace DAVA
{
namespace Intersection
{
bool RayTriangle(const Ray3& ray, const Vector3& p0, const Vector3& p1, const Vector3& p2, float32& t, float32 tMin, float32 tMax)
{
    Vector3 edge1 = p1 - p0;
    Vector3 edge2 = p2 - p0;
    Vector3 pvector = CrossProduct(ray.direction, edge2);

    float32 det = DotProduct(pvector, edge1);
    if (det > -EPSILON && det < EPSILON)
        return false; // Ray are parallel to triangle

    float32 invDet = 1.0f / det;

    Vector3 tvector = ray.origin - p0;
    float u = DotProduct(tvector, pvector) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 qvector = CrossProduct(tvector, edge1);
    float32 v = DotProduct(ray.direction, qvector) * invDet;
    if (v < 0.0f || (u + v) > 1.0f)
        return false;

    t = DotProduct(edge2, qvector) * invDet;
    //intersectionPoint = ray.origin + t * ray.direction;
    return (t >= tMin) && (t <= tMax);
}

bool SegmentTriangle(const Vector3& s0, const Vector3& s1, const Vector3& p0, const Vector3& p1, const Vector3& p2, float32& t, float32 tMin, float32 tMax)
{
    Vector3 raydirection = s1 - s0;
    Vector3 edge1 = p1 - p0;
    Vector3 edge2 = p2 - p0;
    Vector3 pvector = CrossProduct(raydirection, edge2);

    float32 det = DotProduct(pvector, edge1);
    if (det > -EPSILON && det < EPSILON)
        return false; // Ray are parallel to triangle

    float32 invDet = 1.0f / det;

    Vector3 tvector = s0 - p0;
    float u = DotProduct(tvector, pvector) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 qvector = CrossProduct(tvector, edge1);
    float32 v = DotProduct(raydirection, qvector) * invDet;
    if (v < 0.0f || (u + v) > 1.0f)
        return false;

    t = DotProduct(edge2, qvector) * invDet;
    //intersectionPoint = ray.origin + t * ray.direction;
    return (t >= tMin) && (t <= tMax);
}
};
};
