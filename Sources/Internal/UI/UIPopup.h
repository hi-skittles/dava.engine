#ifndef __DAVAENGINE_UI_POPUP__
#define __DAVAENGINE_UI_POPUP__

#include "Base/BaseTypes.h"
#include "UI/UIScreen.h"

namespace DAVA
{
class UIPopup : public UIScreen
{
public:
    UIPopup(const Rect& rect);
    bool isTransparent;

    virtual void Show();
    virtual void Hide();
};
};

#endif