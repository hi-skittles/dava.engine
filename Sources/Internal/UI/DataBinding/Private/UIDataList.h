#pragma once

#include "UI/DataBinding/Private/UIDataModel.h"
#include "UI/UIList.h"
#include "UI/UIListDelegate.h"

#include "Reflection/Reflection.h"

namespace DAVA
{
class UIDataBindingIssueDelegate;
class UIDataBindingDependenciesManager;
class UIDataListComponent;
class UIDataSourceComponent;
class UIDataModel;
class FormulaExpression;
class UIPackage;

class UIDataList : public UIDataModel, private UIListDelegate
{
public:
    UIDataList(UIDataListComponent* component, bool editorMode);
    ~UIDataList() override;

    UIComponent* GetComponent() const override;

    void MarkAsUnprocessed();
    bool Process(UIDataBindingDependenciesManager* dependenciesManager) override;

private:
    int32 ElementsCount(UIList* list) override;
    UIListCell* CellAtIndex(UIList* list, int32 index) override;
    float32 CellWidth(UIList* list, int32 index) override;
    float32 CellHeight(UIList* list, int32 index) override;
    void OnCellSelected(UIList* forList, UIListCell* selectedCell) override;

    void RemoveCreatedControls();

    UIDataListComponent* component = nullptr;
    UIList* list = nullptr;
    DAVA::int32 listSelectedIndex = -1;

    RefPtr<UIControl> cellPrototype;
    RefPtr<UIListCell> listCellPrototype;
    Vector<Reflection::Field> data;
    Vector<RefPtr<UIControl>> createdControls;

    std::shared_ptr<FormulaExpression> expression;

    bool processed = false;
};
}
