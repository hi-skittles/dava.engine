#include "UI/Flow/Private/UIFlowTransitionEffect.h"
#include "Render/2D/Sprite.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/Texture.h"
#include "UI/Render/UIRenderSystem.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreenshoter.h"

namespace DAVA
{
UIFlowTransitionEffect::UIFlowTransitionEffect() = default;

UIFlowTransitionEffect::UIFlowTransitionEffect(const UIFlowTransitionEffectConfig& config)
    : config(config)
{
}

UIFlowTransitionEffect::~UIFlowTransitionEffect() = default;

bool UIFlowTransitionEffect::IsFinish() const
{
    return position >= config.duration;
}

void UIFlowTransitionEffect::Start()
{
    position = 0.f;
}

void UIFlowTransitionEffect::Stop()
{
    position = config.duration;
}

void UIFlowTransitionEffect::MakePrevShot(UIRenderSystem* renderSystem, UIControl* ctrl)
{
    RefPtr<Texture> tex = renderSystem->GetScreenshoter()->MakeScreenshot(ctrl, PixelFormat::FORMAT_RGBA8888, true, true);
    prevShot = Sprite::CreateFromTexture(tex.Get(), 0, 0, ctrl->GetSize().dx, ctrl->GetSize().dy, false);
}

void UIFlowTransitionEffect::MakeNextShot(UIRenderSystem* renderSystem, UIControl* ctrl)
{
    RefPtr<Texture> tex = renderSystem->GetScreenshoter()->MakeScreenshot(ctrl, PixelFormat::FORMAT_RGBA8888, true, true);
    nextShot = Sprite::CreateFromTexture(tex.Get(), 0, 0, ctrl->GetSize().dx, ctrl->GetSize().dy, false);
}

void UIFlowTransitionEffect::Process(float32 delta)
{
    if (position < config.duration)
    {
        position += delta;
    }
}

void UIFlowTransitionEffect::Render(UIRenderSystem* renderSystem)
{
    float32 progress = std::min(std::max(position / config.duration, 0.f), 1.f);

    SpriteDrawState drawState;
    drawState.SetMaterial(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL);
    Color drawColor;
    Vector2 size;

    if (config.effectOut != UIFlowTransitionEffectConfig::None)
    {
        // Prev
        size = prevShot.Get()->GetSize();
        drawColor = Color::White;
        drawState.SetPosition(0.f, 0.f);
        drawState.SetScale(1.f, 1.f);
        switch (config.effectOut)
        {
        case UIFlowTransitionEffectConfig::Static:
            break;
        case UIFlowTransitionEffectConfig::FadeAlpha:
            drawColor.a = 1.f - progress;
            break;
        case UIFlowTransitionEffectConfig::Fade:
            drawColor.r = drawColor.g = drawColor.b = 1.f - progress;
            break;
        case UIFlowTransitionEffectConfig::Scale:
            drawState.SetPosition(size * 0.5 * progress);
            drawState.SetScale(1.f - progress, 1.f - progress);
            break;
        case UIFlowTransitionEffectConfig::Flip:
            drawState.SetScale(progress <= 0.5 ? (1.f - progress * 2.f) : 0.f, 1.f);
            drawState.SetPosition(size.dx * progress, 0.f);
            break;
        case UIFlowTransitionEffectConfig::MoveLeft:
            drawState.SetPosition(-size.dx * progress, 0.f);
            break;
        case UIFlowTransitionEffectConfig::MoveRight:
            drawState.SetPosition(size.dx * progress, 0.f);
            break;
        default:
            break;
        }
        renderSystem->GetRenderSystem2D()->Draw(prevShot.Get(), &drawState, drawColor);
    }

    if (config.effectIn != UIFlowTransitionEffectConfig::None)
    {
        // Next
        size = nextShot.Get()->GetSize();
        drawColor = Color::White;
        drawState.SetPosition(0.f, 0.f);
        drawState.SetScale(1.f, 1.f);
        switch (config.effectIn)
        {
        case UIFlowTransitionEffectConfig::Static:
            break;
        case UIFlowTransitionEffectConfig::FadeAlpha:
            drawColor.a = progress;
            break;
        case UIFlowTransitionEffectConfig::Fade:
            drawColor.r = drawColor.g = drawColor.b = progress;
            break;
        case UIFlowTransitionEffectConfig::Scale:
            drawState.SetPosition(size * 0.5 * (1.f - progress));
            drawState.SetScale(progress, progress);
            break;
        case UIFlowTransitionEffectConfig::Flip:
            drawState.SetScale(progress >= 0.5 ? ((progress - 0.5f) * 2.f) : 0.f, 1.f);
            drawState.SetPosition(size.dx * (1.f - progress), 0.f);
            break;
        case UIFlowTransitionEffectConfig::MoveLeft:
            drawState.SetPosition(-size.dx * (1.f - progress), 0.f);
            break;
        case UIFlowTransitionEffectConfig::MoveRight:
            drawState.SetPosition(size.dx * (1.f - progress), 0.f);
            break;

        default:
            break;
        }
        renderSystem->GetRenderSystem2D()->Draw(nextShot.Get(), &drawState, drawColor);
    }
}

void UIFlowTransitionEffect::Reverse()
{
    std::swap(config.effectIn, config.effectOut);
}
}
