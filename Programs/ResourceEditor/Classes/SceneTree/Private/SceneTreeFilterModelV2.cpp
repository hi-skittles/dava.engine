#include "Classes/SceneTree/Private/SceneTreeFilterModelV2.h"
#include "Classes/SceneTree/Private/SceneTreeRoles.h"

#include <REPlatform/Global/SceneTree/SceneTreeFiltration.h>
#include <REPlatform/DataNodes/Selectable.h>

#include <TArc/DataProcessing/AnyQMetaType.h>
#include <TArc/Utils/ReflectionHelpers.h>
#include <TArc/Utils/ScopedValueGuard.h>
#include <TArc/DataProcessing/AnyQMetaType.h>

#include <Base/BaseTypes.h>
#include <Base/Type.h>
#include <Debug/DVAssert.h>
#include <Reflection/ReflectedType.h>
#include <Reflection/ReflectedTypeDB.h>

#include <QBrush>
#include <QColor>

SceneTreeFilterModelV2::SceneTreeFilterModelV2(DAVA::ContextAccessor* accessor_)
    : accessor(accessor_)
{
}

const QString& SceneTreeFilterModelV2::GetFilter() const
{
    return filter;
}

void SceneTreeFilterModelV2::SetFilter(const QString& filter_)
{
    if (filter == filter_)
    {
        return;
    }

    filter = filter_;
    Refilter();
}

QVariant SceneTreeFilterModelV2::data(const QModelIndex& index, int role) const
{
    if (index.isValid() == false && role == Qt::BackgroundRole && (filter.isEmpty() == false || filtersChain.empty() == false))
    {
        QHash<QModelIndex, FilterData>::const_iterator iter = filtrationData.constFind(mapToSource(index));
        DVASSERT(iter != filtrationData.cend());
        if (iter.value().isMatched == true)
        {
            return QBrush(QColor(0, 255, 0, 20));
        }
    }

    return QSortFilterProxyModel::data(index, role);
}

bool SceneTreeFilterModelV2::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (inForceFetchState == true || (filter.isEmpty() == true && filtersChain.empty() == true))
    {
        return true;
    }

    QModelIndex itemIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    QHash<QModelIndex, FilterData>::const_iterator iter = filtrationData.constFind(itemIndex);
    if (iter == filtrationData.cend())
    {
        PrepareFiltrationData(sourceModel(), itemIndex);
        iter = filtrationData.constFind(itemIndex);
    }
    DVASSERT(iter != filtrationData.constEnd());
    return iter.value().isVisible;
}

bool SceneTreeFilterModelV2::PrepareFiltrationData(QAbstractItemModel* model, const QModelIndex& index) const
{
    bool isSomeChildIsMatched = false;
    int rowCount = model->rowCount(index);
    for (int i = 0; (i < rowCount) && (isSomeChildIsMatched == false); ++i)
    {
        isSomeChildIsMatched |= PrepareFiltrationData(model, model->index(i, 0, index));
    }

    FilterData data;
    QString displayName = index.data(Qt::DisplayRole).toString();
    QString filterItemData = index.data(ToItemRoleCast(eSceneTreeRoles::FilterDataRole)).toString();
    data.isMatched = displayName.contains(filter, Qt::CaseInsensitive) || filter == filterItemData;

    if (data.isMatched == true && index.isValid())
    {
        for (DAVA::SceneTreeFilterBase* filterBase : filtersChain)
        {
            if (filterBase->IsEnabled())
            {
                DAVA::Selectable internalObject = DAVA::Selectable(index.data(ToItemRoleCast(eSceneTreeRoles::InternalObjectRole)).value<DAVA::Any>());
                bool isMatched = filterBase->IsMatched(internalObject, accessor);
                if (filterBase->IsInverted())
                {
                    isMatched = !isMatched;
                }

                data.isMatched &= isMatched;
            }

            if (data.isMatched == false)
            {
                break;
            }
        }
    }

    data.isVisible = isSomeChildIsMatched || data.isMatched;

    filtrationData.insert(index, data);
    return data.isVisible;
}

void SceneTreeFilterModelV2::FullFetchModel(QAbstractItemModel* model, const QModelIndex& index)
{
    if (model->canFetchMore(index) == true)
    {
        model->fetchMore(index);
    }

    int rowCount = model->rowCount(index);
    for (int i = 0; i < rowCount; ++i)
    {
        FullFetchModel(model, model->index(i, 0, index));
    }
}

const DAVA::Vector<DAVA::SceneTreeFilterBase*>& SceneTreeFilterModelV2::GetFiltersChain() const
{
    return filtersChain;
}

void SceneTreeFilterModelV2::AddFilterToChain(const DAVA::ReflectedType* filterType, const FilterState& state)
{
    for (DAVA::SceneTreeFilterBase* filterBase : filtersChain)
    {
        const DAVA::ReflectedType* filterBaseType = DAVA::ReflectedTypeDB::GetByPointer(filterBase);
        if (filterBaseType == filterType)
        {
            return;
        }
    }

    DAVA::SceneTreeFilterBase* createdFilter = filterType->CreateObject(DAVA::ReflectedType::CreatePolicy::ByPointer).Cast<DAVA::SceneTreeFilterBase*>();
    createdFilter->SetEnabled(state.enabled);
    createdFilter->SetInverted(state.inverted);
    createdFilter->changed.Connect(this, &SceneTreeFilterModelV2::Refilter);
    filtersChain.push_back(createdFilter);

    Refilter();
}

void SceneTreeFilterModelV2::DeleteFilter(DAVA::int32 filterIndex)
{
    DVASSERT(filterIndex < static_cast<DAVA::int32>(filtersChain.size()));
    DAVA::SceneTreeFilterBase* filterToDestroy = filtersChain[filterIndex];
    const DAVA::ReflectedType* filterType = DAVA::ReflectedTypeDB::GetByPointer(filterToDestroy);
    if (filterType->GetDtor() != nullptr)
    {
        DAVA::Any valueToDestroy(filterToDestroy);
        filterType->Destroy(std::move(valueToDestroy));
    }
    else
    {
        delete filterToDestroy;
    }
    filtersChain.erase(filtersChain.begin() + filterIndex);
    Refilter();
}

void SceneTreeFilterModelV2::Refilter()
{
    bool infoWasEmpty = filtrationData.empty();
    filtrationData.clear();
    if (filter.isEmpty() == false || filtersChain.empty() == false)
    {
        {
            DAVA::ScopedValueGuard<bool> guard(inForceFetchState, true);
            FullFetchModel(sourceModel(), QModelIndex());
        }
        PrepareFiltrationData(sourceModel(), QModelIndex());
    }

    if (filtrationData.empty() == false || infoWasEmpty == false)
    {
        invalidate();
    }
}
