#pragma once

#include "Infrastructure/BaseScreen.h"

class TestBed;
class UIStylesTest : public BaseScreen
{
public:
    UIStylesTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
    void Update(DAVA::float32 delta) override;

private:
    DAVA::RefPtr<DAVA::UIControl> proto;
    DAVA::RefPtr<DAVA::UIControl> container;
    DAVA::RefPtr<DAVA::UIStaticText> statusText;
};
