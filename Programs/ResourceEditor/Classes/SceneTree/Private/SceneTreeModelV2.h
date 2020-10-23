#pragma once

#include "Classes/SceneTree/Private/SceneTreeItemTraits.h"

#include <REPlatform/DataNodes/Selectable.h>

#include <TArc/Utils/QtDelayedExecutor.h>
#include <TArc/Controls/PropertyPanel/Private/ObjectsPool.h>
#include <TArc/Qt/QtString.h>

#include <Scene3D/Scene.h>

#include <QAbstractItemModel>
#include <QMimeData>
#include <QModelIndexList>
#include <QObject>
#include <QStringList>

struct SceneTreeItemV2
{
public:
    SceneTreeItemV2() = default;

    SceneTreeItemV2* parent = nullptr;
    DAVA::int32 positionInParent = 0;

    DAVA::Vector<std::shared_ptr<SceneTreeItemV2>> children;
    DAVA::Selectable object;

    DAVA::Vector<DAVA::int32> unfetchedChildren;
};

class SceneTreeModelV2 : public QAbstractItemModel
{
public:
    SceneTreeModelV2(DAVA::Scene* scene, DAVA::ContextAccessor* accessor);
    ~SceneTreeModelV2();

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    bool hasChildren(const QModelIndex& parent) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    Qt::DropActions supportedDropActions() const override;
    Qt::DropActions supportedDragActions() const override;

    QModelIndex GetIndexByObject(const DAVA::Selectable& object) const;
    DAVA::Selectable GetObjectByIndex(const QModelIndex& index) const;

    void SyncChanges();

    DAVA::Scene* GetScene();
    SceneTreeItemV2* MapItem(const QModelIndex& index) const;
    QModelIndex MapItem(SceneTreeItemV2* item) const;

private:
    std::shared_ptr<SceneTreeItemV2> rootItem;
    DAVA::ObjectsPool<SceneTreeItemV2, DAVA::SingleThreadStrategy> objectsPool;

    DAVA::UnorderedMap<DAVA::Selectable, std::shared_ptr<SceneTreeItemV2>> mapping;
    DAVA::UnorderedSet<DAVA::Selectable> fetchedObjects;

    SceneTreeTraitsManager traits;
};