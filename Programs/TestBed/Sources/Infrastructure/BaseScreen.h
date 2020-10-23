#pragma once

#include "DAVAEngine.h"

class TestBed;
class BaseScreen : public DAVA::UIScreen
{
protected:
    virtual ~BaseScreen()
    {
    }

public:
    BaseScreen(TestBed& app, const DAVA::String& screenName);

    inline DAVA::int32 GetScreenId();

protected:
    void LoadResources() override;
    void UnloadResources() override;

    virtual void OnBackNavigation(DAVA::Window* window);
    virtual void OnExitButton(DAVA::BaseObject* obj, void* data, void* callerData);

    TestBed& app;

private:
    static DAVA::int32 globalScreenId; // 1, on create of screen increment

    DAVA::int32 currentScreenId;
    DAVA::UIButton* exitButton = nullptr;
};

DAVA::int32 BaseScreen::GetScreenId()
{
    return currentScreenId;
}
