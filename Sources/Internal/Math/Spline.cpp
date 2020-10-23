#include "Base/BaseMath.h"

namespace DAVA
{
BasicSpline3::BasicSpline3()
{
    pointCount = 0;
}

BasicSpline3::BasicSpline3(const Polygon3& poly)
{
    Construct(poly);
}

void BasicSpline3::Construct(const Polygon3& poly)
{
    points.clear();
    pointCount = 0;

    for (int k = 0; k < poly.pointCount; ++k)
    {
        SplinePoint point;
        point.point = poly.points[k];
        points.push_back(point);
    }
    pointCount = poly.pointCount;
    RebuildSpline();
}

void BasicSpline3::RebuildSpline()
{
    if (pointCount < 4)
        return;

    points[0].r = points[1].point - points[0].point;
    points[pointCount - 1].r = points[pointCount - 1].point - points[pointCount - 2].point;

    for (int k = 1; k < pointCount - 1; ++k)
    {
        //		Vector3 g1 = points[k].point - points[k - 1].point;
        //		Vector3 g2 = points[k + 1].point - points[k].point;
        //		Vector3 g3 = g2 - g1;
        //		points[k].r = g1 + 0.5f * g3;
        points[k].r = (points[k + 1].point - points[k - 1].point) * 0.5f;
        // to smooth spline this vector can be increased but no more than 3 * cos(a) (a angle between segments)
    }
}

Vector3 BasicSpline3::Evaluate(int segment, float t)
{
    //int segment = tglobal / (pointCount - 1);
    //float t = fmod(tglobal, 1.0f);

    Vector3 p1 = points[segment].point;
    Vector3 p2 = p1;
    Vector3 r1 = points[segment].r;
    Vector3 r2 = r1;
    if (segment + 1 < pointCount)
    {
        p2 = points[segment + 1].point;
        r2 = points[segment + 1].r;
    }

    Vector3 a = 2 * p1 - 2 * p2 + r1 + r2;
    Vector3 b = -3 * p1 + 3 * p2 - 2 * r1 - r2;
    Vector3 c = r1;
    Vector3 d = p1;

    Vector3 res = a * t * t * t + b * t * t + c * t + d;
    return res;
}

Vector3 BasicSpline3::EvaluateDerivative(int segment, float t)
{
    //int segment = tglobal / (pointCount - 1);
    //float t = fmod(tglobal, 1.0f);

    Vector3 p1 = points[segment].point;
    Vector3 p2 = p1;
    Vector3 r1 = points[segment].r;
    Vector3 r2 = r1;
    if (segment + 1 < pointCount)
    {
        p2 = points[segment + 1].point;
        r2 = points[segment + 1].r;
    }

    Vector3 a = 2 * p1 - 2 * p2 + r1 + r2;
    Vector3 b = -3 * p1 + 3 * p2 - 2 * r1 - r2;
    Vector3 c = r1;
    Vector3 d = p1;

    Vector3 res = 3.0f * a * t * t + 2.0f * b * t + c;
    return res;
}

BasicSpline2::BasicSpline2()
{
    pointCount = 0;
}

BasicSpline2::BasicSpline2(const Polygon2& poly)
{
    Construct(poly);
}

void BasicSpline2::Construct(const Polygon2& poly)
{
    points.clear();
    pointCount = 0;

    for (int k = 0; k < poly.pointCount; ++k)
    {
        SplinePoint point;
        point.point = poly.points[k];
        points.push_back(point);
    }
    pointCount = poly.pointCount;
    RebuildSpline();
}

void BasicSpline2::RebuildSpline()
{
    if (pointCount < 4)
        return;

    points[0].r = points[1].point - points[0].point;
    points[pointCount - 1].r = points[pointCount - 1].point - points[pointCount - 2].point;

    for (int k = 1; k < pointCount - 1; ++k)
    {
        //		Vector3 g1 = points[k].point - points[k - 1].point;
        //		Vector3 g2 = points[k + 1].point - points[k].point;
        //		Vector3 g3 = g2 - g1;
        //		points[k].r = g1 + 0.5f * g3;
        points[k].r = (points[k + 1].point - points[k - 1].point) * 0.5f;
        // to smooth spline this vector can be increased but no more than 3 * cos(a) (a angle between segments)
    }
}

Vector2 BasicSpline2::Evaluate(int segment, float t)
{
    //int segment = tglobal / (pointCount - 1);
    //float t = fmod(tglobal, 1.0f);

    Vector2 p1 = points[segment].point;
    Vector2 p2 = p1;
    Vector2 r1 = points[segment].r;
    Vector2 r2 = r1;
    if (segment + 1 < pointCount)
    {
        p2 = points[segment + 1].point;
        r2 = points[segment + 1].r;
    }

    Vector2 a = 2 * p1 - 2 * p2 + r1 + r2;
    Vector2 b = -3 * p1 + 3 * p2 - 2 * r1 - r2;
    Vector2 c = r1;
    Vector2 d = p1;

    Vector2 res = a * t * t * t + b * t * t + c * t + d;
    return res;
}
};
