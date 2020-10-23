#pragma once

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "Math/Rect.h"

namespace DAVA
{
struct Matrix3;
class Polygon2;

class UIGeometricData
{
    friend class UIControl;

public:
    Vector2 position;
    Vector2 size;

    Vector2 pivotPoint;
    Vector2 scale = Vector2(1.0f, 1.0f);
    float32 angle = 0.0f;

    mutable float32 cosA = 1.0f;
    mutable float32 sinA = 0.0f;

    void AddGeometricData(const UIGeometricData& data);

    DAVA_DEPRECATED(void AddToGeometricData(const UIGeometricData& data))
    {
        AddGeometricData(data);
    }

    void BuildTransformMatrix(Matrix3& transformMatr) const;

    void GetPolygon(Polygon2& polygon) const;

    const Rect& GetUnrotatedRect() const;

    Rect GetAABBox() const;

private:
    mutable float32 calculatedAngle = 0.0f;
    Rect unrotatedRect;
};
};