#pragma once

#include "UI/DataBinding/Private/UIDataNode.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIDataBindingComponent;
class FormulaExpression;
class UIDataBindingIssueDelegate;
class UIDataBindingDependenciesManager;

class UIDataBinding : public UIDataNode
{
public:
    UIDataBinding(UIDataBindingComponent* component, bool editorMode);
    ~UIDataBinding();

    UIComponent* GetComponent() const override;

    void ProcessReadFromModel(UIDataBindingDependenciesManager* dependenciesManager);
    bool ProcessWriteToModel(UIDataBindingDependenciesManager* dependenciesManager);

private:
    UIDataBindingComponent* component = nullptr;
    std::shared_ptr<FormulaExpression> expression;

    Reflection controlReflection;
};
}
