#pragma once

#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Base/RefPtr.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Input/InputElements.h"
#include "UI/Components/UISingleComponent.h"
#include "UI/UIEvent.h"

#define FRAME_SKIP 5

/**
	\defgroup controlsystem	UI System
*/
namespace DAVA
{
class Mouse;
class ScreenSwitchListener;
class UIComponent;
class UIControl;
class UIFocusSystem;
class UIInputSystem;
class UILayoutSystem;
class UIPopup;
class UIRenderSystem;
class UIScreen;
class UIScreenTransition;
class UIScreenshoter;
class UISoundSystem;
class UIStyleSheetSystem;
class UISystem;
class UITextSystem;
class UIUpdateSystem;
class UIEventsSystem;
class VirtualCoordinatesSystem;
struct InputEvent;

/**
	 \brief	UIControlSystem it's a core of the all controls work.
		ControlSystem managed all update, draw, appearence and disappearence of the controls.
		ControlSystem works with th UIScreenManager to process screen setting and switching.
		Also ControlSystem processed all user input events to the controls.
	 */

class UIControlSystem final
{
public:
    /**
	 \brief Sets the requested screen as current.
		Screen will be seted only on the next frame.
		Previous seted screen will be removed.
	 \param[in] Screen you want to set as current
	 \param[in] Transition you want to use for the screen setting.
	 */
    void SetScreen(UIScreen* newMainControl);

    /**
	 \brief Sets the requested screen as current.
	 \returns currently seted screen
	 */
    UIScreen* GetScreen() const;

    /**
	 \brief Adds new popup to the popup container.
	 \param[in] Popup control to add.
	 */
    void AddPopup(UIPopup* newPopup);

    /**
	 \brief Removes popup from the popup container.
	 \param[in] Popup control to remove.
	 */
    void RemovePopup(UIPopup* newPopup);

    /**
	 \brief Removes all popups from the popup container.
	 */
    void RemoveAllPopups();

    /**
	 \brief Returns popups container.
		User can manage this container manually (change popup sequence, removes or adds popups)
	 \returns popup container
	 */
    UIControl* GetPopupContainer() const;

    /**
	 \brief Disabled all controls inputs.
		Locking all inputs if input is unlocked or incrementing lock counter.
	 \returns current lock input counter
	 */
    int32 LockInput();

    /**
	 \brief Enabling all controls inputs.
	 Decrementing lock counter if counter is zero unlocking all inputs.
	 \returns current lock input counter
	 */
    int32 UnlockInput();

    /**
	 \brief Returns lock input counter.
	 \returns current lock input counter
	 */
    int32 GetLockInputCounter() const;

    /**
	 \brief Cancel all inputs for the requested control.
	 \param[in] control to cancel inputs for.
	 */
    void CancelInputs(UIControl* control, bool hierarchical = true);

    /**
	 \brief Cancel requested input.
	 \param[in] event to cancel.
	 */
    void CancelInput(UIEvent* touch);

    /**
	 \brief Cancelling all current inputs.
	 */
    void CancelAllInputs();

    /**
	 \brief Sets the current screen to 0 LOL.
	 */
    void Reset();

    bool HandleInputEvent(const InputEvent& inputEvent);

    /**
	 \brief Calls by the system for input processing.
	 */
    void OnInput(UIEvent* newEvent);

    /**
	 \brief Calls very frame by the system for update.
	 */
    void Update();

    /**
	 \brief Update system with custom time elapsed value.
	 */
    void UpdateWithCustomTime(float32 timeElapsed);

    /**
     \brief Calls update logic for specific control. Used to make screenshoots.
        Not recommended to use in common code.
     */
    void ForceUpdateControl(float32 timeElapsed, UIControl* control);

    /**
	 \brief Calls every frame by the system for draw.
		Draws all controls hierarchy to the screen.
	 */
    void Draw();

    /**
     \brief Calls draw logic for specific control. Used to make screenshoots.
        Not recommended to use in common code.
     */
    void ForceDrawControl(UIControl* control);

    //	void SetTransitionType(int newTransitionType);

    /**
	 \brief Returns all currently active inputs.
	 \returns all inputs active in the system
	 */
    const Vector<UIEvent>& GetAllInputs() const;

    /**
	 \brief Sets requested control as a exclusive input locker.
	 All inputs goes only to the exclusive input locker if input locker is present.
	 \param[in] control to set the input locker.
	 \param[in] event id to cause a lock. All other events will be cancelled(excepts the locker == NULL situation).
	 */
    void SetExclusiveInputLocker(UIControl* locker, uint32 lockEventId);

    /**
	 \brief Returns current exclusive input locker. Returns NULL if exclusive input locker is not present.
	 \returns exclusive input locker
	 */
    UIControl* GetExclusiveInputLocker() const;

    /**
	 \brief Sets input with the requested ID to the required control.
		Input removes from the current owner. OnInputCancel() calls for the old control.  
		New control starts to handle all input activities.
	 \param[in] Input ID. Can be found in the UIEvent:touchId.
	 \param[in] Control that should handle the input.
	 */
    void SwitchInputToControl(uint32 eventID, UIControl* targetControl);

    /**
	 \brief Used internally by Replay class
	 */
    void ReplayEvents();

    /**
	 \brief Called by the core when screen size is changed
	 */
    void ScreenSizeChanged(const Rect& newFullscreenRect);

    /**
	 \brief Called by the control to set himself as the hovered control
	 */
    void SetHoveredControl(UIControl* newHovered);

    /**
	 \brief Returns control hovered by the mnouse for now
	 */
    UIControl* GetHoveredControl() const;

    /**
	 \brief Called by the control to set himself as the focused control
	 */
    void SetFocusedControl(UIControl* newFocused);

    /**
	 \brief Returns currently focused control
	 */
    UIControl* GetFocusedControl() const;

    /*
     \brief Called by the control himself
     */
    void ProcessControlEvent(int32 eventType, const UIEvent* uiEvent, UIControl* control);

    void AddScreenSwitchListener(ScreenSwitchListener* listener);
    void RemoveScreenSwitchListener(ScreenSwitchListener* listener);

    /**
	 \brief Disallow screen switch.
	 Locking screen switch or incrementing lock counter.
	 \returns current screen switch lock counter
	 */
    int32 LockSwitch();

    /**
	 \brief Allow screen switch.
	 Decrementing lock counter if counter is zero unlocking screen switch.
	 \returns current screen switch lock counter
	 */
    int32 UnlockSwitch();

    bool IsRtl() const;
    void SetRtl(bool rtl);

    bool IsBiDiSupportEnabled() const;
    void SetBiDiSupportEnabled(bool support);

    bool IsHostControl(const UIControl* control) const;

    void RegisterControl(UIControl* control);
    void UnregisterControl(UIControl* control);

    void RegisterVisibleControl(UIControl* control);
    void UnregisterVisibleControl(UIControl* control);

    void RegisterComponent(UIControl* control, UIComponent* component);
    void UnregisterComponent(UIControl* control, UIComponent* component);

    void AddSystem(std::unique_ptr<UISystem> sceneSystem, const UISystem* insertBeforeSystem = nullptr);
    std::unique_ptr<UISystem> RemoveSystem(const UISystem* sceneSystem);

    template <typename SystemClass>
    SystemClass* GetSystem() const
    {
        for (auto& system : systems)
        {
            if (DAVA::IsPointerToExactClass<SystemClass>(system.get()))
            {
                return static_cast<SystemClass*>(system.get());
            }
        }

        return nullptr;
    }

    void AddSingleComponent(std::unique_ptr<UISingleComponent> single);
    std::unique_ptr<UISingleComponent> RemoveSingleComponent(const UISingleComponent* singleComponent);

    template <typename T>
    T* GetSingleComponent() const
    {
        for (auto& c : singleComponents)
        {
            if (IsPointerToExactClass<T>(c.get()))
            {
                return static_cast<T*>(c.get());
            }
        }
        return nullptr;
    }

    UITextSystem* GetTextSystem() const;
    UILayoutSystem* GetLayoutSystem() const;
    UIInputSystem* GetInputSystem() const;
    UIFocusSystem* GetFocusSystem() const;
    UISoundSystem* GetSoundSystem() const;
    UIUpdateSystem* GetUpdateSystem() const;
    UIStyleSheetSystem* GetStyleSheetSystem() const;
    UIRenderSystem* GetRenderSystem() const;
    UIEventsSystem* GetEventsSystem() const;

    void SetDoubleTapSettings(float32 time, float32 inch);

    VirtualCoordinatesSystem* vcs = nullptr; // TODO: Should be completely removed in favor of direct DAVA::Window methods

    void SetFlowRoot(UIControl* root);
    UIControl* GetFlowRoot() const;

    void SetPhysicalSafeAreaInsets(float32 left, float32 top, float32 right, float32 bottom, bool isLeftNotch, bool isRightNotch);

private:
    UIControlSystem();
    ~UIControlSystem();
    void Init();
    void Shutdown();

    void ProcessScreenLogic();

    void NotifyListenersWillSwitch(UIScreen* screen);
    void NotifyListenersDidSwitch(UIScreen* screen);
    bool CheckTimeAndPosition(UIEvent* newEvent);
    int32 CalculatedTapCount(UIEvent* newEvent);

    UIEvent MakeUIEvent(const InputEvent& inputEvent) const;
    eModifierKeys GetKeyboardModifierKeys() const;
    static eMouseButtons TranslateMouseElementToButtons(eInputElements element);

    friend class Private::EngineBackend;

    Vector<std::unique_ptr<UISystem>> systems;
    Vector<std::unique_ptr<UISingleComponent>> singleComponents;

    UITextSystem* textSystem = nullptr;
    UILayoutSystem* layoutSystem = nullptr;
    UIStyleSheetSystem* styleSheetSystem = nullptr;
    UIInputSystem* inputSystem = nullptr;
    UISoundSystem* soundSystem = nullptr;
    UIUpdateSystem* updateSystem = nullptr;
    UIRenderSystem* renderSystem = nullptr;
    UIEventsSystem* eventsSystem = nullptr;

    Vector<ScreenSwitchListener*> screenSwitchListeners;

    RefPtr<UIScreen> currentScreen;
    RefPtr<UIScreen> nextScreen;
    RefPtr<UIControl> popupContainer;
    Set<UIPopup*> popupsToRemove;

    int32 lockInputCounter = 0;
    int32 screenLockCount = 0;
    int32 frameSkip = 0;

    Rect fullscreenRect;

    bool removeCurrentScreen = false;

    uint32 resizePerFrame = 0; //used for logging some strange crahses on android

    float32 doubleClickTime = 0.f;
    float32 doubleClickInchSquare = 0.f;
    struct LastClickData
    {
        uint32 touchId = 0;
        Vector2 physPoint;
        float64 timestamp = 0.0;
        int32 tapCount = 0;
        bool lastClickEnded = false;
        RefPtr<UIControl> touchLocker; // last control has handled input
    };
    LastClickData lastClickData;

    RefPtr<UIControl> flowRoot;
};
}
