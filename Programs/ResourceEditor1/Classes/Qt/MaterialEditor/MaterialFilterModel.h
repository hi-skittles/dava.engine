#ifndef __MATERIALS_FILTER_MODEL_H__
#define __MATERIALS_FILTER_MODEL_H__

#include "Render/Material/NMaterial.h"

#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QString>

class QMimeData;
class QStandardItem;
class SceneEditor2;
class MaterialItem;
class SelectableGroup;
class MaterialModel;

class MaterialFilteringModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum eFilterType
    {
        SHOW_ALL,
        SHOW_ONLY_INSTANCES,
        SHOW_INSTANCES_AND_MATERIALS,
        SHOW_NOTHING,
    };

public:
    MaterialFilteringModel(MaterialModel* treeModel, QObject* parent = NULL);

    void Sync();

    void SetScene(SceneEditor2* scene);
    SceneEditor2* GetScene();
    void SetSelection(const SelectableGroup* group);
    DAVA::NMaterial* GetMaterial(const QModelIndex& index) const;
    QModelIndex GetIndex(DAVA::NMaterial* material, const QModelIndex& parent = QModelIndex()) const;

    bool dropCanBeAccepted(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
    void setFilterType(int type);
    int getFilterType() const;

    // QSortFilterProxyModel
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    // QStandardItemModel
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

private:
    MaterialModel* materialModel;
    eFilterType filterType;
};

#endif // __MATERIALS_FILTER_MODEL_H__
