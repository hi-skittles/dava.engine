#pragma once

#include "Infrastructure/BaseScreen.h"
#include <UI/Render/UIDebugRenderComponent.h>

namespace OverdrawPerformanceTester
{
class OverdrawTestingScreen;

using namespace DAVA;

class OverdrawTest : public BaseScreen
{
public:
    OverdrawTest(TestBed& app_);

    virtual void LoadResources();
    virtual void UnloadResources();

    virtual ~OverdrawTest();

private:
    struct ButtonInfo
    {
        WideString caption;
        int32 tag;
        Rect rect;
        int16 data;
    };

    void CreateLabel(const Rect&& rect, const WideString&& caption);
    void ReleaseButtons(UnorderedMap<UIButton*, ButtonInfo>& buttons);
    void OnStart(BaseObject* caller, void* param, void* callerData);
    void OnResolutionButtonClick(BaseObject* sender, void* data, void* callerData);
    void OnTextureFormatButtonClick(BaseObject* sender, void* data, void* callerData);
    void OnChangeOverdrawButtonClick(BaseObject* sender, void* data, void* callerData);
    void OnChangeChartHeightButtonClick(BaseObject* sender, void* data, void* callerData);
    UIButton* CreateButton(const Rect& rect, const WideString& text);

    template <std::size_t arrSize>
    void InitializeButtons(const Array<ButtonInfo, arrSize>& buttonsInfo, UnorderedMap<UIButton*, ButtonInfo>& buttonsMap, Message&& msg,
                           bool isFirstButtonGreen = true);

    UIStaticText* overdrawCountLabel = nullptr;
    UIStaticText* chartHeightLabel = nullptr;
    Font* font = nullptr;
    UIButton* startButton = nullptr;
    OverdrawTestingScreen* testingScreen = nullptr;

    UnorderedMap<UIButton*, ButtonInfo> resolutionButtons;
    UnorderedMap<UIButton*, ButtonInfo> texturePixelFormatButtons;
    UnorderedMap<UIButton*, ButtonInfo> overdrawButtons;
    UnorderedMap<UIButton*, ButtonInfo> chartHeightButtons;

    static const Array<ButtonInfo, 4> resolutionButtonsInfo;
    static const Array<ButtonInfo, 9> texturePixelFormatButtonsInfo;
    static const Array<ButtonInfo, 2> overdrawButtonsInfo;
    static const Array<ButtonInfo, 2> chartHeightButtonsInfo;
};

template <std::size_t arrSize>
inline void OverdrawTest::InitializeButtons(const Array<ButtonInfo, arrSize>& buttonsInfo, UnorderedMap<UIButton*, ButtonInfo>& buttonsMap, Message&& msg, bool isFirstButtonGreen)
{
    for (size_t i = 0; i < buttonsInfo.size(); i++)
    {
        UIButton* btn = CreateButton(buttonsInfo[i].rect, buttonsInfo[i].caption);
        btn->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, msg);
        UIDebugRenderComponent* debugCmp = btn->GetOrCreateComponent<UIDebugRenderComponent>();
        if (isFirstButtonGreen && i == 0)
            debugCmp->SetDrawColor(Color::Green);
        btn->SetTag(buttonsInfo[i].tag);
        AddControl(btn);
        buttonsMap[btn] = buttonsInfo[i];
    }
}
}
