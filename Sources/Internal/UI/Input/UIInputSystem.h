#pragma once 

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Math/Vector.h"
#include "UI/Events/UIActionMap.h"
#include "UI/Events/UIInputMap.h"
#include "Functional/Signal.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIControl;
class UIScreen;
class UIEvent;
class UIFocusSystem;

class UIInputSystem
: public UISystem
{
public:
    UIInputSystem();
    ~UIInputSystem() override;

    void Process(DAVA::float32 elapsedTime) override{};

    void OnControlVisible(UIControl* control) override;
    void OnControlInvisible(UIControl* control) override;

    void SetCurrentScreen(UIScreen* screen);
    void SetPopupContainer(UIControl* popupContainer);

    void HandleEvent(UIEvent* event);

    void CancelInput(UIEvent* touch);
    void CancelAllInputs();
    void CancelInputs(UIControl* control, bool hierarchical);
    void SwitchInputToControl(uint32 eventID, UIControl* targetControl);

    const Vector<UIEvent>& GetAllInputs() const;
    bool IsAnyInputLockedByControl(const UIControl* control) const;

    void SetExclusiveInputLocker(UIControl* locker, uint32 lockEventId);
    UIControl* GetExclusiveInputLocker() const;
    void SetHoveredControl(UIControl* newHovered);
    UIControl* GetHoveredControl() const;
    UIControl* GetModalControl() const;

    UIFocusSystem* GetFocusSystem() const;

    void MoveFocusLeft(const Any& data);
    void MoveFocusRight(const Any& data);
    void MoveFocusUp(const Any& data);
    void MoveFocusDown(const Any& data);

    void MoveFocusForward(const Any& data);
    void MoveFocusBackward(const Any& data);

    /** \deprecated use UIEventsSystem method instead \sa UIEventsSystem */
    DAVA_DEPRECATED(void BindGlobalShortcut(const KeyboardShortcut& shortcut, const FastName& eventName));
    /** \deprecated use UIEventsSystem method instead \sa UIEventsSystem */
    DAVA_DEPRECATED(void BindGlobalAction(const FastName& eventName, const UIActionMap::SimpleAction& action));

    static const FastName ACTION_FOCUS_LEFT;
    static const FastName ACTION_FOCUS_RIGHT;
    static const FastName ACTION_FOCUS_UP;
    static const FastName ACTION_FOCUS_DOWN;

    static const FastName ACTION_FOCUS_NEXT;
    static const FastName ACTION_FOCUS_PREV;

    static const FastName ACTION_PERFORM;
    static const FastName ACTION_ESCAPE;

    DAVA::Signal<UIEvent*> notProcessedEventSignal;

private:
    bool HandleTouchEvent(UIEvent* event);
    bool HandleKeyEvent(UIEvent* event);
    bool HandleOtherEvent(UIEvent* event);

    void UpdateModalControl();
    void CancelInputForAllOutsideChildren(UIControl* root);

    UIControl* FindNearestToUserModalControl() const;
    UIControl* FindNearestToUserModalControlImpl(UIControl* current) const;

    UIScreen* currentScreen = nullptr;
    UIControl* popupContainer = nullptr;
    RefPtr<UIControl> modalControl;

    UIFocusSystem* focusSystem = nullptr;

    RefPtr<UIControl> hovered;

    Vector<UIEvent> touchEvents;
    UIControl* focusedControlWhenTouchBegan = nullptr;
    Vector2 positionOfTouchWhenTouchBegan;
    RefPtr<UIControl> exclusiveInputLocker;
};
}
