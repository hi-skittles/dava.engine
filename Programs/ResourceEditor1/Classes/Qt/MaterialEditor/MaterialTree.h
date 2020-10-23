#pragma once

#include <QWidget>
#include <QTreeView>
#include <QMap>

#include "MaterialModel.h"

namespace DAVA
{
namespace TArc
{
class FieldBinder;
}
}

class RECommandNotificationObject;
class SelectableGroup;
class MaterialFilteringModel;

class MaterialTree : public QTreeView
{
    Q_OBJECT

public:
    MaterialTree(QWidget* parent = 0);
    ~MaterialTree();

    void SetScene(SceneEditor2* sceneEditor);
    DAVA::NMaterial* GetMaterial(const QModelIndex& index) const;

    void SelectMaterial(DAVA::NMaterial* material);
    void SelectEntities(const QList<DAVA::NMaterial*>& materials);

    void Update();

    int getFilterType() const;
    void setFilterType(int filterType);

signals:
    void Updated();
    void ContextMenuPrepare(QMenu*);

public slots:
    void ShowContextMenu(const QPoint& pos);
    void OnCommandExecuted(SceneEditor2* scene, const RECommandNotificationObject& commandNotification);
    void OnStructureChanged(SceneEditor2* scene, DAVA::Entity* parent);
    void OnSelectEntities();

protected:
    MaterialFilteringModel* treeModel;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void dragTryAccepted(QDragMoveEvent* event);
    void GetDropParams(const QPoint& pos, QModelIndex& index, int& row, int& col);

private:
    void OnSelectionChanged(const DAVA::Any& selection);
    std::unique_ptr<DAVA::TArc::FieldBinder> selectionFieldBinder;
};
