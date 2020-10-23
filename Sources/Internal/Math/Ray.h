#ifndef __DAVAENGINE_RAY_H__
#define __DAVAENGINE_RAY_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"

namespace DAVA
{
/**	
	\ingroup math
	\brief Ray in 2D space.
 */
class Ray2
{
public:
    inline Ray2(const Vector2& _origin, const Vector2& _direction);
    inline Vector2 ToPoint(float32 t) const;

    Vector2 origin;
    Vector2 direction;
};

/**	
	\ingroup math
	\brief Ray in 3D space.
 */
class Ray3
{
public:
    inline Ray3(const Vector3& _origin, const Vector3& _direction);
    inline Vector3 ToPoint(float32 t) const;

    Vector3 origin;
    Vector3 direction;
};

class Ray3Optimized : public Ray3
{
public:
    inline Ray3Optimized(const Vector3& _origin, const Vector3& _direction);

    Vector3 invDirection;
    uint32 sign;
};

// Implementation

inline Ray2::Ray2(const Vector2& _origin, const Vector2& _direction)
    : origin(_origin)
    , direction(_direction)
{
}

inline Ray3::Ray3(const Vector3& _origin, const Vector3& _direction)
    : origin(_origin)
    , direction(_direction)
{
}

inline Ray3Optimized::Ray3Optimized(const Vector3& _origin, const Vector3& _direction)
    : Ray3(_origin, _direction)
{
    invDirection = 1.0f / direction;
    sign = (invDirection.x < 0.0f) | ((invDirection.y < 0.0f) << 1) | ((invDirection.z < 0.0f) << 2);
}

// Implementation
inline Vector2 Ray2::ToPoint(float32 t) const
{
    return Vector2(origin + t * direction);
}

inline Vector3 Ray3::ToPoint(float32 t) const
{
    return Vector3(origin + t * direction);
}

/**
    \ingroup math
    \brief Find intersection between triangle and ray. 
 
    Function find intersection parameter T that can be in range [0.0f, Infinity). It means that 
    function count that ray is infinite. If you need to find intersection between Triangle and Line you 
    need to manually check if intersection parameter are in desired bounds. 
    
    For example: for Ray3(Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0, 0.0, 0.5f)) and triangle
        Vector3(0.0f, 0.0f, 0.0f), Vector3(15.0f, 0.0f, 0.0f), Vector3(0.0f, 15.0f, 0.0f) intersection result 
    will be true, and T value will be equal 2.0f.
 */
namespace Intersection
{
bool RayTriangle(const Ray3& ray, const Vector3& p0, const Vector3& p1, const Vector3& p2, float32& t, float32 tMin = 0.0f, float32 tMax = FLOAT_MAX);
bool SegmentTriangle(const Vector3& s0, const Vector3& s1, const Vector3& p0, const Vector3& p1, const Vector3& p2, float32& t, float32 tMin = 0.0f, float32 tMax = FLOAT_MAX);
};
};

#endif // __DAVAENGINE_RAY_H__
