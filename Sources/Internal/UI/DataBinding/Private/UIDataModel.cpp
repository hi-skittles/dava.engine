#include "UI/DataBinding/Private/UIDataModel.h"

#include "UI/DataBinding/Private/UIDataSource.h"
#include "UI/DataBinding/Private/UIDataList.h"
#include "UI/DataBinding/Private/UIDataChildFactory.h"

#include "UI/Formula/FormulaContext.h"
#include "UI/Formula/Private/FormulaException.h"
#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaExecutor.h"
#include "UI/UIControl.h"

#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
UIDataModel::UIDataModel(UIComponent* component, int32 priority, bool editorMode)
    : UIDataNode(editorMode)
{
    int32 depth = 0;
    if (component != nullptr)
    {
        UIControl* c = component->GetControl();
        while (c)
        {
            c = c->GetParent();
            depth++;
        }
    }

    order = depth * PRIORITY_COUNT + priority;
}

UIDataModel::~UIDataModel()
{
}

std::unique_ptr<UIDataModel> UIDataModel::Create(UIDataSourceComponent* component, int32 componentIndex, bool editorMode)
{
    DVASSERT(0 <= componentIndex && componentIndex < PRIORITY_FACTORY - PRIORITY_SOURCE); // componentIndex should be limited to avoit priority conflicts
    return std::make_unique<UIDataSource>(component, componentIndex, editorMode);
}

std::unique_ptr<UIDataModel> UIDataModel::Create(UIDataListComponent* component, int32 componentIndex, bool editorMode)
{
    DVASSERT(componentIndex == 0); // UIDataListComponent hasn't be multiple
    return std::make_unique<UIDataList>(component, editorMode);
}

std::unique_ptr<UIDataModel> UIDataModel::Create(UIDataChildFactoryComponent* component, int32 componentIndex, bool editorMode)
{
    DVASSERT(componentIndex == 0); // UIDataChildFactoryComponent hasn't be multiple
    return std::make_unique<UIDataChildFactory>(component, editorMode);
}

int32 UIDataModel::GetOrder() const
{
    return order;
}

bool UIDataModel::IsDirty() const
{
    return dirty;
}

void UIDataModel::SetDirty()
{
    dirty = true;
}

void UIDataModel::ResetDirty()
{
    dirty = false;
}

const std::shared_ptr<FormulaContext>& UIDataModel::GetFormulaContext() const
{
    return context;
}

const std::shared_ptr<FormulaContext>& UIDataModel::GetRootContext() const
{
    DVASSERT(GetParent() != nullptr);
    return GetParent()->GetRootContext();
}

void UIDataModel::MarkAsUnprocessed()
{
    processed = false;
}

bool UIDataModel::Process(UIDataBindingDependenciesManager* dependenciesManager)
{
    if (processed)
    {
        return false;
    }

    processed = true;
    return true;
}
}
