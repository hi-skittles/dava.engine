#include "OverdrawTest.h"

#include "OverdrawTest/OverdrawTestingScreen.h"
#include "OverdrawTest/OverdrawTestConfig.h"
#include "Render/PixelFormatDescriptor.h"

namespace OverdrawPerformanceTester
{
const float32 resolutionButtonsXOffset = 10.0f;
const float32 resolutionButtonsYOffset = 100.0f;
const float32 buttonHeight = 40.0f;
const float32 buttonWidth = 120.0f;
const float32 heigthDistanceBetweenButtons = 10.0f;
const float32 texturePixelFormatXOffset = 150.0f;
const float32 texturePixelFormatYOffset = 100.0f;
const float32 overdrawXOffset = 290.0f;
const float32 overdrawYOffset = 100.0f;
const float32 chartHeightYOffset = overdrawYOffset + buttonHeight * 4;
const float32 minFrametimeThreshold = 0.033f;
const float32 frametimeIncreaseStep = 0.016f;
const uint16 testingScreenNumber = 1024; // Screen index must be above tests number. 1024 looks good.

const Array<OverdrawTest::ButtonInfo, 4> OverdrawTest::resolutionButtonsInfo =
{ {
{ L"2048", 1, DAVA::Rect(resolutionButtonsXOffset, resolutionButtonsYOffset + buttonHeight * 0 + heigthDistanceBetweenButtons * 0, buttonWidth, buttonHeight), 2048 },
{ L"1024", 2, DAVA::Rect(resolutionButtonsXOffset, resolutionButtonsYOffset + buttonHeight * 1 + heigthDistanceBetweenButtons * 1, buttonWidth, buttonHeight), 1024 },
{ L"512", 3, DAVA::Rect(resolutionButtonsXOffset, resolutionButtonsYOffset + buttonHeight * 2 + heigthDistanceBetweenButtons * 2, buttonWidth, buttonHeight), 512 },
{ L"256", 4, DAVA::Rect(resolutionButtonsXOffset, resolutionButtonsYOffset + buttonHeight * 3 + heigthDistanceBetweenButtons * 3, buttonWidth, buttonHeight), 256 }
} };

const Array<OverdrawTest::ButtonInfo, 9> OverdrawTest::texturePixelFormatButtonsInfo =
{ {
{ L"RGBA 8888", 1, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 0 + heigthDistanceBetweenButtons * 0, buttonWidth, buttonHeight), FORMAT_RGBA8888 },
{ L"RGBA 4444", 2, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 1 + heigthDistanceBetweenButtons * 1, buttonWidth, buttonHeight), FORMAT_RGBA4444 },
{ L"PVR 2", 3, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 2 + heigthDistanceBetweenButtons * 2, buttonWidth, buttonHeight), FORMAT_PVR2 },
{ L"PVR 4", 4, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 3 + heigthDistanceBetweenButtons * 3, buttonWidth, buttonHeight), FORMAT_PVR4 },
{ L"A8", 5, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 4 + heigthDistanceBetweenButtons * 4, buttonWidth, buttonHeight), FORMAT_A8 },
{ L"PVR2_2", 6, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 5 + heigthDistanceBetweenButtons * 5, buttonWidth, buttonHeight), FORMAT_PVR2_2 },
{ L"PVR4_2", 7, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 6 + heigthDistanceBetweenButtons * 6, buttonWidth, buttonHeight), FORMAT_PVR4_2 },
{ L"ETC1", 8, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 7 + heigthDistanceBetweenButtons * 7, buttonWidth, buttonHeight), FORMAT_ETC1 },
{ L"ETC2_RGBA", 9, DAVA::Rect(texturePixelFormatXOffset, texturePixelFormatYOffset + buttonHeight * 8 + heigthDistanceBetweenButtons * 8, buttonWidth, buttonHeight), FORMAT_ETC2_RGBA }
} };

const Array<OverdrawTest::ButtonInfo, 2> OverdrawTest::overdrawButtonsInfo =
{ {
{ L"-", 1, DAVA::Rect(overdrawXOffset, overdrawYOffset + buttonHeight, buttonWidth * 1.5f, buttonHeight), -1 },
{ L"+", 2, DAVA::Rect(overdrawXOffset + buttonWidth * 1.5f, overdrawYOffset + buttonHeight, buttonWidth * 1.5f, buttonHeight), 1 }
} };

const Array<OverdrawTest::ButtonInfo, 2> OverdrawTest::chartHeightButtonsInfo =
{ {
{ L"-", 1, DAVA::Rect(overdrawXOffset, chartHeightYOffset + buttonHeight, buttonWidth * 1.5f, buttonHeight), -1 },
{ L"+", 2, DAVA::Rect(overdrawXOffset + buttonWidth * 1.5f, chartHeightYOffset + buttonHeight, buttonWidth * 1.5f, buttonHeight), 1 }
} };

OverdrawTest::OverdrawTest(TestBed& app_)
    : BaseScreen(app_, "OverdrawTest")
{
    testingScreen = new OverdrawTestingScreen(app_);
    GetEngineContext()->uiScreenManager->RegisterScreen(testingScreenNumber, testingScreen);
}

OverdrawTest::~OverdrawTest()
{
    SafeRelease(testingScreen);
}

void OverdrawTest::LoadResources()
{
    BaseScreen::LoadResources();

    if (font == nullptr)
    {
        font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    }
    DAVA::Rect screenRect = GetRect();
    Size2i screenSize = DAVA::GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    screenRect.dx = static_cast<float32>(screenSize.dx);
    screenRect.dy = static_cast<float32>(screenSize.dy);
    SetRect(screenRect);

    startButton = CreateButton(DAVA::Rect(5, 5, screenRect.dx, buttonHeight), L"Start");
    startButton->GetOrCreateComponent<UIDebugRenderComponent>();
    startButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &OverdrawTest::OnStart));
    AddControl(startButton);
    bool isFormatSupported = DAVA::PixelFormatDescriptor::GetPixelFormatDescriptor(OverdrawTestConfig::pixelFormat).isHardwareSupported;
    startButton->SetVisibilityFlag(isFormatSupported);

    CreateLabel({ overdrawXOffset, overdrawYOffset - buttonHeight, buttonWidth * 3.0f, buttonHeight }, L"Overdraw screens count");
    overdrawCountLabel = new UIStaticText(DAVA::Rect(overdrawXOffset, overdrawYOffset, buttonWidth * 3.0f, buttonHeight));
    overdrawCountLabel->SetFont(font);
    overdrawCountLabel->SetFontSize(17.f);
    overdrawCountLabel->SetTextColor(Color::White);
    overdrawCountLabel->SetText(Format(L"%d", OverdrawTestConfig::overdrawScreensCount));
    overdrawCountLabel->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    overdrawCountLabel->GetOrCreateComponent<UIDebugRenderComponent>();
    AddControl(overdrawCountLabel);
    InitializeButtons(overdrawButtonsInfo, overdrawButtons, Message(this, &OverdrawTest::OnChangeOverdrawButtonClick), false);

    CreateLabel({ overdrawXOffset, chartHeightYOffset - buttonHeight, buttonWidth * 3.0f, buttonHeight }, L"Max frametime");
    chartHeightLabel = new UIStaticText(DAVA::Rect(overdrawXOffset, chartHeightYOffset, buttonWidth * 3.0f, buttonHeight));
    chartHeightLabel->SetFont(font);
    chartHeightLabel->SetFontSize(17.f);
    chartHeightLabel->SetTextColor(Color::White);
    chartHeightLabel->SetText(Format(L"%.3f", OverdrawTestConfig::chartHeight));
    chartHeightLabel->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    chartHeightLabel->GetOrCreateComponent<UIDebugRenderComponent>();
    AddControl(chartHeightLabel);
    InitializeButtons(chartHeightButtonsInfo, chartHeightButtons, Message(this, &OverdrawTest::OnChangeChartHeightButtonClick), false);

    CreateLabel({ resolutionButtonsXOffset, resolutionButtonsYOffset - buttonHeight, buttonWidth, buttonHeight }, L"Tex resolution");
    InitializeButtons(resolutionButtonsInfo, resolutionButtons, Message(this, &OverdrawTest::OnResolutionButtonClick));

    CreateLabel({ texturePixelFormatXOffset, texturePixelFormatYOffset - buttonHeight, buttonWidth, buttonHeight }, L"Tex format");
    InitializeButtons(texturePixelFormatButtonsInfo, texturePixelFormatButtons, Message(this, &OverdrawTest::OnTextureFormatButtonClick));

    GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(false);
}

void OverdrawTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    ReleaseButtons(resolutionButtons);
    ReleaseButtons(texturePixelFormatButtons);
    ReleaseButtons(overdrawButtons);
    ReleaseButtons(chartHeightButtons);
    SafeRelease(startButton);

    SafeRelease(overdrawCountLabel);
    SafeRelease(chartHeightLabel);

    SafeRelease(font);
}

void OverdrawTest::CreateLabel(const DAVA::Rect&& rect, const WideString&& caption)
{
    ScopedPtr<UIStaticText> label(new UIStaticText(rect));
    label->SetFont(font);
    label->SetFontSize(17.f);
    label->SetTextColor(Color::White);
    label->SetText(caption);
    label->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    label->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(false);
    AddControl(label);
}

void OverdrawTest::ReleaseButtons(DAVA::UnorderedMap<UIButton*, ButtonInfo>& buttons)
{
    for (auto& btn : buttons)
    {
        UIButton* buttonToDelete = btn.first;
        SafeRelease(buttonToDelete);
    }
    buttons.clear();
}

void OverdrawTest::OnStart(BaseObject* caller, void* param, void* callerData)
{
    GetEngineContext()->uiScreenManager->SetScreen(testingScreenNumber);
}

void OverdrawTest::OnResolutionButtonClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* pickedButton = DynamicTypeCheck<UIButton*>(sender);
    for (auto& btn : resolutionButtons)
    {
        if (btn.first->GetTag() == pickedButton->GetTag())
        {
            btn.first->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color::Green);
            OverdrawTestConfig::textureResolution = btn.second.data;
        }
        else
            btn.first->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color::Red);
    }
}

void OverdrawTest::OnTextureFormatButtonClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* pickedButton = DynamicTypeCheck<UIButton*>(sender);

    for (auto& btn : texturePixelFormatButtons)
    {
        if (btn.first->GetTag() == pickedButton->GetTag())
        {
            btn.first->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color::Green);
            OverdrawTestConfig::pixelFormat = static_cast<DAVA::PixelFormat>(btn.second.data);

            bool isFormatSupported = DAVA::PixelFormatDescriptor::GetPixelFormatDescriptor(OverdrawTestConfig::pixelFormat).isHardwareSupported;
            startButton->SetVisibilityFlag(isFormatSupported);
        }
        else
            btn.first->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color::Red);
    }
}

void OverdrawTest::OnChangeOverdrawButtonClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* pickedButton = DynamicTypeCheck<UIButton*>(sender);

    for (auto& btn : overdrawButtons)
    {
        if (btn.first->GetTag() == pickedButton->GetTag())
        {
            OverdrawTestConfig::overdrawScreensCount = Max(static_cast<uint8>(1), static_cast<uint8>(OverdrawTestConfig::overdrawScreensCount + btn.second.data));
            overdrawCountLabel->SetText(Format(L"%d", OverdrawTestConfig::overdrawScreensCount));
        }
    }
}

void OverdrawTest::OnChangeChartHeightButtonClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* pickedButton = DynamicTypeCheck<UIButton*>(sender);

    for (auto& btn : overdrawButtons)
    {
        if (btn.first->GetTag() == pickedButton->GetTag())
        {
            OverdrawTestConfig::chartHeight = Max(minFrametimeThreshold, OverdrawTestConfig::chartHeight + btn.second.data * frametimeIncreaseStep);
            chartHeightLabel->SetText(Format(L"%.3f", OverdrawTestConfig::chartHeight));
        }
    }
}

DAVA::UIButton* OverdrawTest::CreateButton(const DAVA::Rect& rect, const WideString& text)
{
    if (font == nullptr)
    {
        font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    }

    UIButton* button = new UIButton(rect);
    button->SetStateText(UIControl::STATE_NORMAL, text);
    button->SetStateTextAlign(UIControl::STATE_NORMAL, ALIGN_HCENTER | ALIGN_VCENTER);
    button->SetStateFont(UIControl::STATE_NORMAL, font);
    button->SetStateFontSize(0xFF, 17.f);
    button->SetStateFontColor(UIControl::STATE_NORMAL, Color::White);
    button->SetStateFontColor(UIControl::STATE_PRESSED_INSIDE, Color(0.7f, 0.7f, 0.7f, 1.f));

    return button;
}
}
