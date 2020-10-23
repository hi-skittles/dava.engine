#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Collision/Collisions.h"
#include "Render/2D/Systems/RenderSystem2D.h"

namespace DAVA
{
void Collisions::ProjectPolygon(const Vector2& axis, Polygon2& poly, float32& min, float32& max)
{
    Vector2* points = poly.GetPoints();
    float32 proj = DotProduct(points[0], axis);
    min = proj;
    max = proj;
    for (int32 pi = 1; pi < poly.GetPointCount(); ++pi)
    {
        proj = DotProduct(points[pi], axis);
        if (proj < min)
            min = proj;
        if (proj > max)
            max = proj;
    }
}

float32 Collisions::IntervalDistance(float32 minA, float32 maxA, float32 minB, float32 maxB)
{
    if (minA < minB)
    {
        return minB - maxA;
    }
    else
    {
        return minA - maxB;
    }
}

void Collisions::AddSeparationAxis(const Vector2& axis)
{
    size_t size = separationAxes.size();
    for (size_t index = 0; index < size; ++index)
    {
        float32 cross = Abs(CrossProduct(axis, separationAxes[index]));
        if (cross < EPSILON)
            return;
    }
    // Logger::FrameworkDebug("new axis: %f %f", axis.x, axis.y);
    separationAxes.push_back(axis);
}

/*
 
 */
class Transform2
{
    Vector2 translate;
    Matrix2 rotate;
    Vector2 scale;
};

bool Collisions::IsPolygonIntersectsPolygon(Polygon2& poly1, Polygon2& poly2)
{
    //#define DEBUG_DRAW_INTERSECTIONS

    Vector2* points1 = poly1.GetPoints();
    Vector2* points2 = poly2.GetPoints();

    separationAxes.clear();

    for (int32 index1 = 0; index1 < poly1.pointCount; ++index1)
    {
        int32 index2 = (index1 + 1 != poly1.pointCount) ? (index1 + 1) : (0);

        Vector2 line = points1[index2] - points1[index1];
        Vector2 normal = Vector2(line.y, -line.x);
        normal.Normalize();

        AddSeparationAxis(normal);

#if defined(DEBUG_DRAW_INTERSECTIONS)
        RenderSystem2D::Instance()->DrawLine(points1[index1] + (line / 2), points1[index1] + (line / 2) + normal * 10, Color(0.0f, 0.0f, 1.0f, 1.0f));
#endif
    }

    for (int32 index1 = 0; index1 < poly2.pointCount; ++index1)
    {
        int32 index2 = (index1 + 1 != poly2.pointCount) ? (index1 + 1) : (0);

        Vector2 line = points2[index2] - points2[index1];
        Vector2 normal = Vector2(line.y, -line.x);
        normal.Normalize();
        AddSeparationAxis(normal);
		
#if defined(DEBUG_DRAW_INTERSECTIONS)
        RenderSystem2D::Instance()->DrawLine(points2[index1] + (line / 3), points2[index1] + (line / 3) + normal * 10, Color(0.0f, 1.0f, 0.0f, 1.0f));
#endif
    }

    size_t size = separationAxes.size();
#if defined(DEBUG_DRAW_INTERSECTIONS)
    for (size_t index = 0; index < size; ++index)
    {
        Vector2 axis = separationAxes[index];
        RenderSystem2D::Instance()->DrawLine(Vector2(50.0f, 50.0f), Vector2(50.0f, 50.0f) + axis * 1000, Color(1.0f, 0.0f, 0.0f, 1.0f));
    }
#endif

    for (size_t index = 0; index < size; ++index)
    {
        Vector2 axis = separationAxes[index];

        float32 p1Min, p1Max;
        ProjectPolygon(axis, poly1, p1Min, p1Max);

        float32 p2Min, p2Max;
        ProjectPolygon(axis, poly2, p2Min, p2Max);

#if defined(DEBUG_DRAW_INTERSECTIONS)
        Vector2 norm = Vector2(axis.y, -axis.x);
        RenderSystem2D::Instance()->DrawLine(Vector2(50.0f, 50.0f) + axis * p1Min + norm * 2.0f, Vector2(50.0f, 50.0f) + axis * p1Max + norm * 2.0f, Color(0.0f, 1.0f, 1.0f, 1.0f));
        RenderSystem2D::Instance()->DrawLine(Vector2(50.0f, 50.0f) + axis * p2Min - norm * 2.0f, Vector2(50.0f, 50.0f) + axis * p2Max - norm * 2.0f, Color(1.0f, 1.0f, 0.0f, 1.0f));
#endif

        if (IntervalDistance(p1Min, p1Max, p2Min, p2Max) > 0)
            return false;
    }

    return true;
}

void Collisions::FindIntersectionPolygonToPolygon(Polygon2& poly1,
                                                  Polygon2& poly2,
                                                  ContactManifold2& manifold)
{
    /*
		Line equation
		Line(x, y) = N * P (x, y) + D; 
	 */

    manifold.count = 0;
    for (int32 p1Index1 = 0; p1Index1 < poly1.pointCount; ++p1Index1)
    {
        int32 p1Index2 = (p1Index1 + 1 != poly1.pointCount) ? (p1Index1 + 1) : (0);

        for (int32 p2Index1 = 0; p2Index1 < poly2.pointCount; ++p2Index1)
        {
            int32 p2Index2 = (p2Index1 + 1 != poly2.pointCount) ? (p2Index1 + 1) : (0);

            Vector2 intersection;

            if (Collisions::Instance()->IsSegmentIntersectsSegment(poly1.points[p1Index1], poly1.points[p1Index2],
                                                                   poly2.points[p2Index1], poly2.points[p2Index2],
                                                                   intersection))
            {
                manifold.contactPoints[manifold.count] = intersection;
                manifold.count++;
                if (manifold.count == MAX_CONTACT_POINTS)
                    return;
            }
        }
    }
}

void Collisions::FindIntersectionPolygonToCircle(const Polygon2& poly1, const Circle& circle, ContactManifold2& manifold)
{
    manifold.count = 0;
    for (int32 p1Index1 = 0; p1Index1 < poly1.pointCount; ++p1Index1)
    {
        int32 p1Index2 = (p1Index1 + 1 != poly1.pointCount) ? (p1Index1 + 1) : (0);

        ContactManifold2 tempManifold;
        Collisions::Instance()->FindIntersectionLineToCircle(poly1.points[p1Index1], poly1.points[p1Index2],
                                                             circle,
                                                             tempManifold);
        if (tempManifold.count != 0)
        {
            manifold.MergeWithManifold(tempManifold);
        }
    }
}
};