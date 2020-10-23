#include "Tests/SpineTest.h"

#include "Infrastructure/TestBed.h"

#include <Base/RefPtr.h>
#include <UI/Render/UIDebugRenderComponent.h>
#include <UI/UIControl.h>
#include <UI/UIControlBackground.h>

#include <UI/Spine/UISpineComponent.h>

using namespace DAVA;

SpineTest::SpineTest(TestBed& app)
    : BaseScreen(app, "SpineTest")
{
}

void SpineTest::LoadResources()
{
    BaseScreen::LoadResources();

    RefPtr<UIControl> ctrl(new UIControl(Rect(200, 200, 50, 50)));
    ctrl->SetPivot(Vector2(0.5f, 0.5f));

    UIDebugRenderComponent* debug = ctrl->GetOrCreateComponent<UIDebugRenderComponent>();
    debug->SetEnabled(true);
    debug->SetPivotPointDrawMode(UIDebugRenderComponent::DRAW_ALWAYS);

    UIControlBackground* bg = ctrl->GetOrCreateComponent<UIControlBackground>();
    if (bg)
    {
        bg->SetDrawType(UIControlBackground::DRAW_BATCH);
    }

    UISpineComponent* sc = ctrl->GetOrCreateComponent<UISpineComponent>();
    if (sc)
    {
        sc->SetSkeletonPath("~res:/TestBed/UI/Spine/SpineTest.json");
        sc->SetAtlasPath("~res:/TestBed/UI/Spine/SpineTest.atlas");
        sc->SetAnimationName("position");
        sc->SetSkinName("gold");
        sc->SetAnimationState(UISpineComponent::PLAYED);
        sc->SetLoopedPlayback(true);
    }

    AddControl(ctrl.Get());
}

void SpineTest::UnloadResources()
{
    BaseScreen::UnloadResources();
    //TODO: Release resources here
}
