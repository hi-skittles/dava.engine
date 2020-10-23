#pragma once

#include <TArc/Controls/ControlProxy.h>
#include <TArc/Controls/ControlDescriptor.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Utils/QtDelayedExecutor.h>

#include <QTreeView>

namespace DAVA
{
class ContextAccessor;
} // namespace DAVA

class QItemSelection;
class QItemSelectionModel;
class QDropEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDragEnterEvent;

class SceneTreeView : public DAVA::ControlProxyImpl<QTreeView>
{
public:
    enum class Fields
    {
        DataModel,
        SelectionModel,
        ExpandedIndexList,
        DoubleClicked, // method(const QModelIndex& index)
        ContextMenuRequested, // method(const QModelIndex& index, const QPoint& globalPos)
        DropExecuted,
        FieldCount
    };

    DECLARE_CONTROL_PARAMS(Fields);

    SceneTreeView(const Params& params, DAVA::ContextAccessor* accessor, DAVA::Reflection model, QWidget* parent = nullptr);

    void AddAction(QAction* action);
    static void EraseEmptyIndexes(DAVA::Set<QPersistentModelIndex>& indexes);

protected:
    void UpdateControl(const DAVA::ControlDescriptor& descriptor) override;

    void OnItemExpanded(const QModelIndex& index);
    void OnItemCollapsed(const QModelIndex& index);
    void OnDoubleClicked(const QModelIndex& index);
    void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    void UpdateVisibleItem(const QItemSelection& selected);

    void contextMenuEvent(QContextMenuEvent* e) override;

    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private:
    DAVA::QtConnections connections;
    DAVA::QtDelayedExecutor executor;
    DAVA::Set<QPersistentModelIndex> expandedIndexList;
    QItemSelectionModel* defaultSelectionModel = nullptr;
    bool inExpandingSync = false;
};
