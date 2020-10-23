#include "UIGeometricData.h"
#include "Math/AABBox2.h"
#include "Math/Polygon2.h"
#include "Math/Matrix3.h"

namespace DAVA
{
void UIGeometricData::AddGeometricData(const UIGeometricData& data)
{
    position.x = data.position.x - data.pivotPoint.x * data.scale.x + position.x * data.scale.x;
    position.y = data.position.y - data.pivotPoint.y * data.scale.y + position.y * data.scale.y;
    if (data.angle != 0)
    {
        float32 tmpX = position.x;
        position.x = (tmpX - data.position.x) * data.cosA + (data.position.y - position.y) * data.sinA + data.position.x;
        position.y = (tmpX - data.position.x) * data.sinA + (position.y - data.position.y) * data.cosA + data.position.y;
    }
    scale.x *= data.scale.x;
    scale.y *= data.scale.y;
    angle += data.angle;
    if (angle != calculatedAngle)
    {
        if (angle != data.angle)
        {
            cosA = std::cos(angle);
            sinA = std::sin(angle);
        }
        else
        {
            cosA = data.cosA;
            sinA = data.sinA;
        }
        calculatedAngle = angle;
    }

    unrotatedRect.x = position.x - pivotPoint.x * scale.x;
    unrotatedRect.y = position.y - pivotPoint.y * scale.y;
    unrotatedRect.dx = size.x * scale.x;
    unrotatedRect.dy = size.y * scale.y;
}

void UIGeometricData::BuildTransformMatrix(Matrix3& transformMatr) const
{
    Matrix3 pivotMatr;
    pivotMatr.BuildTranslation(-pivotPoint);

    Matrix3 translateMatr;
    translateMatr.BuildTranslation(position);
    // well it must be here otherwise there is a bug!
    if (calculatedAngle != angle)
    {
        cosA = std::cos(angle);
        sinA = std::sin(angle);
        calculatedAngle = angle;
    }
    Matrix3 rotateMatr;
    rotateMatr.BuildRotation(cosA, sinA);

    Matrix3 scaleMatr;
    scaleMatr.BuildScale(scale);

    transformMatr = pivotMatr * scaleMatr * rotateMatr * translateMatr;
}

void UIGeometricData::GetPolygon(Polygon2& polygon) const
{
    polygon.Clear();
    polygon.points.reserve(4);
    polygon.AddPoint(Vector2());
    polygon.AddPoint(Vector2(size.x, 0));
    polygon.AddPoint(size);
    polygon.AddPoint(Vector2(0, size.y));

    Matrix3 transformMtx;
    BuildTransformMatrix(transformMtx);
    polygon.Transform(transformMtx);
}

const DAVA::Rect& UIGeometricData::GetUnrotatedRect() const
{
    return unrotatedRect;
}

Rect UIGeometricData::GetAABBox() const
{
    Polygon2 polygon;
    GetPolygon(polygon);

    AABBox2 aabbox;
    for (int32 i = 0; i < polygon.GetPointCount(); ++i)
    {
        aabbox.AddPoint(polygon.GetPoints()[i]);
    }
    Rect bboxRect = Rect(aabbox.min, aabbox.max - aabbox.min);
    return bboxRect;
}
}