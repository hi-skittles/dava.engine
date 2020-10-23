#include "UI/UIControlSystem.h"
#include "Debug/DVAssert.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Debug/ProfilerOverlay.h"
#include "Debug/Replay.h"
#include "DeviceManager/DeviceManager.h"
#include "Engine/Engine.h"
#include "Input/InputEvent.h"
#include "Input/InputSystem.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "Input/TouchScreen.h"
#include "Logger/Logger.h"
#include "Platform/DeviceInfo.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/TextBlock.h"
#include "Render/RenderHelper.h"
#include "Render/Renderer.h"
#include "Time/SystemTimer.h"
#include "UI/DataBinding/UIDataBindingSystem.h"
#include "UI/DataBinding/UIDataBindingPostProcessingSystem.h"
#include "UI/Flow/Private/UIFlowTransitionAnimationSystem.h"
#include "UI/Flow/UIFlowControllerSystem.h"
#include "UI/Flow/UIFlowStateSystem.h"
#include "UI/Flow/UIFlowViewSystem.h"
#include "UI/Focus/UIFocusSystem.h"
#include "UI/Input/UIInputSystem.h"
#include "UI/Layouts/UILayoutSystem.h"
#include "UI/Render/UIRenderSystem.h"
#include "UI/RichContent/UIRichContentSystem.h"
#include "UI/ScreenSwitchListener.h"
#include "UI/Scroll/UIScrollBarLinkSystem.h"
#include "UI/Scroll/UIScrollSystem.h"
#include "UI/Sound/UISoundSystem.h"
#include "UI/Styles/UIStyleSheetSystem.h"
#include "UI/Text/UITextSystem.h"
#include "UI/Script/UIScriptSystem.h"
#include "UI/Events/UIEventsSystem.h"
#include "UI/UIControlSystem.h"
#include "UI/UIEvent.h"
#include "UI/UIPopup.h"
#include "UI/UIScreen.h"
#include "UI/UIScreenTransition.h"
#include "UI/UISystem.h"
#include "UI/Update/UIUpdateSystem.h"
#include "UI/Joypad/UIJoypadSystem.h"
#include "UI/Scene3D/UIEntityMarkerSystem.h"

namespace DAVA
{
UIControlSystem::UIControlSystem()
{
    vcs = new VirtualCoordinatesSystem();
    vcs->EnableReloadResourceOnResize(true);
    vcs->virtualSizeChanged.Connect(this, [](const Size2i&) { TextBlock::ScreenResolutionChanged(); });
    vcs->physicalSizeChanged.Connect(this, [](const Size2i&) { TextBlock::ScreenResolutionChanged(); });

    AddSystem(std::make_unique<UIInputSystem>());
    AddSystem(std::make_unique<UIEventsSystem>());
    AddSystem(std::make_unique<UIFlowStateSystem>());
    AddSystem(std::make_unique<UIFlowViewSystem>());
    AddSystem(std::make_unique<UIFlowControllerSystem>());
    AddSystem(std::make_unique<UIScriptSystem>());
    AddSystem(std::make_unique<UIUpdateSystem>());
    AddSystem(std::make_unique<UIDataBindingSystem>());
    AddSystem(std::make_unique<UIRichContentSystem>());
    AddSystem(std::make_unique<UIEntityMarkerSystem>());
    AddSystem(std::make_unique<UIStyleSheetSystem>());
    AddSystem(std::make_unique<UITextSystem>()); // Must be before UILayoutSystem
    AddSystem(std::make_unique<UILayoutSystem>());
    AddSystem(std::make_unique<UIScrollSystem>());
    AddSystem(std::make_unique<UIScrollBarLinkSystem>());
    AddSystem(std::make_unique<UISoundSystem>());
    AddSystem(std::make_unique<UIJoypadSystem>());
    AddSystem(std::make_unique<UIRenderSystem>(RenderSystem2D::Instance()));
    AddSystem(std::make_unique<UIDataBindingPostProcessingSystem>(GetSystem<UIDataBindingSystem>()));

    AddSystem(std::make_unique<UIFlowTransitionAnimationSystem>(GetSystem<UIFlowStateSystem>(), GetSystem<UIRenderSystem>()), GetSystem<UIFlowViewSystem>());

    inputSystem = GetSystem<UIInputSystem>();
    styleSheetSystem = GetSystem<UIStyleSheetSystem>();
    textSystem = GetSystem<UITextSystem>();
    layoutSystem = GetSystem<UILayoutSystem>();
    soundSystem = GetSystem<UISoundSystem>();
    updateSystem = GetSystem<UIUpdateSystem>();
    renderSystem = GetSystem<UIRenderSystem>();
    eventsSystem = GetSystem<UIEventsSystem>();

    eventsSystem->RegisterCommands();

    SetDoubleTapSettings(0.5f, 0.25f);
}

UIControlSystem::~UIControlSystem()
{
    soundSystem = nullptr;
    inputSystem = nullptr;
    styleSheetSystem = nullptr;
    textSystem = nullptr;
    layoutSystem = nullptr;
    updateSystem = nullptr;
    renderSystem = nullptr;
    eventsSystem = nullptr;

    systems.clear();
    SafeDelete(vcs);
}

void UIControlSystem::Init()
{
    popupContainer.Set(new UIControl(Rect(0, 0, 1, 1)));
    popupContainer->SetName("UIControlSystem_popupContainer");
    popupContainer->SetInputEnabled(false);
    popupContainer->InvokeActive(UIControl::eViewState::VISIBLE);
    inputSystem->SetPopupContainer(popupContainer.Get());
    styleSheetSystem->SetPopupContainer(popupContainer);
    layoutSystem->SetPopupContainer(popupContainer);
    renderSystem->SetPopupContainer(popupContainer);
}

void UIControlSystem::Shutdown()
{
    inputSystem->SetPopupContainer(nullptr);
    inputSystem->SetCurrentScreen(nullptr);
    styleSheetSystem->SetPopupContainer(RefPtr<UIControl>());
    styleSheetSystem->SetCurrentScreen(RefPtr<UIScreen>());
    layoutSystem->SetPopupContainer(RefPtr<UIControl>());
    layoutSystem->SetCurrentScreen(RefPtr<UIScreen>());
    renderSystem->SetPopupContainer(RefPtr<UIControl>());
    renderSystem->SetCurrentScreen(RefPtr<UIScreen>());

    popupContainer->InvokeInactive();
    popupContainer->SetScene(nullptr);
    popupContainer = nullptr;

    if (currentScreen.Valid())
    {
        currentScreen->InvokeInactive();
        currentScreen->SetScene(nullptr);
        currentScreen = nullptr;
    }

    lastClickData.touchLocker = nullptr;
}

void UIControlSystem::SetScreen(UIScreen* _nextScreen)
{
    if (_nextScreen == currentScreen)
    {
        if (nextScreen != nullptr)
        {
            nextScreen = nullptr;
        }
        return;
    }

    if (nextScreen.Valid())
    {
        Logger::Warning("2 screen switches during one frame.");
    }

    nextScreen = _nextScreen;

    if (nextScreen == nullptr)
    {
        removeCurrentScreen = true;
        ProcessScreenLogic();
    }
}

UIScreen* UIControlSystem::GetScreen() const
{
    return currentScreen.Get();
}

void UIControlSystem::AddPopup(UIPopup* newPopup)
{
    Set<UIPopup*>::const_iterator it = popupsToRemove.find(newPopup);
    if (popupsToRemove.end() != it)
    {
        popupsToRemove.erase(it);
        return;
    }

    if (newPopup->GetRect() != fullscreenRect)
    {
        newPopup->SystemScreenSizeChanged(fullscreenRect);
    }

    newPopup->LoadGroup();
    popupContainer->AddControl(newPopup);
}

void UIControlSystem::RemovePopup(UIPopup* popup)
{
    if (popupsToRemove.count(popup))
    {
        Logger::Warning("[UIControlSystem::RemovePopup] attempt to double remove popup during one frame.");
        return;
    }

    const auto& popups = popupContainer->GetChildren();
    if (popups.end() == std::find(popups.begin(), popups.end(), RefPtr<UIControl>::ConstructWithRetain(popup)))
    {
        Logger::Error("[UIControlSystem::RemovePopup] attempt to remove uknown popup.");
        DVASSERT(false);
        return;
    }

    popupsToRemove.insert(popup);
}

void UIControlSystem::RemoveAllPopups()
{
    popupsToRemove.clear();
    const auto& totalChilds = popupContainer->GetChildren();
    for (auto it = totalChilds.begin(); it != totalChilds.end(); it++)
    {
        popupsToRemove.insert(DynamicTypeCheck<UIPopup*>(it->Get()));
    }
}

UIControl* UIControlSystem::GetPopupContainer() const
{
    return popupContainer.Get();
}

void UIControlSystem::Reset()
{
    inputSystem->SetCurrentScreen(nullptr);
    styleSheetSystem->SetCurrentScreen(RefPtr<UIScreen>());
    layoutSystem->SetCurrentScreen(RefPtr<UIScreen>());
    renderSystem->SetCurrentScreen(RefPtr<UIScreen>());
    SetScreen(nullptr);
}

void UIControlSystem::ForceUpdateControl(float32 timeElapsed, UIControl* control)
{
    DVASSERT(control != nullptr);
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_UPDATE);

    if (control == nullptr || !Renderer::GetOptions()->IsOptionEnabled(RenderOptions::UPDATE_UI_CONTROL_SYSTEM))
        return;

    for (auto& system : systems)
    {
        system->ForceProcessControl(timeElapsed, control);
    }
}

void UIControlSystem::ForceDrawControl(UIControl* control)
{
    DVASSERT(control != nullptr);
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_DRAW);

    if (control == nullptr)
        return;

    renderSystem->ForceRenderControl(control);
}

void UIControlSystem::ProcessScreenLogic()
{
    /*
     if next screen or we need to removecurrent screen
     */
    if (screenLockCount == 0 && (nextScreen.Valid() || removeCurrentScreen))
    {
        RefPtr<UIScreen> nextScreenProcessed;

        nextScreenProcessed = nextScreen;
        nextScreen = nullptr; // functions called by this method can request another screen switch (for example, LoadResources)

        LockInput();

        CancelAllInputs();

        NotifyListenersWillSwitch(nextScreenProcessed.Get());

        // if we have current screen we call events, unload resources for it group
        if (currentScreen)
        {
            currentScreen->InvokeInactive();
            currentScreen->SetScene(nullptr);

            RefPtr<UIScreen> prevScreen = currentScreen;
            currentScreen = nullptr;
            inputSystem->SetCurrentScreen(currentScreen.Get());
            styleSheetSystem->SetCurrentScreen(currentScreen);
            layoutSystem->SetCurrentScreen(currentScreen);
            renderSystem->SetCurrentScreen(currentScreen);

            if ((nextScreenProcessed == nullptr) || (prevScreen->GetGroupId() != nextScreenProcessed->GetGroupId()))
            {
                prevScreen->UnloadGroup();
            }
        }
        // if we have next screen we load new resources, if it equal to zero we just remove screen
        if (nextScreenProcessed)
        {
            if (nextScreenProcessed->GetRect() != fullscreenRect)
            {
                nextScreenProcessed->SystemScreenSizeChanged(fullscreenRect);
            }

            nextScreenProcessed->LoadGroup();
        }
        currentScreen = nextScreenProcessed;

        if (currentScreen)
        {
            currentScreen->SetScene(this);
            currentScreen->InvokeActive(UIControl::eViewState::VISIBLE);
        }
        inputSystem->SetCurrentScreen(currentScreen.Get());
        styleSheetSystem->SetCurrentScreen(currentScreen);
        layoutSystem->SetCurrentScreen(currentScreen);
        renderSystem->SetCurrentScreen(currentScreen);

        NotifyListenersDidSwitch(currentScreen.Get());

        UnlockInput();

        frameSkip = FRAME_SKIP;
        removeCurrentScreen = false;
    }

    /*
     if we have popups to remove, we removes them here
     */
    for (Set<UIPopup*>::iterator it = popupsToRemove.begin(); it != popupsToRemove.end(); it++)
    {
        UIPopup* p = *it;
        if (p)
        {
            p->Retain();
            popupContainer->RemoveControl(p);
            p->UnloadGroup();
            p->Release();
        }
    }
    popupsToRemove.clear();
}

void UIControlSystem::Update()
{
    float32 timeElapsed = SystemTimer::GetFrameDelta();
    UpdateWithCustomTime(timeElapsed);
}

void UIControlSystem::UpdateWithCustomTime(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_UPDATE);

    ProcessScreenLogic();

    if (Renderer::GetOptions()->IsOptionEnabled(RenderOptions::UPDATE_UI_CONTROL_SYSTEM))
    {
        for (auto& system : systems)
        {
            system->Process(timeElapsed);
        }

        for (auto& components : singleComponents)
        {
            components->ResetState();
        }
    }
}

void UIControlSystem::Draw()
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_DRAW);

    resizePerFrame = 0;

    renderSystem->Render();
    GetSystem<UIFlowTransitionAnimationSystem>()->Render();

    if (frameSkip > 0)
    {
        frameSkip--;
    }
}

void UIControlSystem::SwitchInputToControl(uint32 eventID, UIControl* targetControl)
{
    return inputSystem->SwitchInputToControl(eventID, targetControl);
}

bool UIControlSystem::HandleInputEvent(const InputEvent& inputEvent)
{
    UIEvent uie = MakeUIEvent(inputEvent);
    if (uie.phase != UIEvent::Phase::ERROR)
    {
        OnInput(&uie);
        return true; // ????
    }
    return false;
}

void UIControlSystem::OnInput(UIEvent* newEvent)
{
    newEvent->point = GetEngineContext()->uiControlSystem->vcs->ConvertInputToVirtual(newEvent->physPoint);
    newEvent->tapCount = CalculatedTapCount(newEvent);

    if (Replay::IsPlayback())
    {
        return;
    }

    if (lockInputCounter > 0)
    {
        return;
    }

    if (ProfilerOverlay::globalProfilerOverlay && ProfilerOverlay::globalProfilerOverlay->OnInput(newEvent))
        return;

    if (frameSkip <= 0)
    {
        if (Replay::IsRecord())
        {
            Replay::Instance()->RecordEvent(newEvent);
        }
        inputSystem->HandleEvent(newEvent);
        // Store last 'touchLocker' reference.
        if (newEvent->touchLocker)
        {
            lastClickData.touchLocker = newEvent->touchLocker;
        }
    } // end if frameSkip <= 0
}

void UIControlSystem::CancelInput(UIEvent* touch)
{
    inputSystem->CancelInput(touch);
}

void UIControlSystem::CancelAllInputs()
{
    lastClickData.touchLocker = nullptr;
    lastClickData.tapCount = 0;
    lastClickData.lastClickEnded = false;

    inputSystem->CancelAllInputs();
}

void UIControlSystem::CancelInputs(UIControl* control, bool hierarchical)
{
    inputSystem->CancelInputs(control, hierarchical);
}

int32 UIControlSystem::LockInput()
{
    lockInputCounter++;
    if (lockInputCounter > 0)
    {
        CancelAllInputs();
    }
    return lockInputCounter;
}

int32 UIControlSystem::UnlockInput()
{
    DVASSERT(lockInputCounter != 0);

    lockInputCounter--;
    if (lockInputCounter == 0)
    {
        // VB: Done that because hottych asked to do that.
        CancelAllInputs();
    }
    return lockInputCounter;
}

int32 UIControlSystem::GetLockInputCounter() const
{
    return lockInputCounter;
}

const Vector<UIEvent>& UIControlSystem::GetAllInputs() const
{
    return inputSystem->GetAllInputs();
}

void UIControlSystem::SetExclusiveInputLocker(UIControl* locker, uint32 lockEventId)
{
    inputSystem->SetExclusiveInputLocker(locker, lockEventId);
}

UIControl* UIControlSystem::GetExclusiveInputLocker() const
{
    return inputSystem->GetExclusiveInputLocker();
}

void UIControlSystem::ScreenSizeChanged(const Rect& newFullscreenRect)
{
    if (fullscreenRect == newFullscreenRect)
    {
        return;
    }

    resizePerFrame++;
    if (resizePerFrame >= 5)
    {
        Logger::Warning("Resizes per frame : %d", resizePerFrame);
    }

    fullscreenRect = newFullscreenRect;

    if (currentScreen.Valid())
    {
        currentScreen->SystemScreenSizeChanged(fullscreenRect);
    }

    popupContainer->SystemScreenSizeChanged(fullscreenRect);
}

void UIControlSystem::SetHoveredControl(UIControl* newHovered)
{
    inputSystem->SetHoveredControl(newHovered);
}

UIControl* UIControlSystem::GetHoveredControl() const
{
    return inputSystem->GetHoveredControl();
}

void UIControlSystem::SetFocusedControl(UIControl* newFocused)
{
    GetFocusSystem()->SetFocusedControl(newFocused);
}

UIControl* UIControlSystem::GetFocusedControl() const
{
    return GetFocusSystem()->GetFocusedControl();
}

void UIControlSystem::ProcessControlEvent(int32 eventType, const UIEvent* uiEvent, UIControl* control)
{
    soundSystem->ProcessControlEvent(eventType, uiEvent, control);
    eventsSystem->ProcessControlEvent(eventType, uiEvent, control);
}

void UIControlSystem::ReplayEvents()
{
    while (Replay::Instance()->IsEvent())
    {
        int32 activeCount = Replay::Instance()->PlayEventsCount();
        while (activeCount--)
        {
            UIEvent ev = Replay::Instance()->PlayEvent();
            OnInput(&ev);
        }
    }
}

int32 UIControlSystem::LockSwitch()
{
    screenLockCount++;
    return screenLockCount;
}

int32 UIControlSystem::UnlockSwitch()
{
    screenLockCount--;
    DVASSERT(screenLockCount >= 0);
    return screenLockCount;
}

void UIControlSystem::AddScreenSwitchListener(ScreenSwitchListener* listener)
{
    screenSwitchListeners.push_back(listener);
}

void UIControlSystem::RemoveScreenSwitchListener(ScreenSwitchListener* listener)
{
    Vector<ScreenSwitchListener*>::iterator it = std::find(screenSwitchListeners.begin(), screenSwitchListeners.end(), listener);
    if (it != screenSwitchListeners.end())
        screenSwitchListeners.erase(it);
}

void UIControlSystem::NotifyListenersWillSwitch(UIScreen* screen)
{
    // TODO do we need Copy?
    Vector<ScreenSwitchListener*> screenSwitchListenersCopy = screenSwitchListeners;
    for (auto& listener : screenSwitchListenersCopy)
    {
        listener->OnScreenWillSwitch(screen);
    }
}

void UIControlSystem::NotifyListenersDidSwitch(UIScreen* screen)
{
    // TODO do we need Copy?
    Vector<ScreenSwitchListener*> screenSwitchListenersCopy = screenSwitchListeners;
    for (auto& listener : screenSwitchListenersCopy)
    {
        listener->OnScreenDidSwitch(screen);
    }
}

bool UIControlSystem::CheckTimeAndPosition(UIEvent* newEvent)
{
    if ((lastClickData.timestamp != 0.0) && ((newEvent->timestamp - lastClickData.timestamp) < doubleClickTime))
    {
        Vector2 point = lastClickData.physPoint - newEvent->physPoint;

        float32 dpi = GetPrimaryWindow()->GetDPI();
        float32 doubleClickPhysSquare = doubleClickInchSquare * (dpi * dpi);

        if (point.SquareLength() <= doubleClickPhysSquare)
        {
            return true;
        }
    }
    return false;
}

int32 UIControlSystem::CalculatedTapCount(UIEvent* newEvent)
{
    int32 tapCount = 1;

    // Observe double click:
    // doubleClickTime - interval between newEvent and lastEvent,
    // doubleClickPhysSquare - square for double click in physical pixels
    if (newEvent->phase == UIEvent::Phase::BEGAN)
    {
        DVASSERT(newEvent->tapCount == 0 && "Native implementation disabled, tapCount must be 0");
        // only if last event ended
        if (lastClickData.lastClickEnded)
        {
            // Make addditional 'IsPointInside' check for correct double tap detection.
            // Event point must be in previously tapped control rect.
            UIControl* lastTouchLocker = lastClickData.touchLocker.Get();
            if (CheckTimeAndPosition(newEvent) && (lastTouchLocker == nullptr || lastTouchLocker->IsPointInside(newEvent->point)))
            {
                tapCount = lastClickData.tapCount + 1;
            }
        }
        lastClickData.touchId = newEvent->touchId;
        lastClickData.timestamp = newEvent->timestamp;
        lastClickData.physPoint = newEvent->physPoint;
        lastClickData.tapCount = tapCount;
        lastClickData.lastClickEnded = false;
    }
    else if (newEvent->phase == UIEvent::Phase::ENDED)
    {
        if (newEvent->touchId == lastClickData.touchId)
        {
            lastClickData.lastClickEnded = true;
            if (lastClickData.tapCount != 1 && CheckTimeAndPosition(newEvent))
            {
                tapCount = lastClickData.tapCount;
            }
        }
    }
    return tapCount;
}

bool UIControlSystem::IsRtl() const
{
    return layoutSystem->IsRtl();
}

void UIControlSystem::SetRtl(bool rtl)
{
    layoutSystem->SetRtl(rtl);
}

bool UIControlSystem::IsBiDiSupportEnabled() const
{
    return TextBlock::IsBiDiSupportEnabled();
}

void UIControlSystem::SetBiDiSupportEnabled(bool support)
{
    TextBlock::SetBiDiSupportEnabled(support);
}

bool UIControlSystem::IsHostControl(const UIControl* control) const
{
    return (GetScreen() == control || GetPopupContainer() == control || GetFlowRoot() == control);
}

void UIControlSystem::RegisterControl(UIControl* control)
{
    for (auto& system : systems)
    {
        system->RegisterControl(control);
    }
}

void UIControlSystem::UnregisterControl(UIControl* control)
{
    for (auto& system : systems)
    {
        system->UnregisterControl(control);
    }
}

void UIControlSystem::RegisterVisibleControl(UIControl* control)
{
    for (auto& system : systems)
    {
        system->OnControlVisible(control);
    }
}

void UIControlSystem::UnregisterVisibleControl(UIControl* control)
{
    for (auto& system : systems)
    {
        system->OnControlInvisible(control);
    }
}

void UIControlSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    for (auto& system : systems)
    {
        system->RegisterComponent(control, component);
    }
}

void UIControlSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    for (auto& system : systems)
    {
        system->UnregisterComponent(control, component);
    }
}

void UIControlSystem::AddSystem(std::unique_ptr<UISystem> system, const UISystem* insertBeforeSystem)
{
    system->SetScene(this);
    UISystem* weak = system.get();
    if (insertBeforeSystem)
    {
        auto insertIt = std::find_if(systems.begin(), systems.end(),
                                     [insertBeforeSystem](const std::unique_ptr<UISystem>& systemPtr) {
                                         return systemPtr.get() == insertBeforeSystem;
                                     });
        DVASSERT(insertIt != systems.end());
        systems.insert(insertIt, std::move(system));
    }
    else
    {
        systems.push_back(std::move(system));
    }
    weak->RegisterSystem();
}

std::unique_ptr<UISystem> UIControlSystem::RemoveSystem(const UISystem* system)
{
    auto it = std::find_if(systems.begin(), systems.end(),
                           [system](const std::unique_ptr<UISystem>& systemPtr) {
                               return systemPtr.get() == system;
                           });

    if (it != systems.end())
    {
        (*it)->UnregisterSystem();
        std::unique_ptr<UISystem> systemPtr(it->release());
        systems.erase(it);
        systemPtr->SetScene(nullptr);
        return systemPtr;
    }

    return nullptr;
}

void UIControlSystem::AddSingleComponent(std::unique_ptr<UISingleComponent> single)
{
    singleComponents.push_back(std::move(single));
}

std::unique_ptr<UISingleComponent> UIControlSystem::RemoveSingleComponent(const UISingleComponent* singleComponent)
{
    auto it = std::find_if(singleComponents.begin(), singleComponents.end(),
                           [singleComponent](const std::unique_ptr<UISingleComponent>& ptr) {
                               return ptr.get() == singleComponent;
                           });
    if (it != singleComponents.end())
    {
        std::unique_ptr<UISingleComponent> ptr(it->release());
        singleComponents.erase(it);
        return ptr;
    }
    return nullptr;
}

UITextSystem* UIControlSystem::GetTextSystem() const
{
    return textSystem;
}

UILayoutSystem* UIControlSystem::GetLayoutSystem() const
{
    return layoutSystem;
}

UIInputSystem* UIControlSystem::GetInputSystem() const
{
    return inputSystem;
}

UIFocusSystem* UIControlSystem::GetFocusSystem() const
{
    return inputSystem->GetFocusSystem();
}

UISoundSystem* UIControlSystem::GetSoundSystem() const
{
    return soundSystem;
}

UIStyleSheetSystem* UIControlSystem::GetStyleSheetSystem() const
{
    return styleSheetSystem;
}

DAVA::UIRenderSystem* UIControlSystem::GetRenderSystem() const
{
    return renderSystem;
}

UIUpdateSystem* UIControlSystem::GetUpdateSystem() const
{
    return updateSystem;
}

UIEventsSystem* UIControlSystem::GetEventsSystem() const
{
    return eventsSystem;
}

void UIControlSystem::SetDoubleTapSettings(float32 time, float32 inch)
{
    DVASSERT((time > 0.0f) && (inch > 0.0f));
    doubleClickTime = time;
    doubleClickInchSquare = inch * inch;
}

void UIControlSystem::SetFlowRoot(UIControl* root)
{
    if (flowRoot == root)
    {
        return;
    }

    if (flowRoot)
    {
        flowRoot->InvokeInactive();
        flowRoot->SetScene(nullptr);
    }

    flowRoot = root;

    if (flowRoot)
    {
        flowRoot->SetScene(this);
        flowRoot->InvokeActive(UIControl::eViewState::VISIBLE);
    }
}

UIControl* UIControlSystem::GetFlowRoot() const
{
    return flowRoot.Get();
}

void UIControlSystem::SetPhysicalSafeAreaInsets(float32 left, float32 top, float32 right, float32 bottom, bool isLeftNotch, bool isRightNotch)
{
    layoutSystem->SetPhysicalSafeAreaInsets(left, top, right, bottom, isLeftNotch, isRightNotch);
}

UIEvent UIControlSystem::MakeUIEvent(const InputEvent& inputEvent) const
{
    UIEvent uie;
    uie.phase = UIEvent::Phase::ERROR;
    uie.timestamp = inputEvent.timestamp;
    uie.window = inputEvent.window;

    if (inputEvent.deviceType == eInputDeviceTypes::KEYBOARD)
    {
        uie.device = eInputDevices::KEYBOARD;
        uie.key = inputEvent.elementId;
        uie.modifiers = GetKeyboardModifierKeys();

        if (inputEvent.keyboardEvent.charCode > 0)
        {
            uie.phase = inputEvent.keyboardEvent.charRepeated ? UIEvent::Phase::CHAR_REPEAT : UIEvent::Phase::CHAR;
            uie.keyChar = inputEvent.keyboardEvent.charCode;
        }
        else
        {
            if (inputEvent.digitalState.IsReleased())
            {
                uie.phase = UIEvent::Phase::KEY_UP;
            }
            else
            {
                if (inputEvent.digitalState.IsJustPressed())
                {
                    uie.phase = UIEvent::Phase::KEY_DOWN;
                }
                else
                {
                    uie.phase = UIEvent::Phase::KEY_DOWN_REPEAT;
                }
            }
        }
    }
    else if (IsMouseInputElement(inputEvent.elementId))
    {
        uie.device = eInputDevices::MOUSE;
        uie.mouseButton = eMouseButtons::NONE;
        uie.isRelative = inputEvent.mouseEvent.isRelative;
        uie.modifiers = GetKeyboardModifierKeys();

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        AnalogElementState mousePosition = mouse->GetPosition();
        AnalogElementState mouseWheelDelta = mouse->GetWheelDelta();

        switch (inputEvent.elementId)
        {
        case eInputElements::MOUSE_LBUTTON:
        case eInputElements::MOUSE_RBUTTON:
        case eInputElements::MOUSE_MBUTTON:
        case eInputElements::MOUSE_EXT1BUTTON:
        case eInputElements::MOUSE_EXT2BUTTON:
            uie.phase = inputEvent.digitalState.IsPressed() ? UIEvent::Phase::BEGAN : UIEvent::Phase::ENDED;
            uie.mouseButton = static_cast<eMouseButtons>(inputEvent.elementId - eInputElements::MOUSE_LBUTTON + 1);
            uie.physPoint = { mousePosition.x, mousePosition.y };
            break;
        case eInputElements::MOUSE_WHEEL:
            uie.phase = UIEvent::Phase::WHEEL;
            uie.physPoint = { mousePosition.x, mousePosition.y };
            uie.wheelDelta = { mouseWheelDelta.x, mouseWheelDelta.y };
            break;
        case eInputElements::MOUSE_POSITION:
            // TODO: Holy shit, how to make multiple DRAG UIEvents from single inputEvent
            uie.mouseButton = TranslateMouseElementToButtons(mouse->GetFirstPressedButton());
            uie.phase = uie.mouseButton == eMouseButtons::NONE ? UIEvent::Phase::MOVE : UIEvent::Phase::DRAG;
            uie.physPoint = { mousePosition.x, mousePosition.y };
            break;
        default:
            DVASSERT(0, "Unexpected mouse input element");
            break;
        }
    }
    else if (IsTouchInputElement(inputEvent.elementId))
    {
        uie.device = eInputDevices::TOUCH_SURFACE;
        uie.modifiers = GetKeyboardModifierKeys();

        const bool isDigitalEvent = IsTouchClickInputElement(inputEvent.elementId);

        // UIEvent's touch id = touch index + 1 (since 0 means 'no touch' inside of UI system)

        if (isDigitalEvent)
        {
            AnalogElementState analogState = inputEvent.device->GetAnalogElementState(GetTouchPositionElementFromClickElement(inputEvent.elementId));

            uie.phase = inputEvent.digitalState.IsPressed() ? UIEvent::Phase::BEGAN : UIEvent::Phase::ENDED;
            uie.physPoint = { analogState.x, analogState.y };

            uie.touchId = inputEvent.elementId - eInputElements::TOUCH_FIRST_CLICK + 1;
        }
        else
        {
            uie.phase = UIEvent::Phase::DRAG;
            uie.physPoint = { inputEvent.analogState.x, inputEvent.analogState.y };
            uie.touchId = inputEvent.elementId - eInputElements::TOUCH_FIRST_POSITION + 1;
        }
    }

    return uie;
}

eModifierKeys UIControlSystem::GetKeyboardModifierKeys() const
{
    eModifierKeys modifierKeys = eModifierKeys::NONE;
    Keyboard* keyboard = GetEngineContext()->deviceManager->GetKeyboard();
    if (keyboard != nullptr)
    {
        DigitalElementState lctrl = keyboard->GetKeyState(eInputElements::KB_LCTRL);
        DigitalElementState rctrl = keyboard->GetKeyState(eInputElements::KB_RCTRL);
        if (lctrl.IsPressed() || rctrl.IsPressed())
        {
            modifierKeys |= eModifierKeys::CONTROL;
        }

        DigitalElementState lshift = keyboard->GetKeyState(eInputElements::KB_LSHIFT);
        DigitalElementState rshift = keyboard->GetKeyState(eInputElements::KB_RSHIFT);
        if (lshift.IsPressed() || rshift.IsPressed())
        {
            modifierKeys |= eModifierKeys::SHIFT;
        }

        DigitalElementState lalt = keyboard->GetKeyState(eInputElements::KB_LALT);
        DigitalElementState ralt = keyboard->GetKeyState(eInputElements::KB_RALT);
        if (lalt.IsPressed() || ralt.IsPressed())
        {
            modifierKeys |= eModifierKeys::ALT;
        }

        DigitalElementState lcmd = keyboard->GetKeyState(eInputElements::KB_LCMD);
        DigitalElementState rcmd = keyboard->GetKeyState(eInputElements::KB_RCMD);
        if (lcmd.IsPressed() || rcmd.IsPressed())
        {
            modifierKeys |= eModifierKeys::COMMAND;
        }
    }
    return modifierKeys;
}

eMouseButtons UIControlSystem::TranslateMouseElementToButtons(eInputElements element)
{
    switch (element)
    {
    case eInputElements::MOUSE_LBUTTON:
        return eMouseButtons::LEFT;
    case eInputElements::MOUSE_RBUTTON:
        return eMouseButtons::RIGHT;
    case eInputElements::MOUSE_MBUTTON:
        return eMouseButtons::MIDDLE;
    case eInputElements::MOUSE_EXT1BUTTON:
        return eMouseButtons::EXTENDED1;
    case eInputElements::MOUSE_EXT2BUTTON:
        return eMouseButtons::EXTENDED2;
    default:
        return eMouseButtons::NONE;
    }
}
}
