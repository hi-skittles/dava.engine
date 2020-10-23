#pragma once

#include <Base/RefPtr.h>
#include <UI/UIControl.h>
#include <Math/Color.h>

namespace UIControlUtils
{
DAVA::RefPtr<DAVA::UIControl> CreateLineWithColor(const DAVA::Color& color, const DAVA::String& name);
DAVA::RefPtr<DAVA::UIControl> CreateLineWithTexture(const DAVA::FilePath& texture, const DAVA::String& name);
void MapLineToScreen(DAVA::Vector2::eAxis axis, const DAVA::Rect& localRect, const DAVA::UIGeometricData& gd, const DAVA::RefPtr<DAVA::UIControl>& control);
}
