#pragma once

#include "Analytics/Analytics.h"
#include "UI/UIControl.h"

namespace DAVA
{
namespace Analytics
{
const char UI_EVENT_TYPE_TAG[] = "UIEventType";

const char CLICK_EVENT[] = "Click";
const char DOUBLE_CLICK_EVENT[] = "DoubleClick";
const char KEY_PRESS_EVENT[] = "KeyPress";

const char PRESSED_KEY_TAG[] = "PressedKey";
const char ESC_KEY_PRESSED[] = "EscKeyPressed";
const char BACK_KEY_PRESSED[] = "BackKeyPressed";
const char ENTER_KEY_PRESSED[] = "EnterKeyPressed";

bool EmitUIEvent(UIControl* control, UIControl::eEventType eventType, UIEvent* uiEvent);
bool EmitKeyEvent(UIControl* control, UIEvent* uiEvent);
bool IsUIEvent(const AnalyticsEvent& record);
String GetUIControlName(UIControl* uiControl);

} // namespace Analytics
} // namespace DAVA