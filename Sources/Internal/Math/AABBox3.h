#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Math/Vector.h"
#include "Math/Ray.h"

namespace DAVA
{

#define AABBOX_INFINITY 1000000.0f

/**
	\brief class to work with axial-aligned bounding boxes
	This class can be used for clipping together with Frustum 
 */
class AABBox3
{
public:
    Vector3 min;
    Vector3 max;

    /*	union
	{
		struct 
		{
			struct
			{
				float32 x, y, z;				
			}min, max;
		};
		struct
		{
			float32 x,y,z;
		} verts[2];
	};*/

    //! \brief construct empty bounding box
    inline AABBox3();

    //! \brief construct bounding box from another bounding box
    //! \param _box another bouding box
    inline AABBox3(const AABBox3& _box);

    //! \brief construct bounding box from 2 points
    //! \param _min min point of bounding box
    //! \param _max max point of bounding box
    inline AABBox3(const Vector3& _min, const Vector3& _max);

    //! \brief construct bounding box from center & box size
    //! \param center center point of bounding box
    //! \param size size of bounding box
    inline AABBox3(const Vector3& center, float32 size);

    //! \brief add point to bounding box
    //! if point inside bounding box bounding box not changed
    //! in another case bounding box become larger
    //! \param pt point to add
    inline void AddPoint(const Vector3& pt);
    inline void AddAABBox(const AABBox3& bbox);

    //! \brief check if bounding box intersect other bounding box
    //! \param box another bounding box
    //! \return true if intersect, false otherwise
    inline bool IntersectsWithBox(const AABBox3& box) const;

    //! \brief make bounding box empty
    inline void Empty();

    //! \brief Checks is bounding box is empty
    inline bool IsEmpty() const;

    //! \brief check if point inside bbox.
    inline bool IsInside(const Vector3& pt) const;

    /** 
        \brief Function checks if testBox is fully inside this bounding box. 
        \returns true if testBox is fully inside current bounding box, false otherwise. 
     */
    inline bool IsInside(const AABBox3& testBox) const;

    //! \brief get center
    inline Vector3 GetCenter() const;

    //! \brief get size
    inline Vector3 GetSize() const;

    //! \brief copy operator of bounding box class
    inline AABBox3& operator=(const AABBox3& _bbox);

    //! \brief compare operator of bounding box class
    bool operator==(const AABBox3& _bbox) const
    {
        return (min == _bbox.min) && (max == _bbox.max);
    }
    bool operator!=(const AABBox3& _bbox) const
    {
        return (min != _bbox.min) || (max != _bbox.max);
    }

    void GetTransformedBox(const Matrix4& transform, AABBox3& result) const;
    void GetCorners(Vector3* cornersArray) const;

    float32 GetBoundingSphereRadius() const;
    AABBox3 GetMaxRotationExtentBox(const Vector3& rotationCenter) const;
};

namespace Intersection
{
//! \brief check if bounding box intersect ray
bool RayBox(const Ray3& r, const AABBox3& box);
bool RayBox(const Ray3& r, const AABBox3& box, float32& result);
bool RayBox(const Ray3& r, const AABBox3& box, float32& resultMin, float32& resultMax);
bool SegmentBox(const Vector3& v1, const Vector3& v2, const AABBox3& box, float32& resultMin, float32& resultMax);
bool RayBox(const Ray3Optimized& r, const AABBox3& box, float32& resultMin, float32& resultMax);
//! \brief check if bounding box intersect triangle
bool BoxTriangle(const AABBox3& box, const Vector3& v1, const Vector3& v2, const Vector3& v3);
bool BoxBox(const AABBox3& box1, const AABBox3& box2);
};

//! \brief construct empty bounding box
inline AABBox3::AABBox3()
{
    min = Vector3(AABBOX_INFINITY, AABBOX_INFINITY, AABBOX_INFINITY);
    max = Vector3(-AABBOX_INFINITY, -AABBOX_INFINITY, -AABBOX_INFINITY);
};

//! \brief construct bounding box from another bounding box
//! \param _box another bouding box
inline AABBox3::AABBox3(const AABBox3& _box)
{
    min = _box.min;
    max = _box.max;
}

inline AABBox3::AABBox3(const Vector3& _min, const Vector3& _max)
{
    min = _min;
    max = _max;
}

inline AABBox3::AABBox3(const Vector3& center, float32 size)
{
    min = center - Vector3(size / 2.0f, size / 2.0f, size / 2.0f);
    max = center + Vector3(size / 2.0f, size / 2.0f, size / 2.0f);
}

inline void AABBox3::AddPoint(const Vector3& pt)
{
    if (pt.x < min.x)
        min.x = pt.x;
    if (pt.y < min.y)
        min.y = pt.y;
    if (pt.z < min.z)
        min.z = pt.z;

    if (pt.x > max.x)
        max.x = pt.x;
    if (pt.y > max.y)
        max.y = pt.y;
    if (pt.z > max.z)
        max.z = pt.z;
}

inline void AABBox3::AddAABBox(const AABBox3& bbox)
{
    if (bbox.min.x < min.x)
        min.x = bbox.min.x;
    if (bbox.min.y < min.y)
        min.y = bbox.min.y;
    if (bbox.min.z < min.z)
        min.z = bbox.min.z;

    if (bbox.max.x > max.x)
        max.x = bbox.max.x;
    if (bbox.max.y > max.y)
        max.y = bbox.max.y;
    if (bbox.max.z > max.z)
        max.z = bbox.max.z;
}

//! \brief check if bounding box intersect other bounding box
//! \param box another bounding box
//! \return true if intersect, false otherwise
inline bool AABBox3::IntersectsWithBox(const AABBox3& other) const
{
    return (min.x <= other.max.x && min.y <= other.max.y && min.z <= other.max.z &&
            max.x >= other.min.x && max.y >= other.min.y && max.z >= other.min.z);
};

//! \brief make bounding box empty
inline void AABBox3::Empty()
{
    min = Vector3(AABBOX_INFINITY, AABBOX_INFINITY, AABBOX_INFINITY);
    max = Vector3(-AABBOX_INFINITY, -AABBOX_INFINITY, -AABBOX_INFINITY);
}

inline bool AABBox3::IsEmpty() const
{
    return (min.x > max.x || min.y > max.y || min.z > max.z);
}

inline bool AABBox3::IsInside(const Vector3& pt) const
{
    if (
    (min.x <= pt.x)
    && (min.y <= pt.y)
    && (min.z <= pt.z)
    && (pt.x <= max.x)
    && (pt.y <= max.y)
    && (pt.z <= max.z))
        return true;

    return false;
}

inline bool AABBox3::IsInside(const AABBox3& testBox) const
{
    if (
    (min.x <= testBox.min.x)
    && (min.y <= testBox.min.y)
    && (min.z <= testBox.min.z)
    && (testBox.max.x <= max.x)
    && (testBox.max.y <= max.y)
    && (testBox.max.z <= max.z))
        return true;

    return false;
}

//! \brief copy operator of bounding box class
inline AABBox3& AABBox3::operator=(const AABBox3& _bbox)
{
    min = _bbox.min;
    max = _bbox.max;
    return *this;
}

//! \brief get center
inline Vector3 AABBox3::GetCenter() const
{
    return (min + max) * 0.5f;
}

inline Vector3 AABBox3::GetSize() const
{
    return Vector3(Abs(max.x - min.x), Abs(max.y - min.y), Abs(max.z - min.z));
}

template <>
bool AnyCompare<AABBox3>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);
extern template struct AnyCompare<AABBox3>;

namespace Intersection
{
//! \brief check if bounding box intersect other bounding box
//! \param box another bounding box
//! \return true if intersect, false otherwise
inline bool BoxBox(const AABBox3& box1, const AABBox3& box2)
{
    return (box1.min.x <= box2.max.x && box1.min.y <= box2.max.y && box1.min.z <= box2.max.z &&
            box1.max.x >= box2.min.x && box1.max.y >= box2.min.y && box1.max.z >= box2.min.z);
};
};

}; // namespace DAVA
