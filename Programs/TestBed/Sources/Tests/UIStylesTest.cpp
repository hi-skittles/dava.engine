#include "Tests/UIStylesTest.h"
#include "Infrastructure/TestBed.h"
#include <Time/SystemTimer.h>
#include <UI/UIPackageLoader.h>
#include <UI/Events/UIEventBindingComponent.h>
#include "UI/Update/UIUpdateComponent.h"
#include <UI/Styles/UIStyleSheetSystem.h>
#include <Utils/StringFormat.h>

using namespace DAVA;

static const FastName STYLE_ON = FastName("on");
static const FastName STYLE_OFF = FastName("off");

UIStylesTest::UIStylesTest(TestBed& app)
    : BaseScreen(app, "UIStylesTest")
{
    GetOrCreateComponent<UIUpdateComponent>();
}

void UIStylesTest::LoadResources()
{
    BaseScreen::LoadResources();

    DefaultUIPackageBuilder pkgBuilder;
    UIPackageLoader().LoadPackage("~res:/TestBed/UI/StylesTest.yaml", &pkgBuilder);
    UIControl* main = pkgBuilder.GetPackage()->GetControl("Window");
    auto actions = main->GetOrCreateComponent<UIEventBindingComponent>();
    if (actions)
    {
        actions->BindAction(FastName("ADD"), [&](const DAVA::Any&) {
            container->RemoveClass(STYLE_OFF);
            container->AddClass(STYLE_ON);
        });
        actions->BindAction(FastName("REMOVE"), [&](const DAVA::Any&) {
            container->RemoveClass(STYLE_ON);
            container->AddClass(STYLE_OFF);
        });
        actions->BindAction(FastName("MORE"), [&](const DAVA::Any&) {
            for (uint32 i = 0; i < 1000; ++i)
            {
                RefPtr<UIControl> c(proto->Clone());
                container->AddControl(c.Get());
            }
        });
        actions->GetActionMap().Put(FastName("ADD_GLOBAL"), [&](const DAVA::Any&) {
            GetEngineContext()->uiControlSystem->GetStyleSheetSystem()->AddGlobalClass(FastName("global"));
        });
        actions->GetActionMap().Put(FastName("REMOVE_GLOBAL"), [&](const DAVA::Any&) {
            GetEngineContext()->uiControlSystem->GetStyleSheetSystem()->RemoveGlobalClass(FastName("global"));
        });
    }

    proto = pkgBuilder.GetPackage()->GetControl("Proto");
    container = main->FindByPath("**/Container");
    statusText = main->FindByPath<UIStaticText*>("**/StatusText");

    for (uint32 i = 0; i < 1000; ++i)
    {
        RefPtr<UIControl> c(proto->Clone());
        container->AddControl(c.Get());
    }

    AddControl(main);
}

void UIStylesTest::UnloadResources()
{
    BaseScreen::UnloadResources();
}

void UIStylesTest::Update(float32 delta)
{
    static float32 updateDelta = 0.f;
    static uint32 framesCount = 0;

    BaseScreen::Update(delta);

    updateDelta += SystemTimer::GetRealFrameDelta();
    framesCount += 1;
    if (updateDelta > 0.5f)
    {
        float32 fps = framesCount / updateDelta;
        statusText->SetUtf8Text(Format("FPS: %f, count: %d", fps, container->GetChildren().size()));
        updateDelta = 0.f;
        framesCount = 0;
    }
}
