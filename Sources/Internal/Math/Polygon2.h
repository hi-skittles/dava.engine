#ifndef __DAVAENGINE_POLYGON2_H__
#define __DAVAENGINE_POLYGON2_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "Math/Plane.h"

namespace DAVA
{
/**	
	\ingroup math
	\brief Class that represents Polygon in 2D space. 
 */
class Polygon2
{
    const float32 POLYGON_EPSILON;

public:
    int32 pointCount;
    Vector<Vector2> points;

    //! Construct base polygon with 0 points
    inline Polygon2();

    //! Copy constructor
    inline Polygon2(const Polygon2& p);

    //! Copy operator
    inline Polygon2& operator=(const Polygon2& p);

    //! Function to copy source polygon to destination polygon)
    static inline void Copy(const Polygon2& sourcePolygon, Polygon2& destPolygon);

    //! Clear polygon point list
    inline void Clear();

    //! Add point to polygon
    inline void AddPoint(const Vector2& pt);

    //! Update polygon plane from polygon points
    inline void UpdatePlaneFromPoints();

    //! Get point array
    inline Vector2* GetPoints();
    inline const Vector2* GetPoints() const;
    inline int32 GetPointCount() const;

    //! Remove segment with index
    void RemoveSegmentAtIndex(int32 index);

    //! Remove small segments function remove all segments size of which smaller or equal than removalSize
    int32 RemoveSmallSegments(float32 removalSize);

    //! Remove points in given sphere
    void RemovePointsInCircle(const Vector2& center, float32 radius);

    //! Move by polyline and get position on polyline based on distance
    void InterpolatePositionFromDistance(float32 distance, int startSegment, Vector2& resultPosition, int& resultSegmentIndex) const;

    //! Move by polyline and get position on polyline based on distance
    void InterpolatePositionFromDistanceReverse(float32 distance, int startSegment, Vector2& resultPosition, int& resultSegmentIndex) const;

    //! Check if point lays inside polygon
    bool IsPointInside(const Vector2& pt) const;

    //! Square length of poly segments
    float32 SquareLength() const;

    //! Length of poly segments
    float32 Length() const;

    //! Translate whole polygon
    void Translate(const Vector2& translatePos);

    //! Scale whole polygon around given center point
    void Scale(const Vector2& centerPoint, float32 scale);

    //! Transform polygon using matrix
    void Transform(const Matrix3& matrix);

    //! Calculate center point for the polygon
    void CalculateCenterPoint(Vector2& center) const;

    //! Calculate size rect for the polygon
    void CalculateSizeRect(Vector2& size) const;

    //! Calculate center point and radius for polygon
    float32 CalculateSquareRadius(const Vector2& center) const;

    //! Merge all segments triangle's height of which bigger or equal than minTriangleHeight
    static void MergeFlatPolygonSegments(const Polygon2& srcPolygon, Polygon2& destPolygon, float32 minTriangleHeight);
};

// Implementation

//! Construct base polygon with 0 points
inline Polygon2::Polygon2()
    : POLYGON_EPSILON(0.000001f)
    ,
    pointCount(0)
{
}

//! Copy constructor
inline Polygon2::Polygon2(const Polygon2& p)
    : POLYGON_EPSILON(0.000001f)
{
    Copy(p, *this);
}

//! Copy operator
inline Polygon2& Polygon2::operator=(const Polygon2& p)
{
    Copy(p, *this);
    return *this;
}

//! Function to copy source polygon to destination polygon)
inline void Polygon2::Copy(const Polygon2& sourcePolygon, Polygon2& destPolygon)
{
    destPolygon.pointCount = sourcePolygon.pointCount;
    destPolygon.points = sourcePolygon.points;
}

inline void Polygon2::Clear()
{
    pointCount = 0;
    points.clear();
}

//! Add point to polygon
inline void Polygon2::AddPoint(const Vector2& pt)
{
    points.push_back(pt);
    ++pointCount;
}

//! Get point array
inline Vector2* Polygon2::GetPoints()
{
    return &points.front();
}
inline const Vector2* Polygon2::GetPoints() const
{
    return &points.front();
}

inline int32 Polygon2::GetPointCount() const
{
    return pointCount;
}
};

#endif // __DAVAENGINE_POLYGON_H__