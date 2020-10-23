#pragma once

#include <Math/Color.h>
#include <Math/Vector.h>
#include <Math/Rect.h>
#include <UI/UIControl.h>
#include <Base/BaseTypes.h>
#include <Render/2D/Systems/RenderSystem2D.h>

enum SectorColor
{
    Green,
    Yellow,
    Red
};

DAVA::Color GetColorByType(SectorColor type);

class Sector : public DAVA::UIControl
{
public:
    Sector(const DAVA::Vector2& centerPoint, DAVA::float32 startAngle, DAVA::float32 endAngle, DAVA::float32 radius, SectorColor type);

    enum Mode
    {
        SELECTED,
        UNSELECTED
    };
    void SetMode(Mode);

private:
    // UIControl
    void Draw(const DAVA::UIGeometricData& geometricData) override;
    bool IsPointInside(const DAVA::Vector2& point, bool expandWithFocus = false) const override;

    DAVA::Polygon2 polygon;
    DAVA::Color fillColorPrimary;
    DAVA::Color fillColorInverted;
    DAVA::Color fillColor;
    DAVA::Color borderColor;
    Mode mode = UNSELECTED;
};

class SectorColorBox : public DAVA::UIControl
{
public:
    SectorColorBox(const DAVA::Rect& box, SectorColor type);

private:
    void Draw(const DAVA::UIGeometricData& geometricData) override;

    DAVA::Rect box;
    DAVA::Color fillColor;
    DAVA::Color borderColor;
};
