#include "UI/UIScreenTransition.h"
#include "Engine/Engine.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Render/RenderHelper.h"
#include "Render/Texture.h"
#include "Scene3D/Scene.h"
#include "Time/SystemTimer.h"
#include "UI/Render/UIRenderSystem.h"
#include "UI/UI3DView.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreenshoter.h"
#include "UI/Update/UIUpdateComponent.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIScreenTransition)
{
    ReflectionRegistrator<UIScreenTransition>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIScreenTransition* o) { o->Release(); })
    .End();
}

UIScreenTransition::UIScreenTransition()
{
    interpolationFunc = Interpolation::GetFunction(Interpolation::EASY_IN_EASY_OUT);
    GetOrCreateComponent<UIUpdateComponent>();
}

UIScreenTransition::~UIScreenTransition()
{
    DVASSERT(renderTargetPrevScreen == nullptr && renderTargetNextScreen == nullptr);
}

void UIScreenTransition::CreateRenderTargets()
{
    if (renderTargetPrevScreen || renderTargetNextScreen)
    {
        Logger::FrameworkDebug("Render targets already created");
        return;
    }

    VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;

    Size2i physicalTargetSize = vcs->GetPhysicalScreenSize();

    uint32 width = physicalTargetSize.dx;
    uint32 height = physicalTargetSize.dy;

    Texture* tex1 = Texture::CreateFBO(width, height, rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R5G6B5) ? FORMAT_RGB565 : FORMAT_RGBA8888, true);
    Texture* tex2 = Texture::CreateFBO(width, height, rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R5G6B5) ? FORMAT_RGB565 : FORMAT_RGBA8888, true);

    renderTargetPrevScreen = Sprite::CreateFromTexture(tex1, 0, 0, static_cast<float32>(width), static_cast<float32>(height), true);
    renderTargetNextScreen = Sprite::CreateFromTexture(tex2, 0, 0, static_cast<float32>(width), static_cast<float32>(height), true);

    SafeRelease(tex1);
    SafeRelease(tex2);
}

void UIScreenTransition::ReleaseRenderTargets()
{
    SafeRelease(renderTargetPrevScreen);
    SafeRelease(renderTargetNextScreen);
}

void UIScreenTransition::SetSourceScreen(UIControl* prevScreen, bool updateScreen)
{
    SetSourceControl(prevScreen, updateScreen);
}

void UIScreenTransition::SetDestinationScreen(UIControl* nextScreen, bool updateScreen)
{
    SetDestinationControl(nextScreen, updateScreen);
}

void UIScreenTransition::StartTransition()
{
    currentTime = 0.0f;
    complete = false;

    CreateRenderTargets();
}

void UIScreenTransition::SetSourceControl(UIControl* prevControl, bool updateControl)
{
    DVASSERT(renderTargetPrevScreen && renderTargetNextScreen);

    UIScreenshoter* screenshoter = GetEngineContext()->uiControlSystem->GetRenderSystem()->GetScreenshoter();
    screenshoter->MakeScreenshot(prevControl, renderTargetPrevScreen->GetTexture(), true, updateControl);
}

void UIScreenTransition::SetDestinationControl(UIControl* nextControl, bool updateControl /*= true*/)
{
    DVASSERT(renderTargetPrevScreen && renderTargetNextScreen);

    UIScreenshoter* screenshoter = GetEngineContext()->uiControlSystem->GetRenderSystem()->GetScreenshoter();
    screenshoter->MakeScreenshot(nextControl, renderTargetNextScreen->GetTexture(), true, updateControl);
}

void UIScreenTransition::EndTransition()
{
    ReleaseRenderTargets();
}

void UIScreenTransition::Update(float32 timeElapsed)
{
    UIControl::Update(timeElapsed);

    currentTime += timeElapsed;
    normalizedTime = interpolationFunc(currentTime / duration);
    if (currentTime >= duration)
    {
        currentTime = duration;
        complete = true;
    }
}

void UIScreenTransition::Draw(const UIGeometricData& geometricData)
{
    if (renderTargetPrevScreen && renderTargetNextScreen)
    {
        SpriteDrawState drawState;
        drawState.SetMaterial(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL);

        drawState.SetScale(0.5f, 1.0f);
        drawState.SetPosition(0, 0);

        RenderSystem2D::Instance()->Draw(renderTargetPrevScreen, &drawState, Color::White);

        drawState.SetScale(0.5f, 1.0f);
        drawState.SetPosition((GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dx) / 2.0f, 0);

        RenderSystem2D::Instance()->Draw(renderTargetNextScreen, &drawState, Color::White);
    }
}

void UIScreenTransition::SetDuration(float32 timeInSeconds)
{
    duration = timeInSeconds;
};

bool UIScreenTransition::IsComplete() const
{
    return complete;
}
};
