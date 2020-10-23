#pragma once

#include "Infrastructure/BaseScreen.h"

class TestBed;

namespace DAVA
{
class Window;
}

class SoundTest final : public BaseScreen
{
public:
    SoundTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnWindowFocusChanged(DAVA::Window* w, bool hasFocus);

    void OnPlaySoundGroup1(DAVA::BaseObject* sender, void* data, void* callerData);
    void OnPlaySoundGroup2(DAVA::BaseObject* sender, void* data, void* callerData);
    void OnApplySpeedGroup1(DAVA::BaseObject* sender, void* data, void* callerData);
    void OnApplySpeedGroup2(DAVA::BaseObject* sender, void* data, void* callerData);

    DAVA::UITextField* speedTextFieldGroup1 = nullptr;
    DAVA::UITextField* speedTextFieldGroup2 = nullptr;

    DAVA::SoundEvent* eventGroup1 = nullptr;
    DAVA::SoundEvent* eventGroup2 = nullptr;
};
