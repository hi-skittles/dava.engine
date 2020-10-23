#include "Base/BaseMath.h"
#include "Collision/Collisions.h"

namespace DAVA
{
Collisions::Collisions()
{
    separationAxes.reserve(8);
}
Collisions::~Collisions()
{
}

bool Collisions::Is2DLineIntersects2DLine
(const Vector2& p1, const Vector2& p2,
 const Vector2& p3, const Vector2& p4,
 Vector2& inters)
{
    float a1 = p2.y - p1.y;
    float b1 = p1.x - p2.x;
    float c1 = -a1 * p1.x - b1 * p1.y;

    float a2 = p4.y - p3.y;
    float b2 = p3.x - p4.x;
    float c2 = -a2 * p3.x - b2 * p3.y;

    float determ = (a1 * b2 - a2 * b1);
    if (FLOAT_EQUAL(determ, 0.0f))
    {
        return false;
    }

    inters.x = (b1 * c2 - b2 * c1) / determ;
    inters.y = (a2 * c1 - a1 * c2) / determ;
    return true;
}

float32 Collisions::CalculateDistanceFrom2DPointTo2DLine(const Vector2& lineStart, const Vector2& lineEnd, const Vector2& point)
{
    float32 lineLengthSquared = (lineEnd.x - lineStart.x) * (lineEnd.x - lineStart.x) + (lineEnd.y - lineStart.y) * (lineEnd.y - lineStart.y);
    if (FLOAT_EQUAL(lineLengthSquared, 0.0f))
    {
        // Just a distance between two points.
        return sqrt((point.x - lineStart.x) * (point.x - lineStart.x) + ((point.y - lineStart.y) * (point.y - lineStart.y)));
    }

    float32 numerator = (lineStart.y - lineEnd.y) * point.x + (lineEnd.x - lineStart.x) * point.y +
    (lineStart.x * lineEnd.y - lineEnd.x * lineStart.y);

    // lineLengthSquared == 0 case is handled above.
    return numerator / sqrt(lineLengthSquared);
}

bool Collisions::IsSegmentIntersectsSegment(
const Vector2& p1, const Vector2& p2,
const Vector2& p3, const Vector2& p4,
Vector2& inters)
{
    float x1 = p1.x;
    float y1 = p1.y;

    float x2 = p2.x;
    float y2 = p2.y;

    float x3 = p3.x;
    float y3 = p3.y;

    float x4 = p4.x;
    float y4 = p4.y;

    float determ = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);
    if (FLOAT_EQUAL(determ, 0.0f))
    {
        return false;
    }

    float t1 = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / determ;
    float t2 = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / determ;

    if ((t1 >= 0.0f) && (t1 <= 1.0f))
        if ((t2 >= 0.0f) && (t2 <= 1.0f))
        {
            inters.x = x1 + (x2 - x1) * t1;
            inters.y = y1 + (y2 - y1) * t1;
            return true;
        }
    return false;
}

bool Collisions::Is2DPolygonIntersects2DPolygon(const Polygon2& poly1,
                                                const Polygon2& poly2,
                                                Vector2& inters)
{
    for (int pnt = 0; pnt < poly1.pointCount; pnt++)
    {
        int count = 0;
        float pntY = poly1.points[pnt].y;
        for (int i = 0; i < poly2.pointCount; i++)
        {
            int j = i + 1;
            float iY = poly2.points[i].y;
            float jY = poly2.points[j].y;

            if (iY == jY)
            {
                continue;
            }
            if (iY > pntY && jY > pntY)
            {
                continue;
            }
            if (iY < pntY && jY < pntY)
            {
                continue;
            }
            if (Max(iY, jY) == pntY)
            {
                count++;
            }
            else if (Min(iY, jY) == pntY)
            {
                continue;
            }
            else
            {
                float fpntY = poly1.points[pnt].y;
                float fiY = poly2.points[i].y;
                float fjY = poly2.points[j].y;
                float t = (fpntY - fiY) / (fjY - fiY);
                if (poly2.points[i].x + t * (poly2.points[j].x - poly2.points[i].x) >= poly1.points[pnt].x)
                {
                    count++;
                }
            }
        }
        if (count & 1)
        {
            inters.x = poly1.points[pnt].x;
            inters.y = pntY;
            return true;
        }
    }

    return false;
}

void Collisions::FindIntersectionLineToCircle(const Vector2& p1, const Vector2& p2, const Circle& circle, ContactManifold2& manifold)
{
    float32 a = (p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y);
    float32 b = 2 * ((p2.x - p1.x) * (p1.x - circle.center.x) + (p2.y - p1.y) * (p1.y - circle.center.y));
    float32 c = circle.center.x * circle.center.x + circle.center.y * circle.center.y
    + p1.x * p1.x + p1.y * p1.y - 2 * (circle.center.x * p1.x + circle.center.y * p1.y)
    - circle.radius * circle.radius;
    // -- -- -- -- //
    float32 d = (b * b - 4 * a * c);

    manifold.count = 0;

    if (d < 0.0f)
    {
        // no solutions
        // we do not change anything because we assume that manifold already initialized
        return;
    }

    d = std::sqrt(d);

    if ((d >= 0.0f) && (d <= EPSILON))
    {
        // one solution

        float32 u = -b / (2 * a);
        manifold.count = 0;
        if (u >= 0.0f && u <= 1.0f)
        {
            manifold.AddContactPoint(Vector2(p1.x + (p2.x - p1.x) * u, p1.y + (p2.y - p1.y) * u));
        }
    }
    else if (d > EPSILON)
    {
        // two solutions
        float32 inv2A = 1.0f / (2.0f * a);
        float32 u1 = (-b - d) * inv2A;
        //		if (u1 >= 0.0f - EPSILON && u1 <= 1.0f + EPSILON)
        if (u1 >= 0.0f && u1 <= 1.0f)
        {
            manifold.AddContactPoint(Vector2(p1.x + (p2.x - p1.x) * u1, p1.y + (p2.y - p1.y) * u1));
        }

        float32 u2 = (-b + d) * inv2A;
        //		if (u2 >= 0.0f - EPSILON && u2 <= 1.0f + EPSILON)
        if (u2 >= 0.0f && u2 <= 1.0f)
        {
            manifold.AddContactPoint(Vector2(p1.x + (p2.x - p1.x) * u2, p1.y + (p2.y - p1.y) * u2));
        }
    }
}

void Collisions::FindIntersectionCircleToCircle(const Circle& c1, const Circle& c2, ContactManifold2& manifold)
{
    Vector2 cdiff = c1.center - c2.center;
    float32 radii = c1.radius + c2.radius;
    radii *= radii;
    float32 squareDistance = cdiff.SquareLength();
    if (squareDistance > radii)
    {
        manifold.count = 0;
        return;
    }

    cdiff /= 2;
    manifold.count = 1;
    manifold.contactPoints[0] = c2.center + cdiff;

    // 8293349181
    // Понаморева 11, съезд направо к рынку, поворот на лево, проезжаю мимо школы, поворот еще один на лево.
    //
    // Передаем привет внимательным читателям gamedev.ru, кто нашел этот комент и показал его миру!
}
};
