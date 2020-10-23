#include "Tests/SkeletonAnimationTest.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "UI/UIStaticText.h"
#include "UI/UIControlSystem.h"
#include "UI/Render/UIDebugRenderComponent.h"
#include "Scene3D/Scene.h"
#include "Engine/Engine.h"

using namespace DAVA;

SkeletonAnimationTest::SkeletonAnimationTest(TestBed& app)
    : BaseScreen(app, "SkeletonAnimationTest")
{
}

void SkeletonAnimationTest::LoadResources()
{
    Size2i screenSize = DAVA::GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    Rect screenRect = Rect(0.f, 0.f, float32(screenSize.dx), float32(screenSize.dy));
    SetRect(screenRect);

    scene = new Scene();
    scene->LoadScene("~res:/TestBed/3d/animations/MotusMan/MotusMan_Run.sc2");

    ScopedPtr<Camera> camera(new Camera());
    float32 aspect = float32(screenSize.dy) / float32(screenSize.dx);
    camera->SetupPerspective(70.f, aspect, 0.5f, 2500.f);
    camera->SetPosition(Vector3(-40.f, -80.f, 70.f));
    camera->SetTarget(Vector3(0.f, 0.f, 30.f));
    camera->SetUp(Vector3(0.f, 0.f, 1.f));

    scene->AddCamera(camera);
    scene->SetCurrentCamera(camera);

    Rect viewRect = screenRect;
    viewRect.dx *= 0.7f;
    viewRect.dy *= 0.7f;
    ScopedPtr<UI3DView> sceneView(new UI3DView(viewRect));
    sceneView->SetScene(scene);
    sceneView->GetOrCreateComponent<UIDebugRenderComponent>();
    AddControl(sceneView);

    BaseScreen::LoadResources();
}

void SkeletonAnimationTest::UnloadResources()
{
    SafeRelease(scene);

    BaseScreen::UnloadResources();
}
