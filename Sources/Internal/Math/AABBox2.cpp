#include "Math/AABBox2.h"

namespace DAVA
{
//! \brief check if bounding box intersect ray
bool AABBox2::IsIntersectsWithRay(Ray2& r, float32& tmin, float32& tmax, float32 t0, float32 t1) const
{
    float32 tymin, tymax;

    float32 divx = 1.0f / r.direction.x;
    if (divx >= 0)
    {
        tmin = (min.x - r.origin.x) * divx;
        tmax = (max.x - r.origin.x) * divx;
    }
    else
    {
        tmin = (max.x - r.origin.x) * divx;
        tmax = (min.x - r.origin.x) * divx;
    }

    float32 divy = 1.0f / r.direction.y;
    if (divy >= 0)
    {
        tymin = (min.y - r.origin.y) * divy;
        tymax = (max.y - r.origin.y) * divy;
    }
    else
    {
        tymin = (max.y - r.origin.y) * divy;
        tymax = (min.y - r.origin.y) * divy;
    }
    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    return ((tmin < t1) && (tmax > t0));
}

bool AABBox2::IsIntersectsWithBox(const AABBox2& box) const
{
    if ((box.min.x > this->max.x) || (this->min.x > box.max.x))
        return false;
    if ((box.min.y > this->max.y) || (this->min.y > box.max.y))
        return false;
    return true;
}
};
