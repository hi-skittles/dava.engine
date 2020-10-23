#include "UI/UIHoleTransition.h"
#include "Engine/Engine.h"
#include "UI/UIControlSystem.h"
#include "Time/SystemTimer.h"
#include "Render/RenderHelper.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/Renderer.h"

namespace DAVA
{
UIHoleTransition::UIHoleTransition()
{
    duration = 0.8f;
}

UIHoleTransition::~UIHoleTransition()
{
}
//
//void UIHoleTransition::SetType(eType _type)
//{
//	type = _type;
//}

void UIHoleTransition::SetPolygon(const Polygon2& pts)
{
    clipPoly = pts;
    realPoly = pts;
}

void UIHoleTransition::Update(float32 timeElapsed)
{
    UIScreenTransition::Update(timeElapsed);
    normalizedTime = currentTime / duration;

    float scaleCoef = 1.0f;
    if (normalizedTime <= 0.5f)
        scaleCoef = interpolationFunc(1.0f - normalizedTime * 2.0f);
    else
        scaleCoef = interpolationFunc((normalizedTime - 0.5f) * 2.0f);

    for (int k = 0; k < clipPoly.pointCount; ++k)
    {
        realPoly.points[k] = clipPoly.points[k];
        realPoly.points[k] -= Vector2(GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dx / 2.0f,
                                      GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dy / 2.0f);
        realPoly.points[k] *= scaleCoef;
        realPoly.points[k] += Vector2(GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dx / 2.0f,
                                      GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dy / 2.0f);
    }
}

void UIHoleTransition::Draw(const UIGeometricData& geometricData)
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

    auto rect = Rect(0.0f, 0.0f, static_cast<float32>(GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dx), static_cast<float32>(GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dy));

    RenderSystem2D::Instance()->FillRect(rect, Color::Black);

    drawState.SetPosition(geometricData.position);

    if (normalizedTime < 0.5f)
    {
        renderTargetPrevScreen->SetClipPolygon(&realPoly);
        RenderSystem2D::Instance()->Draw(renderTargetPrevScreen, &drawState, Color::White);
    }
    else
    {
        renderTargetNextScreen->SetClipPolygon(&realPoly);
        RenderSystem2D::Instance()->Draw(renderTargetNextScreen, &drawState, Color::White);
    }

    /*Texture * tx = renderTargetPrevScreen->GetTexture();
	if (normalizedTime > 0.5f)
		tx = renderTargetNextScreen->GetTexture();
	
	for (int k = 0; k < clipPoly.pointCount; ++k)
	{
		texCoords[k] = Vector2(points[k].x / tx->width, points[k].y / tx->height);
	}
	
	RenderManager::Instance()->SetVertexPointer(3, TYPE_FLOAT, 0, points);
	RenderManager::Instance()->SetTexCoordPointer(2, TYPE_FLOAT, 0, texCoords);
	RenderManager::Instance()->SetTexture(tx);
	
	RenderManager::Instance()->FlushState();
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, clipPoly.pointCount);
	*/
    /*
	float32 startXPos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float32 startYPos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float32 endXPos[4] = {RenderManager::Instance()->ScreenWidth(), -RenderManager::ScreenWidth(), 0.0f, 0.0f};
	float32 endYPos[4] = {0.0f, 0.0f, RenderManager::ScreenHeight(), -RenderManager::ScreenHeight()};
	
	float32 xPrevPosition = (endXPos[type] - startXPos[type]) * normalizedTime;
	float32 yPrevPosition = (endYPos[type] - startYPos[type]) * normalizedTime;
	float32 xNextPosition = xPrevPosition - endXPos[type];
	float32 yNextPosition = yPrevPosition - endYPos[type];
	
	renderTargetPrevScreen->SetPosition(xPrevPosition, yPrevPosition);
	renderTargetPrevScreen->Draw();
	
	renderTargetNextScreen->SetPosition(xNextPosition, yNextPosition);
	renderTargetNextScreen->Draw(); */
}
};
