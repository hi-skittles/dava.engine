#include "UI/Text/UITextSystem.h"

#include "Concurrency/Thread.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Engine/Engine.h"
#include "Entity/Component.h"
#include "Render/2D/FontManager.h"
#include "UI/Text/UITextComponent.h"
#include "UI/UIControl.h"
#include "UITextSystemLink.h"
#include "Utils/UTF8Utils.h"

namespace DAVA
{
UITextSystem::~UITextSystem()
{
    for (UITextComponent* component : components)
    {
        // System expect that all components already removed via UnregisterComponent and UnregisterControl.
        DVASSERT(component == nullptr, "Unexpected system state! List of links must be empty!");
    }
}

void UITextSystem::Process(float32 elapsedTime)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_TEXT_SYSTEM);

    // Remove empty links
    if (!components.empty())
    {
        components.erase(std::remove(components.begin(), components.end(), nullptr), components.end());
    }

    // Process links
    for (UITextComponent* component : components)
    {
        if (component != nullptr && component->IsModified())
        {
            ApplyData(component);
        }
    }
}

void UITextSystem::ApplyData(UITextComponent* component)
{
    if (component->IsModified())
    {
        UITextSystemLink* link = component->GetLink();
        UIControlBackground* textBg = link->GetTextBackground();
        UIControlBackground* shadowBg = link->GetShadowBackground();
        TextBlock* textBlock = link->GetTextBlock();

        UIControl* control = component->GetControl();
        DVASSERT(control, "Invalid control pointer!");

        component->SetModified(false);

        textBg->SetColorInheritType(component->GetColorInheritType());
        textBg->SetPerPixelAccuracyType(component->GetPerPixelAccuracyType());
        textBg->SetColor(component->GetColor());

        shadowBg->SetColorInheritType(component->GetColorInheritType());
        shadowBg->SetPerPixelAccuracyType(component->GetPerPixelAccuracyType());
        shadowBg->SetColor(component->GetShadowColor());

        textBlock->SetRectSize(control->size);

        switch (component->GetFitting())
        {
        default:
        case UITextComponent::eTextFitting::FITTING_NONE:
            textBlock->SetFittingOption(0);
            break;
        case UITextComponent::eTextFitting::FITTING_ENLARGE:
            textBlock->SetFittingOption(TextBlock::eFitType::FITTING_ENLARGE);
            break;
        case UITextComponent::eTextFitting::FITTING_REDUCE:
            textBlock->SetFittingOption(TextBlock::eFitType::FITTING_REDUCE);
            break;
        case UITextComponent::eTextFitting::FITTING_FILL:
            textBlock->SetFittingOption(TextBlock::eFitType::FITTING_REDUCE | TextBlock::eFitType::FITTING_ENLARGE);
            break;
        case UITextComponent::eTextFitting::FITTING_POINTS:
            textBlock->SetFittingOption(TextBlock::eFitType::FITTING_POINTS);
            break;
        }

        textBlock->SetText(UTF8Utils::EncodeToWideString(component->GetText()), component->GetRequestedTextRectSize());

        FontPreset preset;
        if (component->GetFont())
        {
            preset.SetFont(RefPtr<Font>::ConstructWithRetain(component->GetFont()));
        }
        else if (!component->GetFontPath().IsEmpty())
        {
            preset.SetFont(GetEngineContext()->fontManager->LoadFont(component->GetFontPath()));
        }
        else if (!component->GetFontName().empty())
        {
            preset = GetEngineContext()->fontManager->GetFontPreset(component->GetFontName());
        }

        if (!FLOAT_EQUAL(component->GetFontSize(), 0.f))
        {
            preset.SetSize(component->GetFontSize());
        }

        if (textBlock->GetFont() != preset.GetFontPtr() || !FLOAT_EQUAL(textBlock->GetFontSize(), preset.GetSize()))
        {
            textBlock->SetFont(preset.GetFontPtr());
            textBlock->SetFontSize(preset.GetSize());
            textBg->SetSprite(nullptr, 0);
            shadowBg->SetSprite(nullptr, 0);
        }

        switch (component->GetMultiline())
        {
        default:
        case UITextComponent::eTextMultiline::MULTILINE_DISABLED:
            textBlock->SetMultiline(false, false);
            break;
        case UITextComponent::eTextMultiline::MULTILINE_ENABLED:
            textBlock->SetMultiline(true, false);
            break;
        case UITextComponent::eTextMultiline::MULTILINE_ENABLED_BY_SYMBOL:
            textBlock->SetMultiline(true, true);
            break;
        }

        textBlock->SetAlign(component->GetAlign());
        textBlock->SetUseRtlAlign(component->GetUseRtlAlign());
        textBlock->SetForceBiDiSupportEnabled(component->IsForceBiDiSupportEnabled());

        if (textBlock->NeedCalculateCacheParams())
        {
            control->SetLayoutDirty();
        }
    }
}

void UITextSystem::ForceProcessControl(float32 elapsedTime, UIControl* control)
{
    UITextComponent* component = control->GetComponent<UITextComponent>();
    if (component)
    {
        ApplyData(component);
    }

    for (const auto& c : control->GetChildren())
    {
        ForceProcessControl(elapsedTime, c.Get());
    }
}

void UITextSystem::RegisterControl(UIControl* control)
{
    UISystem::RegisterControl(control);
    UITextComponent* component = control->GetComponent<UITextComponent>();
    if (component)
    {
        AddLink(component);
    }
}

void UITextSystem::UnregisterControl(UIControl* control)
{
    UITextComponent* component = control->GetComponent<UITextComponent>();
    if (component)
    {
        RemoveLink(component);
    }

    UISystem::UnregisterControl(control);
}

void UITextSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    UISystem::RegisterComponent(control, component);

    UITextComponent* textComponent = CastIfEqual<UITextComponent*>(component);
    if (textComponent != nullptr)
    {
        AddLink(textComponent);
    }
}

void UITextSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    UITextComponent* textComponent = CastIfEqual<UITextComponent*>(component);
    if (textComponent != nullptr)
    {
        RemoveLink(textComponent);
    }

    UISystem::UnregisterComponent(control, component);
}

void UITextSystem::AddLink(UITextComponent* component)
{
    DVASSERT(component);
    UITextSystemLink* link = component->GetLink();
    component->SetModified(true);
    components.push_back(component);
}

void UITextSystem::RemoveLink(UITextComponent* component)
{
    DVASSERT(component);

    auto findIt = std::find(components.begin(), components.end(), component);
    if (findIt != components.end())
    {
        (*findIt) = nullptr; // mark link for delete
    }
    else
    {
        DVASSERT(0 && "Text component link not found in system list!");
    }
}

void UITextSystem::InvalidateAll()
{
    for (UITextComponent* component : components)
    {
        if (component != nullptr)
        {
            component->SetModified(true);
        }
    }
}
}
