#pragma once

#include <DAVAEngine.h>
#include <Render/2D/Font.h>

class BaseScreen : public DAVA::UIScreen
{
    static DAVA::int32 screensCount;

public:
    BaseScreen();

    void LoadResources() override;
    void UnloadResources() override;
    bool SystemInput(DAVA::UIEvent* currentInput) override;

    DAVA::int32 GetScreenID() const
    {
        return screenID;
    };

protected:
    DAVA::UIButton* CreateButton(const DAVA::Rect& rect, const DAVA::WideString& text);

    void SetPreviousScreen() const;
    void SetNextScreen() const;

    DAVA::ScopedPtr<DAVA::Font> font;
    DAVA::float32 fontSize = 20.f;
    DAVA::float32 fontSmallSize = 15.f;

    DAVA::int32 screenID;
};
