#pragma once

#include "Base/BaseTypes.h"
#include "Functional/Signal.h"
#include "UI/UISystem.h"
#include "UI/DataBinding/UIDataBindingIssueDelegate.h"

namespace DAVA
{
class UIControl;
class UIDataSourceComponent;
class UIDataScopeComponent;
class UIDataListComponent;
class UIDataBindingComponent;
class UIDataBindingDependenciesManager;

class FormulaContext;

class UIDataNode;
class UIDataModel;
class UIDataBinding;
class UIDataList;
class UIDataChildFactory;

class UIDataBindingSystem : public UISystem
{
public:
    UIDataBindingSystem();
    virtual ~UIDataBindingSystem();

    std::shared_ptr<FormulaContext> GetFormulaContext(UIControl* control) const;
    void SetDataDirty(void* dataPtr);

    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(float32 elapsedTime) override;

    void FinishProcess();

    void SetIssueDelegate(UIDataBindingIssueDelegate* issueDelegate);
    void SetEditorMode(bool editorMode);

    Signal<UIControl*, UIComponent*> onDataModelProcessed;
    Signal<UIControl*, UIComponent*> onValueWrittenToModel;

private:
    void RegisterDataBinding(UIDataBindingComponent* component);
    void UnregisterDataBinding(UIDataBindingComponent* component);

    void UpdateDependentModelsAndBindings(const UIDataModel* model);

    template <typename ComponentType>
    void TryToCreateDataModel(UIControl* control);

    template <typename ComponentType>
    void TryToCreateDataModel(UIControl* control, UIComponent* component);

    void DeleteDataModel(UIComponent* component);

    UIDataModel* FindParentModel(UIComponent* control) const;
    UIDataModel* FindParentModel(UIControl* control) const;

    Vector<std::shared_ptr<UIDataModel>> dataModels;
    Vector<std::shared_ptr<UIDataBinding>> dataBindings;
    bool hasUnprocessedModels = false;

    std::unique_ptr<UIDataBindingDependenciesManager> dependenciesManager;

    UIDataBindingIssueDelegate* issueDelegate = nullptr;
    std::shared_ptr<UIDataModel> rootModel;

    bool editorMode = false;
};
}
