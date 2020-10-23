#pragma once

#include "UI/DataBinding/Private/UIDataModel.h"
#include "UI/DataBinding/UIDataChildFactoryComponent.h"

namespace DAVA
{
class UIDataModel;
class UIDataBindingComponent;
class FormulaExpression;
class UIDataBindingIssueDelegate;
class UIDataBindingDependenciesManager;

class UIDataChildFactory : public UIDataModel
{
public:
    UIDataChildFactory(UIDataChildFactoryComponent* component, bool editorMode);
    ~UIDataChildFactory() override;

    UIDataChildFactoryComponent* GetComponent() const override;

    void MarkAsUnprocessed();
    bool Process(UIDataBindingDependenciesManager* dependenciesManager) override;

private:
    UIDataChildFactoryComponent* component = nullptr;
    std::shared_ptr<FormulaExpression> packageExpression;
    String packageName;

    std::shared_ptr<FormulaExpression> controlExpression;
    String controlName;

    RefPtr<UIControl> control;

    bool processed = false;
};
}
