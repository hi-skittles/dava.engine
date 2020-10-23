%module Polygon2
%{
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Math/Vector.h"
%}

%import Vector.i

namespace DAVA
{

class Polygon2
{
public:
	
	//! Construct base polygon with 0 points
	Polygon2();

	//! Copy constructor 
	Polygon2(const Polygon2 & p);
	
	//! Copy operator
	Polygon2 & operator = (const Polygon2 & p);

	//! Get point array
	Vector2 * GetPoints();
    const Vector2 * GetPoints() const;
    int32 GetPointCount() const;
	
	//! Check if point lays inside polygon
	bool IsPointInside(const Vector2 & pt) const;
	
    //! Calculate center point for the polygon
	void CalculateCenterPoint(Vector2 & center) const;

    //! Calculate size rect for the polygon
    void CalculateSizeRect(Vector2 &size) const;

	//! Calculate center point and radius for polygon
	float32 CalculateSquareRadius(const Vector2 & center) const;
	
	//! Merge all segments triangle's height of which bigger or equal than minTriangleHeight 
	static void MergeFlatPolygonSegments(const Polygon2 &srcPolygon, Polygon2 &destPolygon, float32 minTriangleHeight);
};
	
};