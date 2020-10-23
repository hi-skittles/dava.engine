#pragma once
#include "Base/BaseTypes.h"

namespace DAVA
{
class UIScrollBar;
class UIScrollBarDelegate
{
public:
    virtual ~UIScrollBarDelegate() = default;

    virtual float32 VisibleAreaSize(UIScrollBar* forScrollBar) = 0;
    virtual float32 TotalAreaSize(UIScrollBar* forScrollBar) = 0;
    virtual float32 ViewPosition(UIScrollBar* forScrollBar) = 0;
    virtual void OnViewPositionChanged(UIScrollBar* byScrollBar, float32 newPosition) = 0;
};
};
