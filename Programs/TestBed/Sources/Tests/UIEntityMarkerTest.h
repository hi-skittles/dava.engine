#pragma once

#include "Infrastructure/BaseScreen.h"

class TestBed;
class UIEntityMarkerTest : public BaseScreen
{
public:
    UIEntityMarkerTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
};
