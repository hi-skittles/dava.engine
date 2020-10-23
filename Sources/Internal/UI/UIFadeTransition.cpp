#include "UI/UIFadeTransition.h"
#include "Time/SystemTimer.h"
#include "UI/UIControlSystem.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/Renderer.h"
#include "Render/RenderHelper.h"

namespace DAVA
{
UIFadeTransition::UIFadeTransition()
{
    type = FADE_MIX;
}

UIFadeTransition::~UIFadeTransition()
{
}

void UIFadeTransition::SetType(eType _type)
{
    type = _type;
}

void UIFadeTransition::Update(float32 timeElapsed)
{
    UIScreenTransition::Update(timeElapsed);
}

void UIFadeTransition::Draw(const UIGeometricData& geometricData)
{
    /*
	 renderTargetPrevScreen->SetScale(0.5f, 1.0f);
	 renderTargetPrevScreen->SetPosition(0, 0);
	 renderTargetPrevScreen->Draw();

	 renderTargetNextScreen->SetScale(0.5f, 1.0f);
	 renderTargetNextScreen->SetPosition(240, 0);
	 renderTargetNextScreen->Draw(); 
	 */
    SpriteDrawState drawState;
    drawState.SetMaterial(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL);
    if (type == FADE_MIX)
    {
        renderTargetPrevScreen->Reset();
        drawState.SetPosition(geometricData.position);
        RenderSystem2D::Instance()->Draw(renderTargetPrevScreen, &drawState, Color::White);

        renderTargetNextScreen->Reset();
        drawState.SetPosition(geometricData.position);
        RenderSystem2D::Instance()->Draw(renderTargetNextScreen, &drawState, Color(1.0f, 1.0f, 1.0f, normalizedTime));
    }
    else if (type == FADE_IN_FADE_OUT)
    {
        if (normalizedTime <= 0.5f)
        {
            drawState.SetPosition(0, 0);
            RenderSystem2D::Instance()->Draw(renderTargetPrevScreen, &drawState, Color(1.0f - normalizedTime * 2, 1.0f - normalizedTime * 2, 1.0f - normalizedTime * 2, 1.0f));
        }
        else
        {
            drawState.SetPosition(0, 0);
            RenderSystem2D::Instance()->Draw(renderTargetNextScreen, &drawState, Color((normalizedTime - 0.5f) * 2, (normalizedTime - 0.5f) * 2, (normalizedTime - 0.5f) * 2, 1.0f));
        }
    }
}
};
