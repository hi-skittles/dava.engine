#include "EditorSystems/UIControlUtils.h"
#include "Render/2D/Sprite.h"
#include "UI/UIControlBackground.h"

namespace UIControlUtilsDetails
{
void FixSize(DAVA::Vector2& size)
{
    size.dx = std::max(size.dx, 1.0f);
    size.dy = std::max(size.dy, 1.0f);
}
}

DAVA::RefPtr<DAVA::UIControl> UIControlUtils::CreateLineWithColor(const DAVA::Color& color, const DAVA::String& name)
{
    using namespace DAVA;

    RefPtr<UIControl> lineControl(new UIControl());
    lineControl->SetName(name);
    UIControlBackground* background = lineControl->GetOrCreateComponent<UIControlBackground>();
    background->SetPerPixelAccuracyType(UIControlBackground::PER_PIXEL_ACCURACY_ENABLED);
    background->SetDrawType(UIControlBackground::DRAW_FILL);
    background->SetColor(color);
    return lineControl;
}

DAVA::RefPtr<DAVA::UIControl> UIControlUtils::CreateLineWithTexture(const DAVA::FilePath& texture, const DAVA::String& name)
{
    using namespace DAVA;
    RefPtr<UIControl> lineControl(new UIControl());
    lineControl->SetName(name);
    UIControlBackground* background = lineControl->GetOrCreateComponent<UIControlBackground>();
    background->SetPerPixelAccuracyType(UIControlBackground::PER_PIXEL_ACCURACY_ENABLED);
    ScopedPtr<Sprite> sprite(Sprite::CreateFromSourceFile(texture));
    background->SetSprite(sprite, 0);
    background->SetDrawType(UIControlBackground::DRAW_TILED);
    return lineControl;
}

void UIControlUtils::MapLineToScreen(DAVA::Vector2::eAxis axis, const DAVA::Rect& localRect, const DAVA::UIGeometricData& gd, const DAVA::RefPtr<DAVA::UIControl>& control)
{
    using namespace DAVA;

    Vector2 pos = localRect.GetPosition();
    Vector2 size = localRect.GetSize();

    pos = Rotate(pos, gd.angle);
    pos *= gd.scale;
    size[axis] *= gd.scale[axis];
    Vector2 gdPos = gd.position - Rotate(gd.pivotPoint * gd.scale, gd.angle);

    UIControlUtilsDetails::FixSize(size);

    Rect lineRect(Vector2(pos + gdPos), size);
    control->SetRect(lineRect);
    control->SetAngle(gd.angle);
}
