#pragma once

#include "Infrastructure/BaseScreen.h"

class TestBed;
class SpineTest : public BaseScreen
{
public:
    SpineTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
};
