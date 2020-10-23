#pragma once

#include "UI/DataBinding/Private/UIDataModel.h"
#include "UI/DataBinding/UIDataSourceComponent.h"

namespace DAVA
{
class FormulaDataMap;
class FormulaContext;
class FormulaExpression;
class UIDataBindingIssueDelegate;
class UIDataBindingDependenciesManager;

class UIDataSource : public UIDataModel
{
public:
    UIDataSource(UIDataSourceComponent* component, int32 componentIndex, bool editorMode);
    ~UIDataSource() override;
    UIComponent* GetComponent() const override;

    bool Process(UIDataBindingDependenciesManager* dependenciesManager) override;

private:
    void ReadDataFromReflection(UIDataBindingDependenciesManager* dependenciesManager, UIDataErrorGuard* errorGuard);
    void ReadDataFromFile(UIDataBindingDependenciesManager* dependenciesManager, UIDataErrorGuard* errorGuard);
    void ReadDataFromExpression(UIDataBindingDependenciesManager* dependenciesManager, UIDataErrorGuard* errorGuard);
    void ReleaseDependencies(UIDataBindingDependenciesManager* dependenciesManager);

    std::shared_ptr<FormulaDataMap> LoadDataMap(const FilePath& path);

    UIDataSourceComponent* component = nullptr;
    std::shared_ptr<FormulaDataMap> sourceData;
    std::shared_ptr<FormulaExpression> expression;
};
}
