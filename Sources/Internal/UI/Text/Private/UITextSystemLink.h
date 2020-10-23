#pragma once

#include "Base/BaseObject.h"
#include "Base/RefPtr.h"

namespace DAVA
{
class UIControl;
class UITextComponent;
class TextBlock;
class UIControlBackground;

/**
    Internal helper class for text parsing and rendering purpose.    
    Stores text and shadow renderers.
    \sa {UITextComponent, UITextSystem}. 
*/
class UITextSystemLink final
{
public:
    UITextSystemLink();
    ~UITextSystemLink();

    /** Text parser and renderer. */
    TextBlock* GetTextBlock() const;
    /** Text layer representation for render. */
    UIControlBackground* GetTextBackground() const;
    /** Text shadow layer representation for render. */
    UIControlBackground* GetShadowBackground() const;

private:
    UITextSystemLink& operator=(const UITextSystemLink&) = delete;

    RefPtr<TextBlock> textBlock;
    RefPtr<UIControlBackground> textBg;
    RefPtr<UIControlBackground> shadowBg;
};

inline TextBlock* UITextSystemLink::GetTextBlock() const
{
    return textBlock.Get();
}

inline UIControlBackground* UITextSystemLink::GetTextBackground() const
{
    return textBg.Get();
}

inline UIControlBackground* UITextSystemLink::GetShadowBackground() const
{
    return shadowBg.Get();
}
}