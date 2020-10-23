#include "UI/Flow/UIFlowViewSystem.h"
#include "Base/TemplateHelpers.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "FileSystem/KeyedArchive.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectedTypeDB.h"
#include "UI/DataBinding/UIDataSourceComponent.h"
#include "UI/DefaultUIPackageBuilder.h"
#include "UI/Flow/UIFlowContext.h"
#include "UI/Flow/UIFlowViewComponent.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIPackageLoader.h"
#include "UI/UIScreen.h"

namespace DAVA
{
UIFlowViewSystem::~UIFlowViewSystem() = default;

void UIFlowViewSystem::RegisterControl(UIControl* control)
{
    UIFlowViewComponent* c = control->GetComponent<UIFlowViewComponent>();
    if (c)
    {
        AddViewLink(c);
    }
}

void UIFlowViewSystem::UnregisterControl(UIControl* control)
{
    UIFlowViewComponent* c = control->GetComponent<UIFlowViewComponent>();
    if (c)
    {
        RemoveViewLink(c);
    }
}

void UIFlowViewSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIFlowViewComponent>())
    {
        UIFlowViewComponent* c = DynamicTypeCheck<UIFlowViewComponent*>(component);
        AddViewLink(c);
    }
}

void UIFlowViewSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIFlowViewComponent>())
    {
        UIFlowViewComponent* c = DynamicTypeCheck<UIFlowViewComponent*>(component);
        RemoveViewLink(c);
    }
}

void UIFlowViewSystem::Process(float32 elapsedTime)
{
}

UIFlowViewComponent* UIFlowViewSystem::GetLinkedComponent(UIControl* view) const
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ViewLink& l) {
        return l.view == view;
    });
    if (it != links.end())
    {
        return it->component;
    }
    return nullptr;
}

UIControl* UIFlowViewSystem::GetLinkedView(UIFlowViewComponent* component) const
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ViewLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        return it->view.Get();
    }
    return nullptr;
}

void UIFlowViewSystem::AddViewLink(UIFlowViewComponent* component)
{
    links.push_back(ViewLink{ component, RefPtr<UIControl>() });
}

void UIFlowViewSystem::RemoveViewLink(UIFlowViewComponent* component)
{
    auto it = std::remove_if(links.begin(), links.end(), [&](const ViewLink& l) { return l.component == component; });
    links.erase(it, links.end());
}

UIControl* UIFlowViewSystem::InitView(UIFlowViewComponent* component, UIFlowContext* context)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ViewLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        FilePath yamlPath = component->GetViewYaml();
        String controlName = component->GetControlName();
        if (!yamlPath.IsEmpty())
        {
            if (yamlPath.Exists())
            {
                DAVA::DefaultUIPackageBuilder pkgBuilder;
                DAVA::UIPackageLoader().LoadPackage(yamlPath, &pkgBuilder);

                UIControl* root = nullptr;
                if (!controlName.empty())
                {
                    root = pkgBuilder.GetPackage()->GetControl(controlName);
                }
                else if (!pkgBuilder.GetPackage()->GetControls().empty())
                {
                    root = pkgBuilder.GetPackage()->GetControls().front().Get();
                }

                if (root != nullptr)
                {
                    if (!component->GetModelName().empty())
                    {
                        KeyedArchive* data = context->GetData()->GetArchive(component->GetModelName());
                        if (data)
                        {
                            UIDataSourceComponent* dataSource = root->GetOrCreateComponent<UIDataSourceComponent>();
                            dataSource->SetData(Reflection::Create(ReflectedObject(data)));
                        }
                    }

                    if (!component->GetModelScope().empty())
                    {
                        RefPtr<UIDataSourceComponent> dataScope = MakeRef<UIDataSourceComponent>();
                        dataScope->SetSourceType(UIDataSourceComponent::FROM_EXPRESSION);
                        dataScope->SetSource(component->GetModelScope());
                        root->AddComponent(dataScope.Get());
                    }

                    // Update link
                    it->view = root;
                    return root;
                }
                else
                {
                    Logger::Warning("Control `%s` from yaml with path `%s` not found!", controlName.c_str(), yamlPath.GetStringValue().c_str());
                }
            }
            else
            {
                Logger::Warning("View yaml with path `%s` not found!", yamlPath.GetStringValue().c_str());
            }
        }
    }

    return nullptr;
}

void UIFlowViewSystem::ReleaseView(UIFlowViewComponent* component)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ViewLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        it->view = nullptr;
    }
}

UIControl* UIFlowViewSystem::ActivateView(UIFlowViewComponent* component, UIFlowContext* context)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ViewLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        if (it->view != nullptr)
        {
            String containerPath = component->GetContainerPath();

            // Find near parent view
            UIControl* parentView = GetScene()->GetScreen();
            UIControl* parentEntity = component->GetControl()->GetParent();
            while (parentEntity)
            {
                UIFlowViewComponent* parentViewComponent = parentEntity->GetComponent<UIFlowViewComponent>();
                if (parentViewComponent)
                {
                    auto it = std::find_if(links.begin(), links.end(), [&](const ViewLink& l) {
                        return l.component == parentViewComponent && l.view != nullptr;
                    });
                    if (it != links.end())
                    {
                        parentView = it->view.Get();
                        break; // Stop search
                    }
                }
                parentEntity = parentEntity->GetParent();
            }

            if (parentView)
            {
                UIControl* container = containerPath.empty() ? parentView : parentView->FindByPath(containerPath);
                if (container)
                {
                    container->AddControl(it->view.Get());
                    return it->view.Get();
                }
            }
        }
    }
    return nullptr;
}

void UIFlowViewSystem::DeactivateView(UIFlowViewComponent* component)
{
    auto it = std::find_if(links.begin(), links.end(), [&](const ViewLink& l) {
        return l.component == component;
    });
    if (it != links.end())
    {
        if (it->view != nullptr)
        {
            it->view->RemoveFromParent();
        }
    }
}
}
