#pragma once

#include "Infrastructure/BaseScreen.h"

class TestBed;
class ImGuiTest : public BaseScreen
{
public:
    ImGuiTest(TestBed& app);

    void LoadResources() override;

protected:
    void Update(DAVA::float32 timeElapsed) override;
    void ShowEngineSettings();

    bool showTestWindow = true;
    bool showAnotherWindow = false;
    DAVA::Color backColor;
};
