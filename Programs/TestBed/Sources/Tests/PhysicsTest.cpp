#include "Tests/PhysicsTest.h"
#include "Infrastructure/TestBed.h"

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
#include <Physics/PhysicsModule.h>
#include <Physics/Private/PhysicsMath.h>
#include <Physics/PhysicsConfigs.h>

#include <UI/Update/UIUpdateComponent.h>
#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <ModuleManager/ModuleManager.h>
#include <Math/Vector.h>

#include <physx/PxScene.h>
#include <physx/PxPhysics.h>
#include <physx/PxRigidActor.h>
#include <physx/PxActor.h>
#include <physx/pvd/PxPvdSceneClient.h>
#include <PxShared/foundation/PxMat44.h>
#include <PxShared/foundation/PxTransform.h>

PhysicsTest::PhysicsTest(TestBed& app)
    : BaseScreen(app, "PhysicsTest")
{
}

void PhysicsTest::LoadResources()
{
    GetOrCreateComponent<DAVA::UIUpdateComponent>();

    BaseScreen::LoadResources();
    const DAVA::EngineContext* ctx = app.GetEngine().GetContext();
    DAVA::PhysicsModule* physicsModule = ctx->moduleManager->GetModule<DAVA::PhysicsModule>();

    scene = physicsModule->CreateScene(DAVA::PhysicsSceneConfig(), physx::PxDefaultSimulationFilterShader, nullptr);
    physx::PxActor* actor = physicsModule->CreateStaticActor();
    physx::PxRigidStatic* staticActor = actor->is<physx::PxRigidStatic>();
    physx::PxShape* shape = physicsModule->CreateBoxShape(DAVA::Vector3(1.0f, 1.0f, 1.0f), DAVA::FastName(""));

    staticActor->attachShape(*shape);
    shape->release();

    scene->addActor(*actor);
    simulationBlock = physicsModule->Allocate(simulationBlockSize, "SimulationBlock", __FILE__, __LINE__);
}

void PhysicsTest::UnloadResources()
{
    const DAVA::EngineContext* ctx = app.GetEngine().GetContext();
    DAVA::PhysicsModule* physicsModule = ctx->moduleManager->GetModule<DAVA::PhysicsModule>();

    physicsModule->Deallocate(simulationBlock);
    scene->release();
    scene = nullptr;
    BaseScreen::UnloadResources();
}

void PhysicsTest::Update(DAVA::float32 timeElapsed)
{
    scene->simulate(timeElapsed, nullptr, simulationBlock, simulationBlockSize, true);
    physx::PxU32 errorState = 0;
    scene->fetchResults(true, &errorState);
    physx::PxPvdSceneClient* client = scene->getScenePvdClient();
    if (client != nullptr)
    {
        client->updateCamera(nullptr, physx::PxVec3(10.0f, 10.0f, 10.0f), physx::PxVec3(0.0f, 0.0f, 1.0f), physx::PxVec3(0.0f, 0.0f, 0.0f));
    }
}
#endif
