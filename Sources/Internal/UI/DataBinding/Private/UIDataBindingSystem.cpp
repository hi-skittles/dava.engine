#include "UI/DataBinding/UIDataBindingSystem.h"

#include "UI/UIControl.h"

#include "UI/DataBinding/UIDataSourceComponent.h"
#include "UI/DataBinding/UIDataBindingComponent.h"
#include "UI/DataBinding/UIDataListComponent.h"
#include "UI/DataBinding/UIDataChildFactoryComponent.h"

#include "UI/DataBinding/Private/UIDataModel.h"
#include "UI/DataBinding/Private/UIDataRoot.h"
#include "UI/DataBinding/Private/UIDataBinding.h"
#include "UI/DataBinding/Private/UIDataList.h"
#include "UI/DataBinding/Private/UIDataChildFactory.h"
#include "UI/DataBinding/Private/UIDataBindingDependenciesManager.h"

#include "UI/Formula/FormulaContext.h"

#include "Logger/Logger.h"

namespace DAVA
{
UIDataBindingSystem::UIDataBindingSystem()
{
    rootModel = std::make_shared<UIDataRoot>(false);
    dependenciesManager = std::make_unique<UIDataBindingDependenciesManager>();
}

UIDataBindingSystem::~UIDataBindingSystem()
{
    dataModels.clear();
    dataBindings.clear();
}

std::shared_ptr<FormulaContext> UIDataBindingSystem::GetFormulaContext(UIControl* control) const
{
    UIControl* c = control;
    while (c)
    {
        for (auto it = dataModels.rbegin(); it != dataModels.rend(); ++it)
        {
            UIDataModel* model = it->get();
            if (model->GetComponent() && c == model->GetComponent()->GetControl())
            {
                return model->GetFormulaContext();
            }
        }
        c = c->GetParent();
    }
    return std::shared_ptr<FormulaContext>();
}

void UIDataBindingSystem::SetDataDirty(void* dataPtr)
{
    dependenciesManager->SetDirty(dataPtr);
}

void UIDataBindingSystem::RegisterControl(UIControl* control)
{
    TryToCreateDataModel<UIDataSourceComponent>(control);
    TryToCreateDataModel<UIDataListComponent>(control);
    TryToCreateDataModel<UIDataChildFactoryComponent>(control);

    int32 count = control->GetComponentCount<UIDataBindingComponent>();
    for (int32 i = 0; i < count; i++)
    {
        RegisterDataBinding(control->GetComponent<UIDataBindingComponent>(i));
    }
}

void UIDataBindingSystem::UnregisterControl(UIControl* control)
{
    int32 count = control->GetComponentCount<UIDataSourceComponent>();
    for (int32 i = 0; i < count; i++)
    {
        DeleteDataModel(control->GetComponent<UIDataSourceComponent>(i));
    }

    DeleteDataModel(control->GetComponent<UIDataListComponent>());
    DeleteDataModel(control->GetComponent<UIDataChildFactoryComponent>());

    count = control->GetComponentCount<UIDataBindingComponent>();
    for (int32 i = 0; i < count; i++)
    {
        UnregisterDataBinding(control->GetComponent<UIDataBindingComponent>(i));
    }
}

void UIDataBindingSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    TryToCreateDataModel<UIDataSourceComponent>(control, component);
    TryToCreateDataModel<UIDataListComponent>(control, component);
    TryToCreateDataModel<UIDataChildFactoryComponent>(control, component);

    if (component->GetType() == Type::Instance<UIDataBindingComponent>())
    {
        RegisterDataBinding(DynamicTypeCheck<UIDataBindingComponent*>(component));
    }
}

void UIDataBindingSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIDataSourceComponent>() ||
        component->GetType() == Type::Instance<UIDataListComponent>() ||
        component->GetType() == Type::Instance<UIDataChildFactoryComponent>())
    {
        DeleteDataModel(component);
    }
    else if (component->GetType() == Type::Instance<UIDataBindingComponent>())
    {
        UnregisterDataBinding(DynamicTypeCheck<UIDataBindingComponent*>(component));
    }
}

void UIDataBindingSystem::Process(float32 elapsedTime)
{
    for (const std::shared_ptr<UIDataModel>& model : dataModels)
    {
        model->MarkAsUnprocessed();
    }

    hasUnprocessedModels = true;

    Vector<std::shared_ptr<UIDataModel>> processedModels;
    while (hasUnprocessedModels)
    {
        hasUnprocessedModels = false;
        Vector<std::shared_ptr<UIDataModel>> copy = dataModels;
        for (const std::shared_ptr<UIDataModel>& model : copy)
        {
            if (std::find(dataModels.begin(), dataModels.end(), model) != dataModels.end())
            {
                if (model->Process(dependenciesManager.get()))
                {
                    processedModels.push_back(model);
                }
                DVASSERT(model->GetFormulaContext() != nullptr);
            }
        }
    }

    for (const std::shared_ptr<UIDataBinding>& binding : dataBindings)
    {
        binding->ProcessReadFromModel(dependenciesManager.get());
    }

    for (const std::shared_ptr<UIDataModel>& model : processedModels)
    {
        if (std::find(dataModels.begin(), dataModels.end(), model) != dataModels.end())
        {
            onDataModelProcessed.Emit(model->GetComponent()->GetControl(), model->GetComponent());
        }
    }
}

void UIDataBindingSystem::FinishProcess()
{
    dependenciesManager->ResetDirties();

    for (const std::shared_ptr<UIDataBinding>& binding : dataBindings)
    {
        if (binding->ProcessWriteToModel(dependenciesManager.get()))
        {
            onValueWrittenToModel.Emit(binding->GetComponent()->GetControl(), binding->GetComponent());
        }
    }

    rootModel->ResetDirty();
    for (const std::shared_ptr<UIDataModel>& model : dataModels)
    {
        model->ResetDirty();
    }
}

void UIDataBindingSystem::SetIssueDelegate(UIDataBindingIssueDelegate* delegate)
{
    issueDelegate = delegate;

    for (const std::shared_ptr<UIDataModel>& dataModel : dataModels)
    {
        dataModel->SetIssueDelegate(delegate);
    }

    for (const std::shared_ptr<UIDataBinding>& dataBinding : dataBindings)
    {
        dataBinding->SetIssueDelegate(delegate);
    }
}

void UIDataBindingSystem::SetEditorMode(bool editorMode_)
{
    editorMode = editorMode_;
}

void UIDataBindingSystem::RegisterDataBinding(UIDataBindingComponent* component)
{
    component->SetDirty(true);

    std::shared_ptr<UIDataBinding> node = std::make_shared<UIDataBinding>(component, editorMode);
    node->SetIssueDelegate(issueDelegate);
    node->SetParent(FindParentModel(component->GetControl()));
    dataBindings.push_back(node);
}

void UIDataBindingSystem::UnregisterDataBinding(UIDataBindingComponent* component)
{
    auto it = std::find_if(dataBindings.begin(), dataBindings.end(), [component](const std::shared_ptr<UIDataBinding>& l) {
        return l->GetComponent() == component;
    });

    if (it != dataBindings.end())
    {
        std::shared_ptr<UIDataBinding> node = *it;
        dependenciesManager->ReleaseDepencency(node->GetDepencencyId());
        dataBindings.erase(it);
    }
}

void UIDataBindingSystem::UpdateDependentModelsAndBindings(const UIDataModel* model)
{
    for (const std::shared_ptr<UIDataModel>& m : dataModels)
    {
        if (m->GetParent() == model)
        {
            m->SetParent(FindParentModel(m->GetComponent()));
        }
    }

    for (const std::shared_ptr<UIDataBinding>& binding : dataBindings)
    {
        if (binding->GetParent() == model)
        {
            binding->SetParent(FindParentModel(binding->GetComponent()->GetControl()));
        }
    }
}

template <typename ComponentType>
void UIDataBindingSystem::TryToCreateDataModel(UIControl* control)
{
    int32 count = control->GetComponentCount<ComponentType>();
    for (int32 i = 0; i < count; i++)
    {
        TryToCreateDataModel<ComponentType>(control, control->GetComponent<ComponentType>(i));
    }
}

template <typename ComponentType>
void UIDataBindingSystem::TryToCreateDataModel(UIControl* control, UIComponent* testComponent)
{
    if (testComponent && testComponent->GetType() == Type::Instance<ComponentType>())
    {
        ComponentType* component = static_cast<ComponentType*>(testComponent);
        component->SetDirty(true);

        bool modelAlreadyCreated = false;
        for (const std::shared_ptr<UIDataModel>& model : dataModels)
        {
            if (model->GetComponent() == component)
            {
                modelAlreadyCreated = true;
                break;
            }
        }

        if (!modelAlreadyCreated)
        {
            dataModels.push_back(UIDataModel::Create(component, control->GetComponentIndex(component), editorMode));
            UIDataModel* model = dataModels.back().get();
            model->SetIssueDelegate(issueDelegate);

            std::stable_sort(dataModels.begin(), dataModels.end(), [](const std::shared_ptr<UIDataModel>& m1, const std::shared_ptr<UIDataModel>& m2) {
                return m1->GetOrder() < m2->GetOrder();
            });

            model->SetParent(FindParentModel(component));
            UpdateDependentModelsAndBindings(model->GetParent());
            hasUnprocessedModels = true;
        }
    }
}

void UIDataBindingSystem::DeleteDataModel(UIComponent* component)
{
    if (component == nullptr)
    {
        return;
    }
    auto it = std::find_if(dataModels.begin(), dataModels.end(), [component](const std::shared_ptr<UIDataModel>& m1) {
        return m1->GetComponent() == component;
    });

    if (it != dataModels.end())
    {
        std::shared_ptr<UIDataModel> model = *it;
        dataModels.erase(it);

        model->GetParent()->SetDirty();

        UpdateDependentModelsAndBindings(model.get());
        dependenciesManager->ReleaseDepencency(model->GetDepencencyId());
    }
}

UIDataModel* UIDataBindingSystem::FindParentModel(UIComponent* component) const
{
    auto modelIt = std::find_if(dataModels.rbegin(), dataModels.rend(), [component](const std::shared_ptr<UIDataModel>& model) {
        return model->GetComponent() == component;
    });

    if (modelIt == dataModels.rend())
    {
        DVASSERT(false);
        return rootModel.get();
    }
    ++modelIt;

    if (modelIt == dataModels.rend())
    {
        return rootModel.get();
    }

    const UIControl* c = component->GetControl();
    while (c)
    {
        for (auto it = modelIt; it != dataModels.rend(); ++it)
        {
            if (c == (*it)->GetComponent()->GetControl())
            {
                return it->get();
            }
        }
        c = c->GetParent();
    }
    return rootModel.get();
}

UIDataModel* UIDataBindingSystem::FindParentModel(UIControl* control) const
{
    const UIControl* c = control;
    while (c)
    {
        for (auto it = dataModels.rbegin(); it != dataModels.rend(); ++it)
        {
            if (c == (*it)->GetComponent()->GetControl())
            {
                return it->get();
            }
        }
        c = c->GetParent();
    }
    return rootModel.get();
}
}
