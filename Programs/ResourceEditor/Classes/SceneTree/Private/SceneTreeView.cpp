#include "Classes/SceneTree/Private/SceneTreeView.h"
#include "Classes/SceneTree/Private/SceneTreeItemDelegateV2.h"

#include <REPlatform/DataNodes/Settings/GlobalSceneSettings.h>
#include <REPlatform/DataNodes/SceneData.h>

#include <TArc/Utils/ScopedValueGuard.h>
#include <TArc/Core/ContextAccessor.h>

#include <Debug/DVAssert.h>
#include <Logger/Logger.h>

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>

void SceneTreeView::EraseEmptyIndexes(DAVA::Set<QPersistentModelIndex>& indexes)
{
    auto iter = indexes.begin();
    while (iter != indexes.end())
    {
        if (iter->isValid() == false)
        {
            iter = indexes.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

SceneTreeView::SceneTreeView(const Params& params, DAVA::ContextAccessor* accessor, DAVA::Reflection model, QWidget* parent)
    : ControlProxyImpl<QTreeView>(params, DAVA::ControlDescriptor(params.fields), accessor, model, parent)
    , defaultSelectionModel(new QItemSelectionModel(nullptr, this))
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setExpandsOnDoubleClick(false);
    setUniformRowHeights(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDragEnabled(true);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);

    setItemDelegate(new SceneTreeItemDelegateV2(this));

    connections.AddConnection(this, &QTreeView::expanded, DAVA::MakeFunction(this, &SceneTreeView::OnItemExpanded));
    connections.AddConnection(this, &QTreeView::collapsed, DAVA::MakeFunction(this, &SceneTreeView::OnItemCollapsed));
    connections.AddConnection(this, &QTreeView::doubleClicked, DAVA::MakeFunction(this, &SceneTreeView::OnDoubleClicked));
}

void SceneTreeView::AddAction(QAction* action)
{
    addAction(action);
}

void SceneTreeView::UpdateControl(const DAVA::ControlDescriptor& descriptor)
{
    if (descriptor.IsChanged(Fields::DataModel) == true)
    {
        setModel(GetFieldValue<QAbstractItemModel*>(Fields::DataModel, nullptr));
    }

    if (descriptor.IsChanged(Fields::ExpandedIndexList) == true)
    {
        DAVA::ScopedValueGuard<bool> guard(inExpandingSync, true);
        expandedIndexList = GetFieldValue(Fields::ExpandedIndexList, DAVA::Set<QPersistentModelIndex>());
        if (expandedIndexList.size() == 1 && (*expandedIndexList.begin() == QModelIndex()))
        {
            expandedIndexList.clear();
            collapseAll();
            QMetaObject::Connection conID = QObject::connect(this, &QTreeView::expanded, [this](const QModelIndex& index) {
                expandedIndexList.insert(index);
            });
            expandAll();
            QObject::disconnect(conID);
            wrapper.SetFieldValue(GetFieldName(Fields::ExpandedIndexList), expandedIndexList);
        }
        else
        {
            collapseAll();
            for (const QPersistentModelIndex& index : expandedIndexList)
            {
                if (index.isValid() == true)
                {
                    DVASSERT(static_cast<QAbstractItemView*>(this)->model() == index.model());
                    expand(index);
                }
            }
        }
    }

    if (descriptor.IsChanged(Fields::SelectionModel) == true)
    {
        QItemSelectionModel* newSelectionModel = GetFieldValue<QItemSelectionModel*>(Fields::SelectionModel, nullptr);
        if (newSelectionModel != nullptr)
        {
            QItemSelectionModel* prevSelectoinModel = selectionModel();
            if (prevSelectoinModel != nullptr)
            {
                connections.RemoveConnection(prevSelectoinModel, &QItemSelectionModel::selectionChanged);
            }
            setSelectionModel(newSelectionModel);
            connections.AddConnection(newSelectionModel, &QItemSelectionModel::selectionChanged, DAVA::MakeFunction(this, &SceneTreeView::OnSelectionChanged));
        }
    }

    UpdateVisibleItem(selectionModel()->selection());
}

void SceneTreeView::OnItemExpanded(const QModelIndex& index)
{
    SCOPED_VALUE_GUARD(bool, inExpandingSync, true, void());
    expandedIndexList.insert(QPersistentModelIndex(index));
    EraseEmptyIndexes(expandedIndexList);
    wrapper.SetFieldValue(GetFieldName(Fields::ExpandedIndexList), expandedIndexList);
}

void SceneTreeView::OnItemCollapsed(const QModelIndex& index)
{
    SCOPED_VALUE_GUARD(bool, inExpandingSync, true, void());
    expandedIndexList.erase(QPersistentModelIndex(index));
    EraseEmptyIndexes(expandedIndexList);
    wrapper.SetFieldValue(GetFieldName(Fields::ExpandedIndexList), expandedIndexList);
}

void SceneTreeView::OnDoubleClicked(const QModelIndex& index)
{
    DAVA::FastName doubleClickedName = GetFieldName(Fields::DoubleClicked);
    if (doubleClickedName.IsValid() == true)
    {
        DAVA::AnyFn fn = model.GetMethod(doubleClickedName.c_str());
        DVASSERT(fn.IsValid() == true);
        fn.Invoke(index);
    }
}

void SceneTreeView::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    UpdateVisibleItem(selected);
}

void SceneTreeView::UpdateVisibleItem(const QItemSelection& selected)
{
    if (selected.isEmpty() == false)
    {
        QRegion region = visualRegionForSelection(selected);
        if (region.isEmpty())
        {
            scrollTo(selected.indexes().front(), QAbstractItemView::PositionAtCenter);
        }
    }
}

void SceneTreeView::contextMenuEvent(QContextMenuEvent* e)
{
    DAVA::FastName contextMenuName = GetFieldName(Fields::ContextMenuRequested);
    if (contextMenuName.IsValid() == true)
    {
        DAVA::AnyFn fn = model.GetMethod(contextMenuName.c_str());
        DVASSERT(fn.IsValid() == true);

        QModelIndex itemIndex = indexAt(e->pos());
        if (itemIndex.isValid())
        {
            fn.Invoke(itemIndex, e->globalPos());
        }
    }
}

void SceneTreeView::dragEnterEvent(QDragEnterEvent* e)
{
    QTreeView::dragEnterEvent(e);

    if (e->source() == this && e->isAccepted() == false)
    {
        e->setDropAction(Qt::IgnoreAction);
        e->accept();
    }
}

void SceneTreeView::dragLeaveEvent(QDragLeaveEvent* e)
{
    QTreeView::dragLeaveEvent(e);
}

void SceneTreeView::dragMoveEvent(QDragMoveEvent* e)
{
    DAVA::GlobalSceneSettings* settings = controlParams.accessor->GetGlobalContext()->GetData<DAVA::GlobalSceneSettings>();
    if (settings->dragAndDropWithShift == true && ((e->keyboardModifiers() & Qt::SHIFT) != Qt::SHIFT))
    {
        e->setDropAction(Qt::IgnoreAction);
        e->accept();
        return;
    }

    QTreeView::dragMoveEvent(e);
}

void SceneTreeView::dropEvent(QDropEvent* e)
{
    QTreeView::dropEvent(e);

    if (e->isAccepted())
    {
        QPersistentModelIndex index = indexAt(e->pos());

        switch (dropIndicatorPosition())
        {
        case QAbstractItemView::AboveItem:
        case QAbstractItemView::BelowItem:
            index = index.parent();
            break;
        case QAbstractItemView::OnItem:
        case QAbstractItemView::OnViewport:
            break;
        }

        DAVA::FastName dropExecuted = GetFieldName(Fields::DropExecuted);
        if (dropExecuted.IsValid() == true)
        {
            DAVA::AnyFn fn = model.GetMethod(dropExecuted.c_str());
            DVASSERT(fn.IsValid() == true);
            fn.Invoke();
        }

        executor.DelayedExecute([this, index]() {
            expand(index);
        });
    }

    e->setDropAction(Qt::IgnoreAction);
    e->accept();
}
