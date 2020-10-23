#pragma once

#include <TArc/Controls/ContentFilter/ContentFilter.h>

#include <QHash>
#include <QSortFilterProxyModel>
#include <QVariant>

namespace DAVA
{
class ContextAccessor;
class SceneTreeFilterBase;
} // namespace DAVA

struct FilterState
{
    bool enabled = true;
    bool inverted = false;

    bool operator==(const FilterState& other) const
    {
        return enabled == other.enabled &&
        inverted == other.inverted;
    }
};

class SceneTreeFilterModelV2 final : public QSortFilterProxyModel
{
public:
    SceneTreeFilterModelV2(DAVA::ContextAccessor* accessor);

    const QString& GetFilter() const;
    void SetFilter(const QString& filter);
    void Refilter();

    const DAVA::Vector<DAVA::SceneTreeFilterBase*>& GetFiltersChain() const;
    void AddFilterToChain(const DAVA::ReflectedType* filterType, const FilterState& state);
    void DeleteFilter(DAVA::int32 filterIndex);

    QVariant data(const QModelIndex& index, int role) const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    bool PrepareFiltrationData(QAbstractItemModel* model, const QModelIndex& index) const;
    void FullFetchModel(QAbstractItemModel* model, const QModelIndex& index);

private:
    QString filter;

    struct FilterData
    {
        bool isVisible = false;
        bool isMatched = false;
    };

    bool inForceFetchState = false;
    mutable QHash<QModelIndex, FilterData> filtrationData;
    DAVA::Vector<DAVA::SceneTreeFilterBase*> filtersChain;
    DAVA::ContextAccessor* accessor = nullptr;
};