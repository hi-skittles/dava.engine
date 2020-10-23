#include "Sector.h"

#include <Debug/DVAssert.h>
#include <Utils/StringFormat.h>
#include <Math/Rect.h>

namespace SectorDetails
{
using namespace DAVA;

const DAVA::float32 CIRCLE_STEP_ANGLE = 5.f;

Polygon2 GenerateSectorPolygon(Vector2 center, float32 startAngle, float32 finalAngle, float32 radius)
{
    DVASSERT(startAngle < finalAngle);
    DVASSERT(radius > 0.f);

    auto PointAtCircle = [&center, &radius](float32 angle) -> Vector2
    {
        float32 sine, cos;
        SinCosFast(DegToRad(angle), sine, cos);

        Vector2 point;
        point.x = center.x + radius * cos;
        point.y = center.y - radius * sine;

        return point;
    };

    Polygon2 polygon;

    polygon.AddPoint(center);
    for (float32 angle = startAngle; angle < finalAngle; angle += CIRCLE_STEP_ANGLE)
    {
        polygon.AddPoint(PointAtCircle(angle));
    }
    polygon.AddPoint(PointAtCircle(finalAngle));

    return polygon;
}

const DAVA::Color green = DAVA::Color(0.f, 1.f, 0.f, 0.7f);
const DAVA::Color yellow = DAVA::Color(1.f, 1.f, 0.f, 0.7f);
const DAVA::Color red = DAVA::Color(1.f, 0.f, 0.f, 0.7f);

DAVA::Color Invert(const DAVA::Color& color)
{
    return DAVA::Color(1.f - color.r, 1.f - color.g, 1.f - color.b, color.a);
}
}

DAVA::Color GetColorByType(SectorColor type)
{
    switch (type)
    {
    case SectorColor::Green:
        return SectorDetails::green;
    case SectorColor::Yellow:
        return SectorDetails::yellow;
    case SectorColor::Red:
        return SectorDetails::red;
    default:
        DVASSERT(false, DAVA::Format("undefined sector type: %u", type).c_str());
        return DAVA::Color();
    }
}

Sector::Sector(const DAVA::Vector2& centerPoint, DAVA::float32 startAngle, DAVA::float32 endAngle, DAVA::float32 radius, SectorColor type)
    : UIControl(DAVA::Rect(centerPoint.x, centerPoint.y, radius, radius))
{
    polygon = SectorDetails::GenerateSectorPolygon(centerPoint, startAngle, endAngle, radius);
    fillColorPrimary = GetColorByType(type);
    fillColorInverted = SectorDetails::Invert(fillColorPrimary);
    borderColor = fillColorPrimary;
    borderColor.a += 0.2f;
    SetMode(mode);
}

void Sector::SetMode(Sector::Mode newMode)
{
    mode = newMode;
    switch (mode)
    {
    case SELECTED:
    {
        fillColor = fillColorInverted;
        break;
    }
    case UNSELECTED:
    default:
    {
        fillColor = fillColorPrimary;
        break;
    }
    }
}

void Sector::Draw(const DAVA::UIGeometricData& geometricData)
{
    DAVA::RenderSystem2D::Instance()->DrawPolygon(polygon, true, borderColor);
    DAVA::RenderSystem2D::Instance()->FillPolygon(polygon, fillColor);
}

bool Sector::IsPointInside(const DAVA::Vector2& point, bool) const
{
    return polygon.IsPointInside(point);
}

SectorColorBox::SectorColorBox(const DAVA::Rect& box, SectorColor type)
    : box(box)
{
    fillColor = GetColorByType(type);
    borderColor = fillColor;
    borderColor.a += 0.2f;
}

void SectorColorBox::Draw(const DAVA::UIGeometricData& geometricData)
{
    DAVA::RenderSystem2D::Instance()->DrawRect(box, borderColor);
    DAVA::RenderSystem2D::Instance()->FillRect(box, fillColor);
}
