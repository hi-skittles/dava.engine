#ifndef __DAVAENGINE_BEZIER_SPLINE_H__
#define __DAVAENGINE_BEZIER_SPLINE_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "Math/Plane.h"

namespace DAVA
{
class BezierSplineSegment3
{
public:
    BezierSplineSegment3(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4);
    ~BezierSplineSegment3();

    Vector3 Evaluate(float t);

private:
    Vector3 a;
    Vector3 b;
    Vector3 c;
    Vector3 d;
};

class BezierSpline2 //why spline is not a base object?
{
public:
    BezierSpline2();
    ~BezierSpline2();

    void AddPoint(const Vector2& pt);
    int GetPointCount() const;
    Vector2 Evaluate(int segment, float t);

private:
    bool isConstructed;

    void Construct();

    struct IPoint
    {
        Vector2 point;
    };
    int pointCount;
    Vector<IPoint> points;

    Vector2 a, b, c, d;
};

inline int BezierSpline2::GetPointCount() const
{
    return pointCount;
}

//! Class for creation of basic and simple splines through n points

class BezierSpline3 //why spline is not a base object?
{
public:
    BezierSpline3();
    ~BezierSpline3();

    void AddPoint(const Vector3& pt);
    int GetPointCount() const;
    Vector3 Evaluate(int segment, float t);

private:
    bool isConstructed;

    void Construct();

    struct IPoint
    {
        Vector3 point;
    };
    int pointCount;
    Vector<IPoint> points;

    Vector3 a, b, c, d;
};

inline int BezierSpline3::GetPointCount() const
{
    return pointCount;
}
};

#endif // __DAVAENGINE_SPLINE_H__
