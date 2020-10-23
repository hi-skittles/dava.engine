#include "Tests/WindowTest.h"

#include "Infrastructure/TestBed.h"

#include <Engine/Engine.h>
#include <Engine/Window.h>
#include <Utils/StringFormat.h>

using namespace DAVA;

class TextFieldDelegate : public UITextFieldDelegate
{
public:
    TextFieldDelegate(Function<void(const Rect& keyboardRect)> callback)
        : callback(callback)
    {
    }

    void OnKeyboardShown(const Rect& keyboardRect) override
    {
        callback(keyboardRect);
    }

    void OnKeyboardHidden() override
    {
        callback(Rect());
    }

private:
    const Function<void(const Rect& keyboardRect)> callback;
};

WindowTest::WindowTest(TestBed& app)
    : BaseScreen(app, "WindowTest")
{
}

void WindowTest::LoadResources()
{
    BaseScreen::LoadResources();

    DAVA::DefaultUIPackageBuilder pkgBuilder;
    DAVA::UIPackageLoader().LoadPackage("~res:/TestBed/UI/WindowTest.yaml", &pkgBuilder);
    UIControl* main = pkgBuilder.GetPackage()->GetControl("MainFrame");
    AddControl(main);

    visibleFrameRectText = main->FindByPath<UIStaticText*>("**/VisibleFrameRectText");
    keyboardFrameRectText = main->FindByPath<UIStaticText*>("**/KeyboardFrameRectText");
    textField1 = main->FindByPath<UITextField*>("**/TextField1");
    textField2 = main->FindByPath<UITextField*>("**/TextField2");

    tfDelegate = new TextFieldDelegate([this](const Rect& r) {
        UpdateKeyboardFrameSize(r);
    });
    textField1->SetDelegate(tfDelegate);
    textField2->SetDelegate(tfDelegate);

    GetPrimaryWindow()->visibleFrameChanged.Connect(this, [this](Window*, const Rect& r) {
        Rect converted = GetEngineContext()->uiControlSystem->vcs->ConvertInputToVirtual(r);
        UpdateVisibleFrameSize(converted);
    });

    UpdateVisibleFrameSize(main->GetRect());
}

void WindowTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    textField1->SetDelegate(nullptr);
    textField2->SetDelegate(nullptr);
    SafeDelete(tfDelegate);

    GetPrimaryWindow()->visibleFrameChanged.Disconnect(this);
}

void WindowTest::UpdateKeyboardFrameSize(const Rect& r)
{
    Vector2 virtualSize = Vector2(static_cast<float32>(GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dx), static_cast<float32>(GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dy));
    Rect clamped = r.Intersection(Rect(Vector2(), virtualSize));
    keyboardFrameRectText->SetUtf8Text(Format("%.3f, %.3f - %.3f, %.3f", clamped.x, clamped.y, clamped.dx, clamped.dy));
}

void WindowTest::UpdateVisibleFrameSize(const Rect& r)
{
    Vector2 virtualSize = Vector2(static_cast<float32>(GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dx), static_cast<float32>(GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dy));
    Rect clamped = r.Intersection(Rect(Vector2(), virtualSize));
    visibleFrameRectText->SetUtf8Text(Format("%.3f, %.3f - %.3f, %.3f", clamped.x, clamped.y, clamped.dx, clamped.dy));
}
