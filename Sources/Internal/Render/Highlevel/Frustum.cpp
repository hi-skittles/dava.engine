#include "Render/RenderHelper.h"
#include "Render/Highlevel/Frustum.h"
#include <Render/2D/Systems/RenderSystem2D.h>

namespace DAVA
{
//! \brief Set view frustum from matrix information
//! \param viewProjection view * projection matrix
void Frustum::Build(const Matrix4& viewProjection, bool zeroBaseClipRange)
{
	
#define SETUP_PLANE(plane, x1, x2, x3, x4) \
	planeArray[plane].n.x = -(x1); planeArray[plane].n.y = -(x2); \
	planeArray[plane].n.z = -(x3); planeArray[plane].d = -(x4); \
	planeArray[plane].Normalize();

    // left
    SETUP_PLANE(EFP_LEFT,
                viewProjection._03 + viewProjection._00,
                viewProjection._13 + viewProjection._10,
                viewProjection._23 + viewProjection._20,
                viewProjection._33 + viewProjection._30);
    // right
    SETUP_PLANE(EFP_RIGHT,
                viewProjection._03 - viewProjection._00,
                viewProjection._13 - viewProjection._10,
                viewProjection._23 - viewProjection._20,
                viewProjection._33 - viewProjection._30);

    // bottom
    SETUP_PLANE(EFP_BOTTOM,
                viewProjection._03 + viewProjection._01,
                viewProjection._13 + viewProjection._11,
                viewProjection._23 + viewProjection._21,
                viewProjection._33 + viewProjection._31);

    // top
    SETUP_PLANE(EFP_TOP,
                viewProjection._03 - viewProjection._01,
                viewProjection._13 - viewProjection._11,
                viewProjection._23 - viewProjection._21,
                viewProjection._33 - viewProjection._31);

    if (zeroBaseClipRange)
    {
        SETUP_PLANE(EFP_NEAR,
                    viewProjection._02,
                    viewProjection._12,
                    viewProjection._22,
                    viewProjection._32);
    }
    else
    {
        SETUP_PLANE(EFP_NEAR,
                    viewProjection._03 + viewProjection._02,
                    viewProjection._13 + viewProjection._12,
                    viewProjection._23 + viewProjection._22,
                    viewProjection._33 + viewProjection._32);
    }

    // far
    SETUP_PLANE(EFP_FAR,
                viewProjection._03 - viewProjection._02,
                viewProjection._13 - viewProjection._12,
                viewProjection._23 - viewProjection._22,
                viewProjection._33 - viewProjection._32);

    planeCount = 6;

#undef SETUP_PLANE

    //build plane min/max access array
    planeAccesBits = 0;

    for (int32 i = 0; i < planeCount; i++)
    {
        if (planeArray[i].n.x < 0)
            planeAccesBits |= 1u << (i * 3 + 0);
        if (planeArray[i].n.y < 0)
            planeAccesBits |= 1u << (i * 3 + 1);
        if (planeArray[i].n.z < 0)
            planeAccesBits |= 1u << (i * 3 + 2);
    }
}

//! \brief Check axial aligned bounding box visibility
//! \param min bounding box minimum point
//! \param max bounding box maximum point
bool Frustum::IsInside(const Vector3& min, const Vector3& max) const
{
    for (int plane = 0; plane < planeCount; ++plane)
    {
        Vector3 testPoint;
        if (planeArray[plane].n.x >= 0.0f)
            testPoint.x = min.x;
        else
            testPoint.x = max.x;

        if (planeArray[plane].n.y >= 0.0f)
            testPoint.y = min.y;
        else
            testPoint.y = max.y;

        if (planeArray[plane].n.z >= 0.0f)
            testPoint.z = min.z;
        else
            testPoint.z = max.z;

        if (planeArray[plane].DistanceToPoint(testPoint) > 0.0f)
            return false;
    }
    return true;
}

//! \brief Check axial aligned bounding box visibility
//! \param box bounding box
bool Frustum::IsInside(const AABBox3& box) const
{
    for (int plane = 0; plane < planeCount; ++plane)
    {
        Vector3 testPoint;
        if (planeArray[plane].n.x >= 0.0f)
            testPoint.x = box.min.x;
        else
            testPoint.x = box.max.x;

        if (planeArray[plane].n.y >= 0.0f)
            testPoint.y = box.min.y;
        else
            testPoint.y = box.max.y;

        if (planeArray[plane].n.z >= 0.0f)
            testPoint.z = box.min.z;
        else
            testPoint.z = box.max.z;

        if (planeArray[plane].DistanceToPoint(testPoint) > 0.0f)
            return false;
    }
    return true;
}

bool Frustum::IsInside(const AABBox3* box) const
{
    for (int plane = 0; plane < planeCount; ++plane)
    {
        Vector3 testPoint;
        if (planeArray[plane].n.x >= 0.0f)
            testPoint.x = box->min.x;
        else
            testPoint.x = box->max.x;

        if (planeArray[plane].n.y >= 0.0f)
            testPoint.y = box->min.y;
        else
            testPoint.y = box->max.y;

        if (planeArray[plane].n.z >= 0.0f)
            testPoint.z = box->min.z;
        else
            testPoint.z = box->max.z;

        if (planeArray[plane].DistanceToPoint(testPoint) > 0.0f)
            return false;
    }
    return true;
}

bool Frustum::IsFullyInside(const AABBox3& box) const
{
    for (int plane = 0; plane < planeCount; ++plane)
    {
        if (planeArray[plane].DistanceToPoint(Vector3(box.min.x, box.min.y, box.min.z)) > 0.0f)
        {
            return false;
        }
        if (planeArray[plane].DistanceToPoint(Vector3(box.min.x, box.min.y, box.max.z)) > 0.0f)
        {
            return false;
        }
        if (planeArray[plane].DistanceToPoint(Vector3(box.min.x, box.max.y, box.min.z)) > 0.0f)
        {
            return false;
        }
        if (planeArray[plane].DistanceToPoint(Vector3(box.max.x, box.min.y, box.min.z)) > 0.0f)
        {
            return false;
        }
        if (planeArray[plane].DistanceToPoint(Vector3(box.max.x, box.max.y, box.min.z)) > 0.0f)
        {
            return false;
        }
        if (planeArray[plane].DistanceToPoint(Vector3(box.min.x, box.max.y, box.max.z)) > 0.0f)
        {
            return false;
        }
        if (planeArray[plane].DistanceToPoint(Vector3(box.max.x, box.min.y, box.max.z)) > 0.0f)
        {
            return false;
        }
        if (planeArray[plane].DistanceToPoint(Vector3(box.max.x, box.max.y, box.max.z)) > 0.0f)
        {
            return false;
        }
    }
    return true;
}

//! \brief Check axial aligned bounding box visibility
//! \param min bounding box minimum point
//! \param max bounding box maximum point
Frustum::eFrustumResult Frustum::Classify(const Vector3& min, const Vector3& max) const
{
    bool intersecting = false;
    for (int plane = 0; plane < planeCount; ++plane)
    {
        Vector3 minTest, maxTest;
        if (planeArray[plane].n.x >= 0.0f)
        {
            minTest.x = min.x;
            maxTest.x = max.x;
        }
        else
        {
            minTest.x = max.x;
            maxTest.x = min.x;
        }

        if (planeArray[plane].n.y >= 0.0f)
        {
            minTest.y = min.y;
            maxTest.y = max.y;
        }
        else
        {
            minTest.y = max.y;
            maxTest.y = min.y;
        }

        if (planeArray[plane].n.z >= 0.0f)
        {
            minTest.z = min.z;
            maxTest.z = max.z;
        }
        else
        {
            minTest.z = max.z;
            maxTest.z = min.z;
        }

        if (planeArray[plane].DistanceToPoint(minTest) > 0.0f)
            return EFR_OUTSIDE;

        if (planeArray[plane].DistanceToPoint(maxTest) >= 0.0f)
            intersecting = true;
    }
    if (intersecting)
        return EFR_INTERSECT;
    return EFR_INSIDE;
}

Frustum::eFrustumResult Frustum::Classify(const AABBox3& box) const
{
    return Classify(box.min, box.max);
}

//#define DISTANCE_TO_PLANE(p, vx, vy, vz) ((p.n.x)*(vx)+(p.n.y)*(vy)+(p.n.z)*(vz)+(p.d))

Frustum::eFrustumResult Frustum::Classify(const AABBox3& box, uint8& planeMask, uint8& startId) const
{
    const float32* verts[2] = { box.min.data, box.max.data };
    Frustum::eFrustumResult result = EFR_INSIDE;
    uint8 k;
    const Plane *plane, *startPlane;
    startPlane = planeArray + startId;
    k = 1 << startId;
    uint32 currPlaneAccess = planeAccesBits >> (startId * 3);
    uint32 invPlaneAccess;
    if (k & planeMask)
    {
        if (startPlane->DistanceToPoint(verts[currPlaneAccess & 1][0], verts[(currPlaneAccess >> 1) & 1][1], verts[(currPlaneAccess >> 2) & 1][2]) > 0.0f)
            return EFR_OUTSIDE;
        invPlaneAccess = ~currPlaneAccess;
        if (startPlane->DistanceToPoint(verts[invPlaneAccess & 1][0], verts[(invPlaneAccess >> 1) & 1][1], verts[(invPlaneAccess >> 2) & 1][2]) >= 0.0f)
            result = EFR_INTERSECT;
        else
            planeMask ^= k; //plane is inside
    }
    for (plane = planeArray, k = 1, currPlaneAccess = planeAccesBits, invPlaneAccess = ~planeAccesBits; k <= planeMask; ++plane, k += k, currPlaneAccess >>= 3)
    {
        if ((k & planeMask) && (plane != startPlane))
        {
            if (plane->DistanceToPoint(verts[currPlaneAccess & 1][0], verts[(currPlaneAccess >> 1) & 1][1], verts[(currPlaneAccess >> 2) & 1][2]) > 0.0f)
            {
                startId = static_cast<uint8>(plane - planeArray);
                return EFR_OUTSIDE;
            }
            invPlaneAccess = ~currPlaneAccess;
            if (plane->DistanceToPoint(verts[invPlaneAccess & 1][0], verts[((invPlaneAccess) >> 1) & 1][1], verts[((invPlaneAccess) >> 2) & 1][2]) >= 0.0f)
                result = EFR_INTERSECT;
            else
                planeMask ^= k; //plane is inside
        }
    }
    return result;
}

bool Frustum::IsInside(const AABBox3& box, uint8 planeMask, uint8& startClippingPlane) const
{
    const float32* verts[2] = { box.min.data, box.max.data };
    uint8 k;
    const Plane *plane, *startPlane;
    startPlane = planeArray + startClippingPlane;
    k = 1 << startClippingPlane;
    uint32 currPlaneAccess = planeAccesBits >> (startClippingPlane * 3);
    if (k & planeMask)
    {
        if (startPlane->DistanceToPoint(verts[currPlaneAccess & 1][0], verts[(currPlaneAccess >> 1) & 1][1], verts[(currPlaneAccess >> 2) & 1][2]) > 0.0f)
            return false;
    }
    for (plane = planeArray, k = 1, currPlaneAccess = planeAccesBits; k <= planeMask; ++plane, k += k, currPlaneAccess >>= 3)
    {
        if ((k & planeMask) && (plane != startPlane))
        {
            if (plane->DistanceToPoint(verts[currPlaneAccess & 1][0], verts[(currPlaneAccess >> 1) & 1][1], verts[(currPlaneAccess >> 2) & 1][2]) > 0.0f)
            {
                startClippingPlane = static_cast<uint8>(plane - planeArray);
                return false;
            }
        }
    }
    return true;
}

//! \brief check bounding sphere visibility against frustum
//! \param point sphere center point
//! \param radius sphere radius
bool Frustum::IsInside(const Vector3& point, const float32 radius) const
{
    for (int plane = 0; plane < planeCount; ++plane)
    {
        if (planeArray[plane].DistanceToPoint(point) > radius)
            return false;
    }
    return true;
}

//
void Frustum::DebugDraw(RenderHelper* drawer)
{
    Vector3 p[50];

    if (planeCount < 6)
    {
        return;
    }

    //for (int i = 0; i < )
    p[0] = Plane3Intersection(planeArray[EFP_LEFT],
                              planeArray[EFP_NEAR],
                              planeArray[EFP_BOTTOM]);

    p[1] = Plane3Intersection(planeArray[EFP_RIGHT],
                              planeArray[EFP_NEAR],
                              planeArray[EFP_BOTTOM]);

    p[3] = Plane3Intersection(planeArray[EFP_LEFT],
                              planeArray[EFP_NEAR],
                              planeArray[EFP_TOP]);

    p[2] = Plane3Intersection(planeArray[EFP_RIGHT],
                              planeArray[EFP_NEAR],
                              planeArray[EFP_TOP]);

    //for (int i = 0; i < )
    p[4] = Plane3Intersection(planeArray[EFP_LEFT],
                              planeArray[EFP_FAR],
                              planeArray[EFP_BOTTOM]);

    p[5] = Plane3Intersection(planeArray[EFP_RIGHT],
                              planeArray[EFP_FAR],
                              planeArray[EFP_BOTTOM]);

    p[7] = Plane3Intersection(planeArray[EFP_LEFT],
                              planeArray[EFP_FAR],
                              planeArray[EFP_TOP]);

    p[6] = Plane3Intersection(planeArray[EFP_RIGHT],
                              planeArray[EFP_FAR],
                              planeArray[EFP_TOP]);

    drawer->DrawLine(p[0], p[1], Color::White);
    drawer->DrawLine(p[1], p[2], Color::White);
    drawer->DrawLine(p[2], p[3], Color::White);
    drawer->DrawLine(p[3], p[0], Color::White);

    drawer->DrawLine(p[4], p[5], Color::White);
    drawer->DrawLine(p[5], p[6], Color::White);
    drawer->DrawLine(p[6], p[7], Color::White);
    drawer->DrawLine(p[7], p[4], Color::White);

    drawer->DrawLine(p[0], p[4], Color::White);
    drawer->DrawLine(p[1], p[5], Color::White);
    drawer->DrawLine(p[2], p[6], Color::White);
    drawer->DrawLine(p[3], p[7], Color::White);
}
};