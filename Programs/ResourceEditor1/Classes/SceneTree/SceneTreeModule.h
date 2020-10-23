#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Controls/ContentFilter/ContentFilter.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Utils/QtDelayedExecutor.h>

#include <Base/Set.h>
#include <Reflection/Reflection.h>

#include <QPersistentModelIndex>

class SceneTreeModelV2;
class SceneTreeFilterBase;
class BaseEntityCreator;
class EntityCreator;

class QItemSelectionModel;
class QMenu;
class QModelIndex;

class SceneTreeModule : public DAVA::TArc::ClientModule
{
private:
    ~SceneTreeModule() override;
    void OnContextCreated(DAVA::TArc::DataContext* context) override;
    void OnContextDeleted(DAVA::TArc::DataContext* context) override;
    void OnContextWillBeChanged(DAVA::TArc::DataContext* current, DAVA::TArc::DataContext* newOne) override;

    void PostInit() override;

    QAbstractItemModel* GetDataModel() const;
    QItemSelectionModel* GetSelectionModel() const;
    void OnFilterChanged(const DAVA::String& newFilter);

    void OnSceneSelectionChanged(const DAVA::Any& value);
    void OnSceneTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected, QItemSelectionModel* selectionModel);
    void OnSyncRequested();

    void BuildCreateMenu(BaseEntityCreator* baseCreator, QMenu* menu);
    void OnAddEntityClicked(EntityCreator* creator);
    void CollapseAll();
    void ExpandAll();
    void OnInverseCollapsing();

    void OnItemDoubleClicked(const QModelIndex& index);
    void OnContextMenuRequested(const QModelIndex& index, const QPoint& globalPos);

    const DAVA::Set<QPersistentModelIndex>& GetExpandedIndexList() const;
    void SetExpandedIndexList(const DAVA::Set<QPersistentModelIndex>& expandedIndexList);

    void ReloadTexturesInSelected();

    // Filtration
    const DAVA::Vector<SceneTreeFilterBase*>& GetFiltersChain() const;
    void AddFilterToChain(const DAVA::Any& filterTypeKey);
    void RemoveFilterFromChain(const DAVA::Any& filterIndex);

    DAVA::ContentFilter::AvailableFilterBase* GetAvailableFilterTypes() const;
    void SaveCurrentChainAsFilterType(QString filterName) const;

private:
    std::unique_ptr<DAVA::TArc::FieldBinder> fieldBinder;
    DAVA::TArc::QtDelayedExecutor executor;
    DAVA::TArc::QtConnections connections;
    bool inSelectionSync = false;

    DAVA_VIRTUAL_REFLECTION(SceneTreeModule, DAVA::TArc::ClientModule);
};
