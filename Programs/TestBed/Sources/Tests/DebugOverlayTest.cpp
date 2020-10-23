#include "Tests/DebugOverlayTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "DeviceManager/DeviceManager.h"
#include "UI/Render/UIDebugRenderComponent.h"
#include "Input/Keyboard.h"
#include "Debug/DebugOverlay.h"
#include "Debug/Private/ImGui.h"

using namespace DAVA;

String TestOverlayItem::GetName() const
{
    return "Test item (testbed)";
}

void TestOverlayItem::Draw()
{
    bool shown = true;
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiSetCond_FirstUseEver);
    if (ImGui::Begin("Test window", &shown, ImGuiWindowFlags_NoFocusOnAppearing))
    {
        ImGui::Text("Test info");
    }
    ImGui::End();

    if (!shown)
    {
        GetEngineContext()->debugOverlay->HideItem(this);
    }
}

DebugOverlayTest::DebugOverlayTest(TestBed& app)
    : BaseScreen(app, "DebugOverlayTest")
{
}

void DebugOverlayTest::LoadResources()
{
    BaseScreen::LoadResources();

    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    UIButton* addItemButton = new UIButton(Rect(20, 20, 250, 50));
    addItemButton->SetStateFont(0xFF, font);
    addItemButton->SetStateFontSize(0xFF, 14.f);
    addItemButton->SetStateFontColor(0xFF, Color::White);
    addItemButton->SetStateText(0xFF, L"Add test item to the overlay");
    addItemButton->GetOrCreateComponent<UIDebugRenderComponent>();
    addItemButton->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &DebugOverlayTest::OnAddItem));
    AddControl(addItemButton);
    SafeRelease(addItemButton);

    UIButton* removeItemButton = new UIButton(Rect(20, 70, 250, 50));
    removeItemButton->SetStateFont(0xFF, font);
    removeItemButton->SetStateFontSize(0xFF, 14.f);
    removeItemButton->SetStateFontColor(0xFF, Color::White);
    removeItemButton->SetStateText(0xFF, L"Remove test item from the overlay");
    removeItemButton->GetOrCreateComponent<UIDebugRenderComponent>();
    removeItemButton->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &DebugOverlayTest::OnRemoveItem));
    AddControl(removeItemButton);
    SafeRelease(removeItemButton);

    UIButton* showHideOverlayButton = new UIButton(Rect(20, 150, 250, 50));
    showHideOverlayButton->SetStateFont(0xFF, font);
    showHideOverlayButton->SetStateFontSize(0xFF, 14.f);
    showHideOverlayButton->SetStateFontColor(0xFF, Color::White);
    showHideOverlayButton->SetStateText(0xFF, L"Show/hide overlay");
    showHideOverlayButton->GetOrCreateComponent<UIDebugRenderComponent>();
    showHideOverlayButton->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &DebugOverlayTest::OnShowHideOverlay));
    AddControl(showHideOverlayButton);
    SafeRelease(showHideOverlayButton);

    GetPrimaryWindow()->update.Connect(this, &DebugOverlayTest::OnWindowUpdate);
}

void DebugOverlayTest::UnloadResources()
{
    RemoveAllControls();

    BaseScreen::UnloadResources();

    GetPrimaryWindow()->update.Disconnect(this);

    DebugOverlay* overlay = GetEngineContext()->debugOverlay;

    if (itemRegistered)
    {
        overlay->UnregisterItem(&testItem);
        itemRegistered = false;
    }

    overlay->Hide();
}

void DebugOverlayTest::OnWindowUpdate(DAVA::Window* window, DAVA::float32 dt)
{
    const EngineContext* context = GetEngineContext();
    Keyboard* kb = context->deviceManager->GetKeyboard();
    if (kb != nullptr)
    {
        DigitalElementState keyState = kb->GetKeyState(DAVA::eInputElements::KB_GRAVE);
        if (keyState.IsJustPressed())
        {
            if (!context->debugOverlay->IsShown())
            {
                context->debugOverlay->Show();
            }
            else
            {
                context->debugOverlay->Hide();
            }
        }
    }
}

void DebugOverlayTest::OnAddItem(DAVA::BaseObject* sender, void* data, void* callerData)
{
    if (itemRegistered == false)
    {
        GetEngineContext()->debugOverlay->RegisterItem(&testItem);
        itemRegistered = true;
    }
}

void DebugOverlayTest::OnRemoveItem(DAVA::BaseObject* sender, void* data, void* callerData)
{
    if (itemRegistered)
    {
        GetEngineContext()->debugOverlay->UnregisterItem(&testItem);
        itemRegistered = false;
    }
}

void DebugOverlayTest::OnShowHideOverlay(DAVA::BaseObject* sender, void* data, void* callerData)
{
    const EngineContext* context = GetEngineContext();

    if (!context->debugOverlay->IsShown())
    {
        context->debugOverlay->Show();
    }
    else
    {
        context->debugOverlay->Hide();
    }
}
