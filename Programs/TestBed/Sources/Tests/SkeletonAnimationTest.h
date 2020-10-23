#pragma once

#include "Infrastructure/BaseScreen.h"

namespace DAVA
{
class Scene;
}

class TestBed;
class SkeletonAnimationTest : public BaseScreen
{
public:
    SkeletonAnimationTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    DAVA::Scene* scene = nullptr;
};
