#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Collision/Collisions.h"
#include "Render/2D/Systems/RenderSystem2D.h"

namespace DAVA
{
//! Check if point lays inside polygon (polygon must be in one plane)
bool Polygon2::IsPointInside(const Vector2& pt) const
{
    // Do not check if this is not a polygon
    if (pointCount <= 2)
        return false;

    int intersectionsCount = 0;
    Vector2 ray0(pt.x, pt.y);
    Vector2 ray1(((points[0].x + points[1].x) / 2.0f) + 100000.0f, ((points[0].y + points[1].y) / 2.0f) + 100000.0f);

/*
	If you enabling debug intersections do not forget to call this function inside Draw of your application.
	If you'll call it inside update or mouse or keyboard input functions you can not get any output.
*/
//#define DEBUG_DRAW_INTERSECTIONS

#if defined(DEBUG_DRAW_INTERSECTIONS)
    RenderSystem2D::Instance()->DrawLine(ray0, ray1, Color(0.0f, 1.0f, 0.0f, 1.0f));
#endif

    for (int i = 0; i < pointCount; ++i)
    {
        int32 i2 = (i == pointCount - 1) ? (0) : (i + 1);
        Vector2 pt1(points[i].x, points[i].y);
        Vector2 pt2(points[i2].x, points[i2].y);

        Vector2 result;
        if (Collisions::Instance()->IsSegmentIntersectsSegment(pt1, pt2, ray0, ray1, result))
        {
			
#if defined(DEBUG_DRAW_INTERSECTIONS)
            RenderSystem2D::Instance()->DrawCircle(result, 5.0f, Color(1.0f, 1.0f, 1.0f, 1.0f));
#endif

            intersectionsCount++;
        }
    }
	
#undef DEBUG_DRAW_INTERSECTIONS

    return intersectionsCount & 1;
}

//! Remove segment with index
void Polygon2::RemoveSegmentAtIndex(int index)
{
    for (int k = index + 1; k < pointCount; ++k)
        points[k - 1] = points[k];
    points.pop_back();

    pointCount--;
}

//! Remove small segments function remove all segments size of which smaller or equal than removalSize
int Polygon2::RemoveSmallSegments(float32 removalSize)
{
    float32 removalSquareSize = removalSize * removalSize;
    int removedCount = 0;
    for (int segment = 0; segment < pointCount - 1; ++segment)
    {
        Vector2 v = points[segment] - points[segment + 1];
        if (v.SquareLength() <= removalSquareSize)
        {
            RemoveSegmentAtIndex(segment);
            segment--;
            removedCount++;
        }
    }
    return removedCount;
}

//! Move by polyline and get position on polyline based on distance
void Polygon2::InterpolatePositionFromDistance(float32 distance, int startSegment, Vector2& resultPosition, int& resultSegmentIndex) const
{
    float32 currentDistance = distance;

    int currentSegment = 0;
    float currentSegmentLenght = 0.0f;
    for (currentSegment = startSegment; currentSegment < pointCount - 1; ++currentSegment)
    {
        Vector2 v = points[currentSegment] - points[currentSegment + 1];
        currentSegmentLenght = v.Length();
        if (currentDistance - currentSegmentLenght < 0.0f)
            break; // we've found right segment
        currentDistance -= currentSegmentLenght;
    }

    resultSegmentIndex = -1;
    if (currentSegment == pointCount - 1)
    {
        return;
    }
    float t = currentDistance / currentSegmentLenght;
    if ((t >= 0.0f) && (t <= 1.0f))
    {
        resultPosition.Lerp(points[currentSegment], points[currentSegment + 1], t);
        resultSegmentIndex = currentSegment;
    }
}

//! Move by polyline and get position on polyline based on distance
void Polygon2::InterpolatePositionFromDistanceReverse(float32 distance, int startSegment, Vector2& resultPosition, int& resultSegmentIndex) const
{
    float32 currentDistance = distance;

    int currentSegment = 0;
    float currentSegmentLenght = 0.0f;
    for (currentSegment = startSegment; currentSegment >= 1; --currentSegment)
    {
        Vector2 v = points[currentSegment] - points[currentSegment - 1];
        currentSegmentLenght = v.Length();
        if (currentDistance - currentSegmentLenght < 0.0f)
            break; // we've found right segment
        currentDistance -= currentSegmentLenght;
    }

    resultSegmentIndex = -1;
    if (currentSegment == 0)
    {
        return;
    }
    float t = currentDistance / currentSegmentLenght;
    if ((t >= 0.0f) && (t <= 1.0f))
    {
        resultPosition.Lerp(points[currentSegment], points[currentSegment - 1], t);
        resultSegmentIndex = currentSegment;
    }
}

//! Remove points in given radius
void Polygon2::RemovePointsInCircle(const Vector2& center, float32 radius)
{
    float32 rad2 = radius * radius;
    for (int p = 0; p < pointCount; ++p)
    {
        Vector2 dist = points[p] - center;
        if (dist.SquareLength() <= rad2)
        {
            RemoveSegmentAtIndex(p);
            p--;
        }
    }
}

//! Square length of poly segments
float32 Polygon2::SquareLength() const
{
    float32 squareLength = 0.0f;
    for (int32 i = 0; i < pointCount - 1; ++i)
    {
        Vector2 segment = points[i] - points[i + 1];
        squareLength += segment.SquareLength();
    }
    return squareLength;
}

//! Length
float32 Polygon2::Length() const
{
    float32 length = 0.0f;
    for (int32 i = 0; i < pointCount - 1; ++i)
    {
        Vector2 segment = points[i] - points[i + 1];
        length += segment.Length();
    }
    return length;
}

//! Scale whole polygon around given center point
void Polygon2::Scale(const Vector2& centerPoint, float32 scale)
{
    for (int p = 0; p < pointCount; ++p)
    {
        points[p] -= centerPoint;
        points[p] *= scale;
        points[p] += centerPoint;
    }
}

void Polygon2::Translate(const Vector2& translatePos)
{
    for (int p = 0; p < pointCount; ++p)
    {
        points[p] += translatePos;
    }
}

//! Transform polygon using matrix
void Polygon2::Transform(const Matrix3& matrix)
{
    for (int p = 0; p < pointCount; ++p)
    {
        points[p] = points[p] * matrix;
    }
}

void Polygon2::CalculateCenterPoint(Vector2& center) const
{
    center.Set(0.0f, 0.0f);
    for (int p = 0; p < pointCount; ++p)
    {
        center += points[p];
    }
    center /= static_cast<float32>(pointCount);
}

void Polygon2::CalculateSizeRect(Vector2& size) const
{
    Vector2 min(points[0]);
    size = min;
    for (int p = 1; p < pointCount; ++p)
    {
        const Vector2& current = points[p];
        if (current.x > size.x)
            size.x = current.x;
        if (current.y > size.y)
            size.y = current.y;
        if (current.x < min.x)
            min.x = current.x;
        if (current.y < min.y)
            min.y = current.y;
    }
    size -= min;
}

float32 Polygon2::CalculateSquareRadius(const Vector2& center) const
{
    float32 radius = 0.0f;
    for (int p = 0; p < pointCount; ++p)
    {
        float32 dist = (points[p] - center).SquareLength();
        if (dist > radius)
            radius = dist;
    }
    return radius;
}

//! Merge all segments triangle's height of which bigger or equal than minTriangleHeight
// http://www.math.ru/dic/276
void Polygon2::MergeFlatPolygonSegments(const Polygon2& srcPolygon, Polygon2& destPolygon, float32 minTriangleHeight)
{
    float mh2 = minTriangleHeight * minTriangleHeight;

    destPolygon.Clear();

    for (int i0 = 0; i0 < srcPolygon.pointCount - 2; ++i0)
    {
        destPolygon.AddPoint(srcPolygon.points[i0]);

        Vector2 v0 = srcPolygon.points[i0];

        for (int i2 = i0 + 2; i2 < srcPolygon.pointCount; ++i2)
        {
            Vector2 v2 = srcPolygon.points[i2];
            float c = (v2 - v0).Length();
            float hc2 = 0.0f;

            for (int i1 = i0 + 1; i1 < i2; ++i1)
            {
                Vector2 v1 = srcPolygon.points[i1];
                float a = (v1 - v0).Length();
                float b = (v2 - v1).Length();
                float p = (a + b + c) / 2.0f;
                hc2 = 4.0f * p * (p - a) * (p - b) * (p - c) / (c * c);

                if (hc2 >= mh2)
                {
                    break;
                }
            }

            if (hc2 >= mh2)
            {
                i0 = i2 - 1 - 1;
                break;
            }
        }
    }

    destPolygon.AddPoint(srcPolygon.points.back());
}
};
