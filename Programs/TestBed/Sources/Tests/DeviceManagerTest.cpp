#include "Tests/DeviceManagerTest.h"

#include "Infrastructure/TestBed.h"

#include <Engine/Engine.h>
#include <DeviceManager/DeviceManager.h>
#include <UI/Render/UIDebugRenderComponent.h>

using namespace DAVA;

DeviceManagerTest::DeviceManagerTest(TestBed& app)
    : BaseScreen(app, "DeviceManagerTest")
{
    deviceManager = app.GetEngine().GetContext()->deviceManager;
}

void DeviceManagerTest::LoadResources()
{
    BaseScreen::LoadResources();

    uiDisplayDescr = new UIStaticText();
    uiDisplayDescr->SetTextColor(Color::White);
    uiDisplayDescr->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    uiDisplayDescr->SetMultilineType(UIStaticText::MULTILINE_ENABLED);
    AddControl(uiDisplayDescr);

    OnDisplayConfigChanged();
    deviceManager->displayConfigChanged.Connect(this, &DeviceManagerTest::OnDisplayConfigChanged);
}

void DeviceManagerTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    RemoveAllControls();

    for (UIStaticText* x : uiDisplays)
    {
        SafeRelease(x);
    }
    uiDisplays.clear();
    SafeRelease(uiDisplayDescr);

    deviceManager->displayConfigChanged.Disconnect(this);
}

void DeviceManagerTest::OnDisplayConfigChanged()
{
    for (UIStaticText* x : uiDisplays)
    {
        RemoveControl(x);
        SafeRelease(x);
    }
    uiDisplays.clear();

    displays = deviceManager->GetDisplays();

    ScopedPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));
    DVASSERT(font);

    const float32 unitLength = 100.f;
    float32 scale = displays[0].rect.dx;

    int n = 0;
    Rect total;
    for (const DisplayInfo& di : displays)
    {
        Rect rc;
        rc.x = di.rect.x / scale * unitLength;
        rc.y = di.rect.y / scale * unitLength;
        rc.dx = di.rect.dx / scale * unitLength;
        rc.dy = di.rect.dy / scale * unitLength;
        total = total.Combine(rc);

        UIStaticText* ui = new UIStaticText(rc);
        ui->SetTextColor(Color::White);
        ui->SetFont(font);
        ui->SetFontSize(10.f);
        ui->GetOrCreateComponent<UIDebugRenderComponent>();
        ui->SetInputEnabled(true);
        UIControlBackground* uiBg = ui->GetOrCreateComponent<UIControlBackground>();
        uiBg->SetColor(Color(0, 0, 0.8f, 1));
        uiBg->SetDrawType(UIControlBackground::DRAW_FILL);
        ui->SetText(UTF8Utils::EncodeToWideString(Format("%d", n + 1)));
        ui->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        ui->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE,
                     Message(this, &DeviceManagerTest::OnDisplayClick,
                             reinterpret_cast<void*>(static_cast<intptr_t>(n))));
        uiDisplays.push_back(ui);

        n += 1;
    }

    float32 dx = 50.f;
    float32 dy = 50.f;
    if (total.x < 0.f)
        dx += -total.x;
    if (total.y < 0.f)
        dy += -total.y;

    for (UIStaticText* x : uiDisplays)
    {
        Rect rc = x->GetRect();
        rc.x += dx;
        rc.y += dy;
        x->SetRect(rc);
        AddControl(x);
    }

    total.x += dx;
    total.y += dy;

    Rect rcDescr;
    rcDescr.x = total.x;
    rcDescr.y = total.y + total.dy + 20;
    rcDescr.dx = 200;
    rcDescr.dy = 100;
    uiDisplayDescr->SetRect(rcDescr);
    uiDisplayDescr->SetFont(font);
    uiDisplayDescr->SetFontSize(14.f);
    uiDisplayDescr->SetText(L"");
}

void DeviceManagerTest::OnDisplayClick(DAVA::BaseObject*, void* args, void*)
{
    const Vector<DisplayInfo>& displays = deviceManager->GetDisplays();
    int i = static_cast<int>(reinterpret_cast<intptr_t>(args));
    if (0 <= i && i < static_cast<int>(displays.size()))
    {
        StringStream ss;
        ss << displays[i].name;
        if (displays[i].primary)
            ss << " (primary)";
        ss << std::endl;
        ss << "raw dpiX: " << displays[i].rawDpiX << std::endl;
        ss << "raw dpiY: " << displays[i].rawDpiY << std::endl;
        ss << "origin: " << displays[i].rect.x << ", " << displays[i].rect.y << std::endl;
        ss << "size: " << displays[i].rect.dx << " x " << displays[i].rect.dy << std::endl;
        ss << "maxFps: " << displays[i].maxFps;
        uiDisplayDescr->SetText(UTF8Utils::EncodeToWideString(ss.str()));
    }
}
