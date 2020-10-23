#include "UI/RichContent/UIRichContentSystem.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "FileSystem/XMLParser.h"
#include "Logger/Logger.h"
#include "Render/RenderOptions.h"
#include "Render/Renderer.h"
#include "UI/RichContent/Private/RichStructs.h"
#include "UI/RichContent/Private/XMLAliasesBuilder.h"
#include "UI/RichContent/Private/XMLRichContentBuilder.h"
#include "UI/RichContent/UIRichContentAliasesComponent.h"
#include "UI/RichContent/UIRichContentComponent.h"
#include "UI/Components/UIControlSourceComponent.h"
#include "UI/UIControl.h"

namespace DAVA
{
UIRichContentSystem::UIRichContentSystem()
{
    Engine* engine = Engine::Instance();
    engine->windowCreated.Connect([&](Window*) {
        RenderOptions* options = Renderer::IsInitialized() ? Renderer::GetOptions() : nullptr;
        if (options)
        {
            isDebugDraw = options->IsOptionEnabled(RenderOptions::DEBUG_DRAW_RICH_ITEMS);
            options->AddObserver(this);
        }
    });
    engine->windowDestroyed.Connect([&](Window*) {
        RenderOptions* options = Renderer::IsInitialized() ? Renderer::GetOptions() : nullptr;
        if (options)
        {
            options->RemoveObserver(this);
        }
    });
}

UIRichContentSystem::~UIRichContentSystem()
{
    RenderOptions* options = Renderer::IsInitialized() ? Renderer::GetOptions() : nullptr;
    if (options)
    {
        options->RemoveObserver(this);
    }
}

void UIRichContentSystem::RegisterControl(UIControl* control)
{
    UISystem::RegisterControl(control);
    UIRichContentComponent* component = control->GetComponent<UIRichContentComponent>();
    if (component)
    {
        AddLink(component);
    }
}

void UIRichContentSystem::UnregisterControl(UIControl* control)
{
    UIRichContentComponent* component = control->GetComponent<UIRichContentComponent>();
    if (component)
    {
        RemoveLink(component);
    }

    UISystem::UnregisterControl(control);
}

void UIRichContentSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    UISystem::RegisterComponent(control, component);

    UIRichContentComponent* richContent = CastIfEqual<UIRichContentComponent*>(component);
    if (richContent != nullptr)
    {
        AddLink(richContent);
    }

    UIRichContentAliasesComponent* richContentAliases = CastIfEqual<UIRichContentAliasesComponent*>(component);
    if (richContentAliases != nullptr)
    {
        AddAliases(control, richContentAliases);
    }
}

void UIRichContentSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    UIRichContentComponent* richContent = CastIfEqual<UIRichContentComponent*>(component);
    if (richContent != nullptr)
    {
        RemoveLink(richContent);
    }

    UIRichContentAliasesComponent* richContentAliases = CastIfEqual<UIRichContentAliasesComponent*>(component);
    if (richContentAliases != nullptr)
    {
        RemoveAliases(control, richContentAliases);
    }

    UISystem::UnregisterComponent(control, component);
}

XMLParserStatus ValidateAliasName(const String& alias)
{
    if (alias.find_first_of("\t\n ") != String::npos)
    {
        XMLParserStatus status;
        status.code = -1;
        status.errorMessage = "Alias contains white-space character(s)";
        return status;
    }
    return XMLParser::ParseStringEx("<" + alias + "/>", nullptr);
}

void UIRichContentSystem::Process(float32 elapsedTime)
{
    // Add new links
    if (!appendLinks.empty())
    {
        links.insert(links.end(), appendLinks.begin(), appendLinks.end());
        appendLinks.clear();
    }
    // Remove empty links
    if (!links.empty())
    {
        links.erase(std::remove_if(links.begin(), links.end(), [](std::shared_ptr<RichContentLink>& l) {
                        return l->component == nullptr;
                    }),
                    links.end());
    }

    // Process links
    for (std::shared_ptr<RichContentLink>& l : links)
    {
        if (l->component)
        {
            bool update = l->component->IsModified();
            l->component->SetModified(false);

            for (RichContentAliasesLink& alink : l->aliasesLinks)
            {
                if (alink.component && alink.component->IsModified())
                {
                    update = true;

                    onBeginProcessComponent.Emit(alink.component);

                    alink.RemoveAll();
                    for (const auto& pair : alink.component->GetAliases())
                    {
                        XMLParserStatus parserStatus = ValidateAliasName(pair.first);
                        if (parserStatus.Success())
                        {
                            XMLAliasesBuilder builder(pair.first);
                            parserStatus = builder.Build(pair.second);
                            if (parserStatus.Success())
                            {
                                alink.PutAlias(builder.GetAlias());
                            }
                            else
                            {
                                const String message = Format("Syntax error in rich content alias `%s` source: %s (%d:%d)", pair.first.c_str(), parserStatus.errorMessage.c_str(), parserStatus.errorLine, parserStatus.errorPosition);
                                onAliasXMLParsingError.Emit(alink.component, pair.first, message);
                                Logger::Error(message.c_str());
                            }
                        }
                        else
                        {
                            const String message = Format("Wrong rich content alias `%s` name : %s", pair.first.c_str(), parserStatus.errorMessage.c_str());
                            onAliasXMLParsingError.Emit(alink.component, pair.first, message);
                            Logger::Error(message.c_str());
                        }
                    }

                    onEndProcessComponent.Emit(alink.component);

                    alink.component->SetModified(false);
                }
            }

            if (update)
            {
                onBeginProcessComponent.Emit(l->component);

                UIControl* root = l->component->GetControl();
                l->RemoveItems();

                XMLRichContentBuilder builder(l.get(), isEditorMode, isDebugDraw);
                XMLParserStatus parserStatus = builder.Build("<span>" + l->component->GetText() + "</span>");
                if (parserStatus.Success())
                {
                    for (const RefPtr<UIControl>& ctrl : builder.GetControls())
                    {
                        root->AddControl(ctrl.Get());
                        l->AddItem(ctrl);
                    }
                }
                else
                {
                    const String message = Format("Syntax error in rich content text: %s (%d:%d)", parserStatus.errorMessage.c_str(), parserStatus.errorLine, parserStatus.errorPosition);
                    onTextXMLParsingError.Emit(l->component, message);
                    Logger::Error(message.c_str());
                }

                onEndProcessComponent.Emit(l->component);
            }
        }
    }
}

void UIRichContentSystem::SetEditorMode(bool editorMode)
{
    isEditorMode = editorMode;
}

void UIRichContentSystem::AddLink(UIRichContentComponent* component)
{
    DVASSERT(component);
    component->SetModified(true);

    std::shared_ptr<RichContentLink> link = std::make_shared<RichContentLink>();
    link->component = component;
    link->control = component->GetControl();

    uint32 count = link->control->GetComponentCount<UIRichContentAliasesComponent>();
    for (uint32 i = 0; i < count; ++i)
    {
        UIRichContentAliasesComponent* c = link->control->GetComponent<UIRichContentAliasesComponent>(i);
        c->SetModified(true);
        link->AddAliases(c);
    }

    appendLinks.push_back(std::move(link));
}

void UIRichContentSystem::RemoveLink(UIRichContentComponent* component)
{
    DVASSERT(component);
    auto findIt = std::find_if(links.begin(), links.end(), [&component](std::shared_ptr<RichContentLink>& l) {
        return l->component == component;
    });
    if (findIt != links.end())
    {
        RichContentLink* link = (*findIt).get();

        link->RemoveItems();
        link->component = nullptr; // mark link for delete

        // Notify about removing components
        onRemoveComponent.Emit(component);
        for (RichContentAliasesLink& aliasLink : link->aliasesLinks)
        {
            onRemoveComponent.Emit(aliasLink.component);
        }
    }

    appendLinks.erase(std::remove_if(appendLinks.begin(), appendLinks.end(), [&component](std::shared_ptr<RichContentLink>& l) {
                          return l->component == component;
                      }),
                      appendLinks.end());
}

void UIRichContentSystem::AddAliases(UIControl* control, UIRichContentAliasesComponent* component)
{
    DVASSERT(component);
    component->SetModified(true);

    auto findIt = std::find_if(links.begin(), links.end(), [&control](std::shared_ptr<RichContentLink>& l) {
        return l->control == control;
    });
    if (findIt != links.end())
    {
        (*findIt)->AddAliases(component);
    }
}

void UIRichContentSystem::RemoveAliases(UIControl* control, UIRichContentAliasesComponent* component)
{
    DVASSERT(component);
    auto findIt = std::find_if(links.begin(), links.end(), [&control](std::shared_ptr<RichContentLink>& l) {
        return l->control == control;
    });
    if (findIt != links.end())
    {
        (*findIt)->RemoveAliases(component);

        // Notify about removing components
        onRemoveComponent.Emit(component);
    }
}

void UIRichContentSystem::HandleEvent(Observable* observable)
{
    RenderOptions* options = static_cast<RenderOptions*>(observable);
    bool newVal = options->IsOptionEnabled(RenderOptions::DEBUG_DRAW_RICH_ITEMS);
    if (newVal != isDebugDraw)
    {
        isDebugDraw = newVal;
        for (std::shared_ptr<RichContentLink>& l : links)
        {
            if (l->component)
            {
                l->component->SetModified(true);
            }
        }
    }
}
}
