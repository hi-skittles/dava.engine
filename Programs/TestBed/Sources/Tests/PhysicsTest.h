#pragma once

#include "Infrastructure/BaseScreen.h"

#include <Base/BaseTypes.h>

class TestBed;

namespace physx
{
class PxScene;
class PxRigidActor;
}

class PhysicsTest : public BaseScreen
{
public:
    PhysicsTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

    void Update(DAVA::float32 timeElapsed) override;

private:
    physx::PxScene* scene = nullptr;
    void* simulationBlock = nullptr;
    DAVA::uint32 simulationBlockSize = 16 * 1024 * 512;
};
