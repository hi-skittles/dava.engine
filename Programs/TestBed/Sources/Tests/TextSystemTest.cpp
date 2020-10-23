#include "Tests/TextSystemTest.h"

#include "Infrastructure/TestBed.h"
#include <Logger/Logger.h>
#include <Utils/Utils.h>
#include <Utils/UTF8Utils.h>
#include <UI/Update/UIUpdateComponent.h>
#include <UI/Text/UITextComponent.h>
#include <UI/Text/UITextSystem.h>
#include <UI/Events/UIEventBindingComponent.h>

using namespace DAVA;

const static std::vector<String> testCaseNames = {
    "NoMultilineTest",
    "MultilineTest",
    "MultilineBySymbolTest",
    "ShadowTest",
    "ParentColorTest",
    "PercentOfContentTest"
};

struct TextTestCase
{
    RefPtr<UIControl> control;
    String name;

    Vector2 origSize;

    float minScale = 0.8f;
    float maxScale = 1.2f;
    float scale = 1.f;
    float scaleStep = 0.05f;

    TextTestCase(UIControl* control_, String name_)
    {
        control = control_;
        name = name_;
        origSize = control->GetSize();
    }

    void Update(float32 delta)
    {
        scale += scaleStep * delta;
        if (scale > maxScale)
        {
            scale = maxScale;
            scaleStep = -scaleStep;
        }
        else if (scale < minScale)
        {
            scale = minScale;
            scaleStep = -scaleStep;
        }
        control->SetSize(origSize * scale);
    }
};

TextSystemTest::TextSystemTest(TestBed& app)
    : BaseScreen(app, "TextSystemTest")
{
    GetOrCreateComponent<UIUpdateComponent>();
}

void TextSystemTest::LoadResources()
{
    BaseScreen::LoadResources();

    DAVA::DefaultUIPackageBuilder pkgBuilder;
    DAVA::UIPackageLoader().LoadPackage("~res:/TestBed/UI/Text/TextSystemTest.yaml", &pkgBuilder);
    UIControl* dialog = pkgBuilder.GetPackage()->GetControl("MainFrame");
    AddControl(dialog);

    statusText = static_cast<UIStaticText*>(dialog->FindByName("StatusText"));
    holderControl = static_cast<UIStaticText*>(dialog->FindByName("Holder"));

    auto actions = dialog->GetOrCreateComponent<UIEventBindingComponent>();
    actions->BindAction(FastName("START"), [&](const DAVA::Any&) {
        state = PLAYING;
        ChangeCurrentTest(testIdx);
    });
    actions->BindAction(FastName("STOP"), [&](const DAVA::Any&) {
        state = STOPPED;
    });
    actions->BindAction(FastName("NEXT"), [&](const DAVA::Any&) {
        ChangeCurrentTest(testIdx + 1);
    });
    actions->BindAction(FastName("PREV"), [&](const DAVA::Any&) {
        ChangeCurrentTest(testIdx - 1);
    });

    for (String name : testCaseNames)
    {
        UIControl* c = pkgBuilder.GetPackage()->GetControl(name);
        objects.push_back(std::make_shared<TextTestCase>(c, name));
    }
    activeObject = nullptr;
}

void TextSystemTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    statusText = nullptr;
    activeObject = nullptr;
    holderControl = nullptr;
}

void TextSystemTest::ChangeCurrentTest(int32 testIdx_)
{
    testIdx = DAVA::Clamp<int32>(testIdx_, 0, static_cast<int32>(objects.size() - 1));

    DVASSERT(objects.size() > 0);

    holderControl->RemoveAllControls();

    activeObject = objects[testIdx];
    if (activeObject)
    {
        activeObject->control->SetPivotPoint(Vector2(0.5f, 0.5f));
        holderControl->AddControl(activeObject->control.Get());
    }
    else
    {
        activeObject = nullptr;
    }
}

void TextSystemTest::Update(float32 delta)
{
    static float32 updateDelta = 0.f;
    static uint32 framesCount = 0;

    BaseScreen::Update(delta);

    updateDelta += SystemTimer::GetRealFrameDelta();
    framesCount += 1;
    if (updateDelta > 0.5f)
    {
        float32 fps = framesCount / updateDelta;
        String stateStr = (state == PLAYING) ? "PLAYING" : "STOPPED";
        String testName = "<NULL>";
        if (activeObject)
        {
            testName = activeObject->name;
        }
        statusText->SetUtf8Text(Format("FPS: %.0f\nSTATE: %s\nTEST: %s", fps, stateStr.c_str(), testName.c_str()));
        updateDelta = 0.f;
        framesCount = 0;
    }

    if (activeObject && state == PLAYING)
    {
        activeObject->Update(delta);
    }
}
