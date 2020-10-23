#pragma once

#include "UI/DataBinding/Private/UIDataNode.h"

#include "UI/DataBinding/UIDataSourceComponent.h"
#include "UI/DataBinding/UIDataListComponent.h"
#include "UI/DataBinding/UIDataChildFactoryComponent.h"

namespace DAVA
{
class FormulaDataMap;
class FormulaContext;
class FormulaExpression;
class UIDataBindingIssueDelegate;
class UIDataBindingDependenciesManager;

class UIDataModel : public UIDataNode
{
public:
    virtual ~UIDataModel();

    static std::unique_ptr<UIDataModel> Create(UIDataSourceComponent* component, int32 componentIndex, bool editorMode);
    static std::unique_ptr<UIDataModel> Create(UIDataListComponent* component, int32 componentIndex, bool editorMode);
    static std::unique_ptr<UIDataModel> Create(UIDataChildFactoryComponent* component, int32 componentIndex, bool editorMode);

    int32 GetOrder() const;

    bool IsDirty() const;
    void SetDirty();
    void ResetDirty();

    const std::shared_ptr<FormulaContext>& GetFormulaContext() const;
    virtual const std::shared_ptr<FormulaContext>& GetRootContext() const;

    void MarkAsUnprocessed();
    virtual bool Process(UIDataBindingDependenciesManager* dependenciesManager);

protected:
    enum Priority
    {
        PRIORITY_ROOT = 0,
        PRIORITY_SOURCE = 1,
        PRIORITY_FACTORY = 1001,
        PRIORITY_LIST = 1002,
        PRIORITY_DATA_BINDING = 1003,
        PRIORITY_COUNT = 1004
    };

    bool processed = false;
    UIDataModel(UIComponent* component, int32 priority, bool editorMode);

protected:
    int32 order = 0;
    bool dirty = false;
    std::shared_ptr<FormulaContext> context;
};
}
