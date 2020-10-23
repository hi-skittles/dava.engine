#ifndef __DAVAENGINE_SPLINE_H__
#define __DAVAENGINE_SPLINE_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "Math/Plane.h"

namespace DAVA
{
// Base spline class
// all splines must be childs of this class
// all splines is 3D functions and they parametric

class Spline
{
public:
    Spline()
    {
    }
    virtual ~Spline()
    {
    }

    virtual int GetPointCount();
    virtual Vector3 Evaluate(float32 t);
};

//! Class for creation of basic and simple splines through n points

class BasicSpline2
{
public:
    struct SplinePoint
    {
        Vector2 point; //
        Vector2 r; //
    };
    int pointCount;
    Vector<SplinePoint> points;

    BasicSpline2();
    BasicSpline2(const Polygon2& poly);

    void Construct(const Polygon2& poly);
    void RebuildSpline();
    Vector2 Evaluate(int segment, float t);
};

class BasicSpline3
{
public:
    struct SplinePoint
    {
        Vector3 point; //
        Vector3 r; //
    };
    int pointCount;
    Vector<SplinePoint> points;

    BasicSpline3();
    BasicSpline3(const Polygon3& poly);

    void Construct(const Polygon3& poly);
    void RebuildSpline();
    Vector3 Evaluate(int segment, float t);
    Vector3 EvaluateDerivative(int segment, float t);
};
};

#endif // __DAVAENGINE_SPLINE_H__
