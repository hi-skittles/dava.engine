#include "UI/UIMoveInTransition.h"
#include "Engine/Engine.h"
#include "UI/UIControlSystem.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/RenderHelper.h"
#include "Time/SystemTimer.h"

namespace DAVA
{
UIMoveInTransition::UIMoveInTransition()
{
    type = FROM_TOP;
    isOver = false;
}

UIMoveInTransition::~UIMoveInTransition()
{
}

void UIMoveInTransition::SetType(eType _type, bool moveOver)
{
    type = _type;
    isOver = moveOver;
}

void UIMoveInTransition::Update(float32 timeElapsed)
{
    UIScreenTransition::Update(timeElapsed);
}

void UIMoveInTransition::Draw(const UIGeometricData& geometricData)
{
    /*
	 renderTargetPrevScreen->SetScale(0.5f, 1.0f);
	 renderTargetPrevScreen->SetPosition(0, 0);
	 renderTargetPrevScreen->Draw();

	 renderTargetNextScreen->SetScale(0.5f, 1.0f);
	 renderTargetNextScreen->SetPosition(240, 0);
	 renderTargetNextScreen->Draw(); 

	 FROM_LEFT = 0, 
	 FROM_RIGHT,
	 FROM_TOP,
	 FROM_BOTTOM,
	 */

    SpriteDrawState drawState;
    drawState.SetMaterial(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL);

    if (type <= FROM_BOTTOM)
    {
        float32 endXPos[4] = { GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dx, -GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dx, 0.0f, 0.0f };
        float32 endYPos[4] = { 0.0f, 0.0f, GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dy, -GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dy };
        float32 xPrevPosition = endXPos[type] * normalizedTime;
        float32 yPrevPosition = endYPos[type] * normalizedTime;
        float32 xNextPosition = xPrevPosition - endXPos[type];
        float32 yNextPosition = yPrevPosition - endYPos[type];

        if (!isOver)
        {
            drawState.SetPosition(xPrevPosition, yPrevPosition);
        }
        else
        {
            drawState.SetPosition(0, 0);
        }
        RenderSystem2D::Instance()->Draw(renderTargetPrevScreen, &drawState, Color::White);

        drawState.SetPosition(xNextPosition, yNextPosition);
        RenderSystem2D::Instance()->Draw(renderTargetNextScreen, &drawState, Color::White);
    }
    else
    {
        float32 endXPos[4] = { GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dx, -GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dx, 0.0f, 0.0f };
        float32 endYPos[4] = { 0.0f, 0.0f, GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dy, -GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dy };
        float32 xPrevPosition = endXPos[type - 4] * normalizedTime;
        float32 yPrevPosition = endYPos[type - 4] * normalizedTime;
        float32 xNextPosition = xPrevPosition - endXPos[type - 4];
        float32 yNextPosition = yPrevPosition - endYPos[type - 4];

        if (!isOver)
        {
            drawState.SetPosition(xNextPosition, yNextPosition);
        }
        else
        {
            drawState.SetPosition(0, 0);
        }

        RenderSystem2D::Instance()->Draw(renderTargetNextScreen, &drawState, Color::White);

        drawState.SetPosition(xPrevPosition, yPrevPosition);
        RenderSystem2D::Instance()->Draw(renderTargetPrevScreen, &drawState, Color::White);
    }
}
};
