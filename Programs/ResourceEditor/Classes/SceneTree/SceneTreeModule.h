#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Controls/ContentFilter/ContentFilter.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Utils/QtDelayedExecutor.h>

#include <Base/Set.h>
#include <Reflection/Reflection.h>

#include <QPersistentModelIndex>

namespace DAVA
{
class BaseEntityCreator;
class SceneTreeFilterBase;
class EntityCreator;
} // namespace DAVA

class SceneTreeModelV2;
class QItemSelectionModel;
class QMenu;
class QModelIndex;

class SceneTreeModule : public DAVA::ClientModule
{
private:
    ~SceneTreeModule() override;
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;
    void OnContextWillBeChanged(DAVA::DataContext* current, DAVA::DataContext* newOne) override;

    void PostInit() override;

    QAbstractItemModel* GetDataModel() const;
    QItemSelectionModel* GetSelectionModel() const;
    void OnFilterChanged(const DAVA::String& newFilter);

    void OnSceneSelectionChanged(const DAVA::Any& value);
    void OnSceneTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected, QItemSelectionModel* selectionModel);
    void OnSyncRequested();

    void BuildCreateMenu(DAVA::BaseEntityCreator* baseCreator, QMenu* menu);
    void OnAddEntityClicked(DAVA::EntityCreator* creator);
    void CollapseAll();
    void ExpandAll();
    void OnInverseCollapsing();

    void OnItemDoubleClicked(const QModelIndex& index);
    void OnContextMenuRequested(const QModelIndex& index, const QPoint& globalPos);

    const DAVA::Set<QPersistentModelIndex>& GetExpandedIndexList() const;
    void SetExpandedIndexList(const DAVA::Set<QPersistentModelIndex>& expandedIndexList);

    void ReloadTexturesInSelected();

    // Filtration
    const DAVA::Vector<DAVA::SceneTreeFilterBase*>& GetFiltersChain() const;
    void AddFilterToChain(const DAVA::Any& filterTypeKey);
    void RemoveFilterFromChain(const DAVA::Any& filterIndex);

    DAVA::ContentFilter::AvailableFilterBase* GetAvailableFilterTypes() const;
    void SaveCurrentChainAsFilterType(QString filterName) const;

private:
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
    DAVA::QtDelayedExecutor executor;
    DAVA::QtConnections connections;
    bool inSelectionSync = false;

    DAVA_VIRTUAL_REFLECTION(SceneTreeModule, DAVA::ClientModule);
};
