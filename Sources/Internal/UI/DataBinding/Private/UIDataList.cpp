#include "UI/DataBinding/Private/UIDataList.h"

#include "UI/DataBinding/UIDataBindingIssueDelegate.h"
#include "UI/DataBinding/UIDataSourceComponent.h"
#include "UI/DataBinding/UIDataListComponent.h"

#include "UI/DataBinding/Private/UIDataBindingDependenciesManager.h"
#include "UI/DataBinding/Private/UIDataModel.h"

#include "UI/Formula/Private/FormulaExpression.h"
#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaExecutor.h"
#include "UI/Formula/FormulaContext.h"

#include "UI/UIPackage.h"
#include "UI/UIPackageLoader.h"
#include "UI/UIListCell.h"
#include "UI/DefaultUIPackageBuilder.h"
#include "UI/Layouts/UILayoutSourceRectComponent.h"
#include "UI/Components/UIControlSourceComponent.h"

#include "Render/2D/FTFont.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
UIDataList::UIDataList(UIDataListComponent* component_, bool editorMode)
    : UIDataModel(component_, PRIORITY_LIST, editorMode)
    , component(component_)
{
    list = dynamic_cast<UIList*>(component->GetControl());

    if (list)
    {
        list->SetDelegate(this);
    }
}

UIDataList::~UIDataList()
{
    if (list)
    {
        if (list->GetDelegate() == this)
        {
            list->SetDelegate(nullptr);
            list->ImmediateClearCells();
        }
        else
        {
            DVASSERT(false);
        }
    }
    RemoveCreatedControls();
}

UIComponent* UIDataList::GetComponent() const
{
    return component;
}

void UIDataList::MarkAsUnprocessed()
{
    processed = false;
}

bool UIDataList::Process(UIDataBindingDependenciesManager* dependenciesManager)
{
    if (!UIDataModel::Process(dependenciesManager))
    {
        return false;
    }

    bool processed = false;

    bool hasToResetError = true;
    bool expChanged = false;

    context = GetParent()->GetFormulaContext();

    if (component->IsDirty())
    {
        component->SetDirty(false);
        cellPrototype = nullptr;
        listCellPrototype = nullptr;
        expression = nullptr;
        hasToResetError = true;
        expChanged = true;
        processed = true;

        if (!component->GetCellPackage().IsEmpty() && !component->GetCellControlName().empty())
        {
            bool recursiveControl = false;
            {
                UIControl* p = component->GetControl();
                while (p != nullptr)
                {
                    UIControlSourceComponent* objComp = p->GetComponent<UIControlSourceComponent>();
                    if (objComp && objComp->GetPackagePath() == component->GetCellPackage().GetStringValue() && objComp->GetControlName() == component->GetCellControlName())
                    {
                        recursiveControl = true;
                        break;
                    }
                    p = p->GetParent();
                }
            }

            if (!recursiveControl)
            {
                DefaultUIPackageBuilder pkgBuilder;
                pkgBuilder.SetEditorMode(editorMode);
                UIPackageLoader().LoadPackage(component->GetCellPackage(), &pkgBuilder);

                if (pkgBuilder.GetPackage() != nullptr)
                {
                    cellPrototype = pkgBuilder.GetPackage()->GetControl(component->GetCellControlName());
                    if (cellPrototype.Valid())
                    {
                        UIControlSourceComponent* objComp = cellPrototype->GetOrCreateComponent<UIControlSourceComponent>();
                        objComp->SetPackagePath(component->GetCellPackage().GetStringValue());
                        objComp->SetControlName(component->GetCellControlName());
                        if (list != nullptr)
                        {
                            listCellPrototype = dynamic_cast<UIListCell*>(cellPrototype.Get());
                            if (!listCellPrototype.Valid())
                            {
                                cellPrototype = nullptr;
                                hasToResetError = false;
                                NotifyError(Format("Control '%s' from package '%s' must be UIListCell",
                                                   component->GetCellControlName().c_str(),
                                                   component->GetCellPackage().GetStringValue().c_str()),
                                            "UIDataListComponent/dataContainer");
                            }
                        }
                    }
                    else
                    {
                        hasToResetError = false;
                        NotifyError(Format("Can't find control '%s' in package '%s'",
                                           component->GetCellControlName().c_str(),
                                           component->GetCellPackage().GetStringValue().c_str()),
                                    "UIDataListComponent/dataContainer");
                    }
                }
                else
                {
                    hasToResetError = false;
                    NotifyError(Format("Can't load package '%s'",
                                       component->GetCellPackage().GetStringValue().c_str()),
                                "UIDataListComponent/dataContainer");
                }
            }
            else
            {
                hasToResetError = false;
                NotifyError(Format("Trying to load control '%s' from package '%s' recursively",
                                   component->GetCellControlName().c_str(),
                                   component->GetCellPackage().GetStringValue().c_str()),
                            "UIDataListComponent/dataContainer");
            }
        }

        if (cellPrototype)
        {
            try
            {
                FormulaParser parser(component->GetDataContainer());
                expression = parser.ParseExpression();
            }
            catch (const FormulaException& error)
            {
                expression.reset();
                hasToResetError = false;
                NotifyError(error.GetFormattedMessage(), "UIDataListComponent/dataContainer");
            }
        }
    }

    std::shared_ptr<FormulaContext> parentContext = parent->GetFormulaContext();

    if (expChanged || parent->IsDirty() || dependenciesManager->IsDirty(dependencyId))
    {
        processed = true;
        listSelectedIndex = -1;
        data.clear();
        if (expression)
        {
            try
            {
                FormulaExecutor executor(parentContext);
                Reflection dataRef = executor.GetDataReference(expression.get());

                const Vector<void*>& dependencies = executor.GetDependencies();

                if (!dependencies.empty())
                {
                    dependencyId = dependenciesManager->MakeDependency(dependencyId, dependencies);
                }
                else if (dependencyId != UIDataBindingDependenciesManager::UNKNOWN_DEPENDENCY)
                {
                    dependenciesManager->ReleaseDepencency(dependencyId);
                }

                data = dataRef.GetFields();
            }
            catch (const FormulaException& error)
            {
                hasToResetError = false;
                NotifyError(error.GetFormattedMessage(), "UIDataListComponent/dataContainer");
            }
        }

        if (list)
        {
            list->ImmediateClearCells();
        }
        else
        {
            RemoveCreatedControls();
            int32 index = 0;
            for (Reflection::Field& field : data)
            {
                if (cellPrototype.Valid())
                {
                    RefPtr<UIControl> cell = cellPrototype->SafeClone();
                    UIDataSourceComponent* cellDataSourceComponent = cell->GetOrCreateComponent<UIDataSourceComponent>();
                    cellDataSourceComponent->SetData(field.ref);

                    if (editorMode)
                    {
                        UILayoutSourceRectComponent* sourceRect = cell->GetOrCreateComponent<UILayoutSourceRectComponent>();
                        sourceRect->SetSize(cell->GetSize());
                        sourceRect->SetPosition(cell->GetPosition());
                    }

                    component->GetControl()->AddControl(cell.Get());
                    createdControls.push_back(cell);
                }
                index++;
            }
        }
    }

    if (list && component->GetSelectedIndex() != listSelectedIndex)
    {
        listSelectedIndex = component->GetSelectedIndex();
        list->Refresh();
    }

    if (hasToResetError && processed)
    {
        ResetError();
    }

    return processed;
}

int32 UIDataList::ElementsCount(UIList* list)
{
    if (!listCellPrototype.Valid())
    {
        return 0;
    }
    return static_cast<int32>(data.size());
}

UIListCell* UIDataList::CellAtIndex(UIList* list, int32 index)
{
    DVASSERT(!data.empty());
    DVASSERT(listCellPrototype.Valid());

    if (listCellPrototype.Valid())
    {
        UIListCell* cell = list->GetReusableCell("cell");
        if (!cell)
        {
            cell = listCellPrototype->Clone();
            cell->SetIdentifier("cell");
        }

        UIDataSourceComponent* cellDataSourceComponent = cell->GetOrCreateComponent<UIDataSourceComponent>();
        cellDataSourceComponent->SetData(data[index].ref);

        cell->SetSelected(component->IsSelectionSupported() && index == component->GetSelectedIndex());

        return cell;
    }
    else
    {
        DVASSERT(false);
        return nullptr;
    }
}

float32 UIDataList::CellWidth(UIList* list, int32 index)
{
    if (listCellPrototype.Valid())
    {
        return listCellPrototype->GetSize().dx;
    }
    return 0.0f;
}

float32 UIDataList::CellHeight(UIList* list, int32 index)
{
    if (listCellPrototype.Valid())
    {
        return listCellPrototype->GetSize().dy;
    }
    return 0.0f;
}

void UIDataList::OnCellSelected(UIList* forList, UIListCell* selectedCell)
{
    if (component->IsSelectionSupported())
    {
        component->SetSelectedIndex(selectedCell->GetIndex());
        listSelectedIndex = selectedCell->GetIndex();
        const auto& ls = forList->GetVisibleCells();
        for (auto it = ls.begin(); it != ls.end(); ++it)
        {
            UIListCell* c = DynamicTypeCheck<UIListCell*>(it->Get());
            c->SetSelected(c->GetIndex() == component->GetSelectedIndex());
        }
    }
}

void UIDataList::RemoveCreatedControls()
{
    for (RefPtr<UIControl>& control : createdControls)
    {
        control->RemoveFromParent();
    }
    createdControls.clear();
}
}
