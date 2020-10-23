#pragma once

#include "DAVAEngine.h"
#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;
class GPUTest : public BaseScreen
{
public:
    GPUTest(TestBed& app);

public:
    void LoadResources() override;
};
