#include "Math/AABBox3.h"

namespace DAVA
{
void AABBox3::GetTransformedBox(const Matrix4& transform, AABBox3& result) const
{
    if (IsEmpty())
    {
        result.Empty();
        return;
    }

    result.min.x = transform.data[12];
    result.min.y = transform.data[13];
    result.min.z = transform.data[14];

    result.max.x = transform.data[12];
    result.max.y = transform.data[13];
    result.max.z = transform.data[14];

    for (int32 i = 0; i < 3; ++i)
    {
        for (int32 j = 0; j < 3; ++j)
        {
            float32 a = transform._data[j][i] * min.data[j];
            float32 b = transform._data[j][i] * max.data[j];

            if (a < b)
            {
                result.min.data[i] += a;
                result.max.data[i] += b;
            }
            else
            {
                result.min.data[i] += b;
                result.max.data[i] += a;
            }
        };
    }
}

void AABBox3::GetCorners(Vector3* cornersArray) const
{
    cornersArray[0] = min;
    cornersArray[1] = max;
    cornersArray[2].Set(min.x, min.y, max.z);
    cornersArray[3].Set(min.x, max.y, min.z);
    cornersArray[4].Set(max.x, min.y, min.z);
    cornersArray[5].Set(max.x, max.y, min.z);
    cornersArray[6].Set(max.x, min.y, max.z);
    cornersArray[7].Set(min.x, max.y, max.z);
}

float32 AABBox3::GetBoundingSphereRadius() const
{
    Vector3 maxToCenter = max - GetCenter();
    Vector3 minToCenter = min - GetCenter();
    float32 maxCoord = Max(maxToCenter.x, Max(maxToCenter.y, maxToCenter.z));
    float32 minCoord = Max(Abs(minToCenter.x), Max(Abs(minToCenter.y), Abs(minToCenter.z)));
    return Max(maxCoord, minCoord);
}

AABBox3 AABBox3::GetMaxRotationExtentBox(const Vector3& rotationCenter) const
{
    float32 rotationRadius = (GetCenter() - rotationCenter).Length();
    return AABBox3(rotationCenter, 2.0f * (rotationRadius + GetBoundingSphereRadius()));
}

template <>
bool AnyCompare<AABBox3>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<AABBox3>() == v2.Get<AABBox3>();
}

namespace Intersection
{
bool RayBox(const Ray3& r, const AABBox3& box)
{
    float32 resultMin = -std::numeric_limits<float32>::max();
    float32 resultMax = +std::numeric_limits<float32>::max();

    float32 txmin, txmax, tymin, tymax, tzmin, tzmax;

    float32 divx = 1.0f / r.direction.x;
    if (divx >= 0)
    {
        txmin = (box.min.x - r.origin.x) * divx;
        txmax = (box.max.x - r.origin.x) * divx;
    }
    else
    {
        txmin = (box.max.x - r.origin.x) * divx;
        txmax = (box.min.x - r.origin.x) * divx;
    }

    if (txmin > resultMin)
        resultMin = txmin;
    if (txmax < resultMax)
        resultMax = txmax;

    float32 divy = 1.0f / r.direction.y;
    if (divy >= 0)
    {
        tymin = (box.min.y - r.origin.y) * divy;
        tymax = (box.max.y - r.origin.y) * divy;
    }
    else
    {
        tymin = (box.max.y - r.origin.y) * divy;
        tymax = (box.min.y - r.origin.y) * divy;
    }
    if ((resultMin > tymax) || (tymin > resultMax))
        return false;

    if (tymin > resultMin)
        resultMin = tymin;
    if (tymax < resultMax)
        resultMax = tymax;

    float32 divz = 1.0f / r.direction.z;
    if (divz >= 0)
    {
        tzmin = (box.min.z - r.origin.z) * divz;
        tzmax = (box.max.z - r.origin.z) * divz;
    }
    else
    {
        tzmin = (box.max.z - r.origin.z) * divz;
        tzmax = (box.min.z - r.origin.z) * divz;
    }

    if ((resultMin > tzmax) || (tzmin > resultMax))
        return false;
    if (tzmin > resultMin)
        resultMin = tzmin;
    if (tzmax < resultMax)
        resultMax = tzmax;

    return (resultMax >= 0.0f);
}

bool RayBox(const Ray3& r, const AABBox3& box, float32& result)
{
    float32 resultMin = -std::numeric_limits<float32>::max();
    float32 resultMax = +std::numeric_limits<float32>::max();

    float32 txmin, txmax, tymin, tymax, tzmin, tzmax;

    float32 divx = 1.0f / r.direction.x;
    if (divx >= 0)
    {
        txmin = (box.min.x - r.origin.x) * divx;
        txmax = (box.max.x - r.origin.x) * divx;
    }
    else
    {
        txmin = (box.max.x - r.origin.x) * divx;
        txmax = (box.min.x - r.origin.x) * divx;
    }

    if (txmin > resultMin)
        resultMin = txmin;
    if (txmax < resultMax)
        resultMax = txmax;

    float32 divy = 1.0f / r.direction.y;
    if (divy >= 0)
    {
        tymin = (box.min.y - r.origin.y) * divy;
        tymax = (box.max.y - r.origin.y) * divy;
    }
    else
    {
        tymin = (box.max.y - r.origin.y) * divy;
        tymax = (box.min.y - r.origin.y) * divy;
    }
    if ((resultMin > tymax) || (tymin > resultMax))
        return false;

    if (tymin > resultMin)
        resultMin = tymin;
    if (tymax < resultMax)
        resultMax = tymax;

    float32 divz = 1.0f / r.direction.z;
    if (divz >= 0)
    {
        tzmin = (box.min.z - r.origin.z) * divz;
        tzmax = (box.max.z - r.origin.z) * divz;
    }
    else
    {
        tzmin = (box.max.z - r.origin.z) * divz;
        tzmax = (box.min.z - r.origin.z) * divz;
    }

    if ((resultMin > tzmax) || (tzmin > resultMax))
        return false;
    if (tzmin > resultMin)
        resultMin = tzmin;
    if (tzmax < resultMax)
        resultMax = tzmax;

    result = resultMin;
    if (result < 0.0f)
    {
        result = resultMax;
        if (result < 0.0f)
            return false;
    }
    return true;
}

bool RayBox(const Ray3& r, const AABBox3& box, float32& resultMin, float32& resultMax)
{
    resultMin = -std::numeric_limits<float32>::max();
    resultMax = +std::numeric_limits<float32>::max();

    float32 txmin, txmax, tymin, tymax, tzmin, tzmax;

    float32 divx = 1.0f / r.direction.x;
    if (divx >= 0)
    {
        txmin = (box.min.x - r.origin.x) * divx;
        txmax = (box.max.x - r.origin.x) * divx;
    }
    else
    {
        txmin = (box.max.x - r.origin.x) * divx;
        txmax = (box.min.x - r.origin.x) * divx;
    }

    if (txmin > resultMin)
        resultMin = txmin;
    if (txmax < resultMax)
        resultMax = txmax;

    float32 divy = 1.0f / r.direction.y;
    if (divy >= 0)
    {
        tymin = (box.min.y - r.origin.y) * divy;
        tymax = (box.max.y - r.origin.y) * divy;
    }
    else
    {
        tymin = (box.max.y - r.origin.y) * divy;
        tymax = (box.min.y - r.origin.y) * divy;
    }
    if ((resultMin > tymax) || (tymin > resultMax))
        return false;

    if (tymin > resultMin)
        resultMin = tymin;
    if (tymax < resultMax)
        resultMax = tymax;

    float32 divz = 1.0f / r.direction.z;
    if (divz >= 0)
    {
        tzmin = (box.min.z - r.origin.z) * divz;
        tzmax = (box.max.z - r.origin.z) * divz;
    }
    else
    {
        tzmin = (box.max.z - r.origin.z) * divz;
        tzmax = (box.min.z - r.origin.z) * divz;
    }

    if ((resultMin > tzmax) || (tzmin > resultMax))
        return false;
    if (tzmin > resultMin)
        resultMin = tzmin;
    if (tzmax < resultMax)
        resultMax = tzmax;

    return (resultMax >= 0.0f);
}

bool RayBox(const Ray3Optimized& r, const AABBox3& box, float32& resultMin, float32& resultMax)
{
    resultMin = -std::numeric_limits<float32>::max();
    resultMax = +std::numeric_limits<float32>::max();

    float32 txmin, txmax, tymin, tymax, tzmin, tzmax;

    float32 divx = r.invDirection.x;
    if (divx >= 0)
    {
        txmin = (box.min.x - r.origin.x) * divx;
        txmax = (box.max.x - r.origin.x) * divx;
    }
    else
    {
        txmin = (box.max.x - r.origin.x) * divx;
        txmax = (box.min.x - r.origin.x) * divx;
    }

    if (txmin > resultMin)
        resultMin = txmin;
    if (txmax < resultMax)
        resultMax = txmax;

    float32 divy = r.invDirection.y;
    if (divy >= 0)
    {
        tymin = (box.min.y - r.origin.y) * divy;
        tymax = (box.max.y - r.origin.y) * divy;
    }
    else
    {
        tymin = (box.max.y - r.origin.y) * divy;
        tymax = (box.min.y - r.origin.y) * divy;
    }
    if ((resultMin > tymax) || (tymin > resultMax))
        return false;

    if (tymin > resultMin)
        resultMin = tymin;
    if (tymax < resultMax)
        resultMax = tymax;

    float32 divz = r.invDirection.z;
    if (divz >= 0)
    {
        tzmin = (box.min.z - r.origin.z) * divz;
        tzmax = (box.max.z - r.origin.z) * divz;
    }
    else
    {
        tzmin = (box.max.z - r.origin.z) * divz;
        tzmax = (box.min.z - r.origin.z) * divz;
    }

    if ((resultMin > tzmax) || (tzmin > resultMax))
        return false;
    if (tzmin > resultMin)
        resultMin = tzmin;
    if (tzmax < resultMax)
        resultMax = tzmax;
    return (resultMax >= 0.0f);
}

bool SegmentBox(const Vector3& v1, const Vector3& v2, const AABBox3& box, float32& resultMin, float32& resultMax)
{
    Vector3 rorigin = v1;
    Vector3 rdirection = v2 - v1;

    resultMin = -std::numeric_limits<float32>::max();
    resultMax = +std::numeric_limits<float32>::max();

    float32 txmin, txmax, tymin, tymax, tzmin, tzmax;

    float32 divx = 1.0f / rdirection.x;
    if (divx >= 0)
    {
        txmin = (box.min.x - rorigin.x) * divx;
        txmax = (box.max.x - rorigin.x) * divx;
    }
    else
    {
        txmin = (box.max.x - rorigin.x) * divx;
        txmax = (box.min.x - rorigin.x) * divx;
    }

    if (txmin > resultMin)
        resultMin = txmin;
    if (txmax < resultMax)
        resultMax = txmax;

    float32 divy = 1.0f / rdirection.y;
    if (divy >= 0)
    {
        tymin = (box.min.y - rorigin.y) * divy;
        tymax = (box.max.y - rorigin.y) * divy;
    }
    else
    {
        tymin = (box.max.y - rorigin.y) * divy;
        tymax = (box.min.y - rorigin.y) * divy;
    }
    if ((resultMin > tymax) || (tymin > resultMax))
        return false;

    if (tymin > resultMin)
        resultMin = tymin;
    if (tymax < resultMax)
        resultMax = tymax;

    float32 divz = 1.0f / rdirection.z;
    if (divz >= 0)
    {
        tzmin = (box.min.z - rorigin.z) * divz;
        tzmax = (box.max.z - rorigin.z) * divz;
    }
    else
    {
        tzmin = (box.max.z - rorigin.z) * divz;
        tzmax = (box.min.z - rorigin.z) * divz;
    }

    if ((resultMin > tzmax) || (tzmin > resultMax))
        return false;
    if (tzmin > resultMin)
        resultMin = tzmin;
    if (tzmax < resultMax)
        resultMax = tzmax;

    return ((resultMax >= 0.0f) && (resultMin <= 1.0f));
}

/*********************************************************************/
/* AABB-triangle overlap test code                                   */
/* by Tomas Akenine-Möller                                           */
/* http://fileadmin.cs.lth.se/cs/personal/tomas_akenine-moller/code/ */
/*********************************************************************/

#define X 0
#define Y 1
#define Z 2

#define CROSS(dest, v1, v2)\
dest[0] = v1[1] * v2[2] - v1[2] * v2[1];\
dest[1] = v1[2] * v2[0] - v1[0] * v2[2];\
dest[2] = v1[0] * v2[1] - v1[1] * v2[0];

#define DOT(v1, v2) (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2])

#define SUB(dest, v1, v2)\
dest[0] = v1[0] - v2[0];\
dest[1] = v1[1] - v2[1];\
dest[2] = v1[2] - v2[2];

#define FINDMINMAX(x0, x1, x2, min, max)\
min = max = x0;\
if (x1 < min) min = x1;\
if (x2 < min) min = x2;\
if (x1 > max) max = x1;\
if (x2 > max) max = x2;

bool planeBoxOverlap(float normal[3], float vert[3], float maxbox[3]) // -NJMP-
{
    int q;
    float vmin[3], vmax[3], v;
    for (q = X; q <= Z; q++)
    {
        v = vert[q]; // -NJMP-
        if (normal[q] > 0.0f)
        {
            vmin[q] = -maxbox[q] - v; // -NJMP-
            vmax[q] = maxbox[q] - v; // -NJMP-
        }
        else
        {
            vmin[q] = maxbox[q] - v; // -NJMP-
            vmax[q] = -maxbox[q] - v; // -NJMP-
        }
    }

    if (DOT(normal, vmin) > 0.0f)
        return false; // -NJMP-

    if (DOT(normal, vmax) >= 0.0f)
        return true; // -NJMP-

    return false;
}

/*======================== X-tests ========================*/

#define AXISTEST_X01(a, b, fa, fb)\
p0 = a * v0[Y] - b * v0[Z];\
p2 = a * v2[Y] - b * v2[Z];\
if (p0 < p2) { min = p0; max = p2; } else { min = p2; max = p0; }\
rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];\
if (min > rad || max < -rad) return false;

#define AXISTEST_X2(a, b, fa, fb)\
p0 = a * v0[Y] - b * v0[Z];\
p1 = a * v1[Y] - b * v1[Z];\
if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; }\
rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];\
if (min > rad || max < -rad) return false;

/*======================== Y-tests ========================*/

#define AXISTEST_Y02(a, b, fa, fb)\
p0 = -a * v0[X] + b * v0[Z];\
p2 = -a * v2[X] + b * v2[Z];\
if (p0 < p2) { min = p0; max = p2; }\
	else { min = p2; max = p0; }\
		rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];\
	if (min > rad || max < -rad) return false;

#define AXISTEST_Y1(a, b, fa, fb)			\
p0 = -a * v0[X] + b * v0[Z];		 	\
p1 = -a * v1[X] + b * v1[Z];	 	 	\
if (p0 < p1) { min = p0; max = p1; }\
else { min = p1; max = p0; }\
rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];\
if (min > rad || max < -rad) return false;

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)			\
p1 = a * v1[X] - b * v1[Y];			\
p2 = a * v2[X] - b * v2[Y];			 	\
if (p2 < p1) { min = p2; max = p1; }\
else { min = p1; max = p2; }\
rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];\
if (min > rad || max < -rad) return false;

#define AXISTEST_Z0(a, b, fa, fb)			\
p0 = a * v0[X] - b * v0[Y];				\
p1 = a * v1[X] - b * v1[Y];			\
if (p0 < p1) { min = p0; max = p1; }\
else { min = p1; max = p0; }\
rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];\
if (min > rad || max < -rad) return false;

bool BoxTriangle(const AABBox3& box, const Vector3& v0_, const Vector3& v1_, const Vector3& v2_)
{
    Vector3 c = box.GetCenter();
    Vector3 s = box.GetSize() * 0.5f; //

    const float* triverts[3] = { v0_.data, v1_.data, v2_.data };
    float* boxcenter = c.data;
    float* boxhalfsize = s.data;

    /* use separating axis theorem to test overlap between triangle and box */
    /* need to test for overlap in these directions: */
    /* 1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
    /* we do not even need to test these) */
    /* 2) normal of the triangle */
    /* 3) crossproduct(edge from tri, {x,y,z}-directin) */
    /* this gives 3x3=9 more tests */
    float v0[3], v1[3], v2[3];
    float min, max, p0, p1, p2, rad, fex, fey, fez; // -NJMP- "d" local variable removed
    float normal[3], e0[3], e1[3], e2[3];
    /* This is the fastest branch on Sun */
    /* move everything so that the boxcenter is in (0,0,0) */
    SUB(v0, triverts[0], boxcenter);
    SUB(v1, triverts[1], boxcenter);
    SUB(v2, triverts[2], boxcenter);
    /* compute triangle edges */
    SUB(e0, v1, v0); /* tri edge 0 */
    SUB(e1, v2, v1); /* tri edge 1 */
    SUB(e2, v0, v2); /* tri edge 2 */
    /* Bullet 3: */
    /* test the 9 tests first (this was faster) */
    fex = fabsf(e0[X]);
    fey = fabsf(e0[Y]);
    fez = fabsf(e0[Z]);
    AXISTEST_X01(e0[Z], e0[Y], fez, fey);
    AXISTEST_Y02(e0[Z], e0[X], fez, fex);
    AXISTEST_Z12(e0[Y], e0[X], fey, fex);
    fex = fabsf(e1[X]);
    fey = fabsf(e1[Y]);
    fez = fabsf(e1[Z]);
    AXISTEST_X01(e1[Z], e1[Y], fez, fey);
    AXISTEST_Y02(e1[Z], e1[X], fez, fex);
    AXISTEST_Z0(e1[Y], e1[X], fey, fex);
    fex = fabsf(e2[X]);
    fey = fabsf(e2[Y]);
    fez = fabsf(e2[Z]);
    AXISTEST_X2(e2[Z], e2[Y], fez, fey);
    AXISTEST_Y1(e2[Z], e2[X], fez, fex);
    AXISTEST_Z12(e2[Y], e2[X], fey, fex);
/* Bullet 1: */
/* first test overlap in the {x,y,z}-directions */
/* find min, max of the triangle each direction, and test for overlap in */
/* that direction -- this is equivalent to testing a minimal AABB around */
/* the triangle against the AABB */
/* test in X-direction */

#if ORIGINAL_CODE
    //  Original code. It's problem that if works not very reliable for the AABBox3 that defined by min, max. Sometimes if values are close to each other
    // we lose precision on edge of bounding box

    FINDMINMAX(v0[X], v1[X], v2[X], min, max);
    if (min > boxhalfsize[X] || max < -boxhalfsize[X])
        return false;
    /* test in Y-direction */
    FINDMINMAX(v0[Y], v1[Y], v2[Y], min, max);
    if (min > boxhalfsize[Y] || max < -boxhalfsize[Y])
        return false;
    /* test in Z-direction */
    FINDMINMAX(v0[Z], v1[Z], v2[Z], min, max);
    if (min > boxhalfsize[Z] || max < -boxhalfsize[Z])
        return false;
#else
    /*
        Technically it require more fixes in AXISTESTS as well.
        The problem that we can't avoid floating point calculations there, and we do not like idea to add or substract EPSILON there. But probably it's an area
        where this code can be improved.
     */
    FINDMINMAX(v0_.x, v1_.x, v2_.x, min, max);
    if (max < box.min.data[X] || min > box.max.data[X])
        return false;

    FINDMINMAX(v0_.y, v1_.y, v2_.y, min, max);
    if (max < box.min.data[Y] || min > box.max.data[Y])
        return false;

    FINDMINMAX(v0_.z, v1_.z, v2_.z, min, max);
    if (max < box.min.data[Z] || min > box.max.data[Z])
        return false;
#endif

    /* Bullet 2: */
    /* test if the box intersects the plane of the triangle */
    /* compute plane equation of triangle: normal*x+d=0 */
    CROSS(normal, e0, e1);
    // -NJMP- (line removed here)
    return planeBoxOverlap(normal, v0, boxhalfsize); /* box and triangle overlaps */
}

#undef X
#undef Y
#undef Z
#undef SUB
#undef DOT
#undef CROSS
#undef FINDMINMAX
};
};
