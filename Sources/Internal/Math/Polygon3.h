#ifndef __DAVAENGINE_POLYGON3_H__
#define __DAVAENGINE_POLYGON3_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "Math/Plane.h"

namespace DAVA
{
/**	
	\ingroup math
	\brief Class that represents Polygon in 3D space. 
 */
class Polygon3
{
    const float32 POLYGON_EPSILON;

public:
    int32 pointCount;
    Vector<Vector3> points;
    Plane plane;

    //! Construct base polygon with 0 points
    inline Polygon3();

    //! Copy constructor
    inline Polygon3(const Polygon3& p);

    //! Copy operator
    inline Polygon3& operator=(const Polygon3& p);

    //! Function to copy source polygon to destination polygon)
    static inline void Copy(const Polygon3& sourcePolygon3, Polygon3& destPolygon3);

    //! Clear polygon point list
    inline void Clear();

    //! Add point to polygon
    inline void AddPoint(const Vector3& pt);

    //! Update polygon plane from polygon points
    inline void UpdatePlaneFromPoints();

    //! Get point array
    inline Vector3* GetPoints();
    inline const Vector3* GetPoints() const;

    //! Remove segment with index
    void RemoveSegmentAtIndex(int32 index);

    //! Remove small segments function remove all segments size of which smaller or equal than removalSize
    int32 RemoveSmallSegments(float32 removalSize);

    //! Remove points in given sphere
    void RemovePointsInSphere(const Vector3& center, float32 radius);

    //! Move by polyline and get position on polyline based on distance
    void InterpolatePositionFromDistance(float32 distance, int startSegment, Vector3& resultPosition, int& resultSegmentIndex) const;

    //! Move by polyline and get position on polyline based on distance
    void InterpolatePositionFromDistanceReverse(float32 distance, int startSegment, Vector3& resultPosition, int& resultSegmentIndex) const;

    //! Clip poly by plane (TODO: not tested)
    void ClipByPlane(const Plane& plane);

    //! Check if point lays inside polygon (polygon must be in one plane)
    bool IsPointInside(const Vector3& pt);

    //! Square length of poly segments
    float32 SquareLength() const;

    //! Length of poly segments
    float32 Length() const;

    //! Translate whole polygon
    void Translate(const Vector3& translatePos);

    //! Scale whole polygon around given center point
    void Scale(const Vector3& centerPoint, float32 scale);

    //! Transform polygon using matrix
    void Transform(const Matrix4& matrix);

    //! Calculate center point for the polygon
    void CalculateCenterPoint(Vector3& center) const;

    //! Calculate center point and radius for polygon
    float32 CalculateSquareRadius(const Vector3& center) const;

    //! Merge all segments triangle's height of which bigger or equal than minTriangleHeight
    static void MergeFlatPolygonSegments(const Polygon3& srcPolygon3, Polygon3& destPolygon3, float32 minTriangleHeight);
};

// Implementation

//! Construct base polygon with 0 points
inline Polygon3::Polygon3()
    : POLYGON_EPSILON(0.000001f)
    ,
    pointCount(0)
{
}

//! Copy constructor
inline Polygon3::Polygon3(const Polygon3& p)
    : POLYGON_EPSILON(0.000001f)
{
    Copy(p, *this);
}

//! Copy operator
inline Polygon3& Polygon3::operator=(const Polygon3& p)
{
    Copy(p, *this);
    return *this;
}

//! Function to copy source polygon to destination polygon)
inline void Polygon3::Copy(const Polygon3& sourcePolygon3, Polygon3& destPolygon3)
{
    destPolygon3.pointCount = sourcePolygon3.pointCount;
    destPolygon3.points = sourcePolygon3.points;
}

inline void Polygon3::Clear()
{
    pointCount = 0;
    points.clear();
}

//! Add point to polygon
inline void Polygon3::AddPoint(const Vector3& pt)
{
    points.push_back(pt);
    ++pointCount;
}

//! Update polygon plane from polygon points
inline void Polygon3::UpdatePlaneFromPoints()
{
    if (pointCount >= 3)
        plane = Plane(points[0], points[1], points[2]);
}

//! Get point array
inline Vector3* Polygon3::GetPoints()
{
    return &points.front();
}
inline const Vector3* Polygon3::GetPoints() const
{
    return &points.front();
}
};

#endif // __DAVAENGINE_POLYGON3_H__