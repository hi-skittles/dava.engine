#ifndef __QT_PROPERTY_MODEL_FILTERINGH__
#define __QT_PROPERTY_MODEL_FILTERINGH__

#include <QSortFilterProxyModel>
#include "QtPropertyModel.h"

class QtPropertyModelFiltering : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    QtPropertyModelFiltering(QtPropertyModel* _propModel, QObject* parent = NULL);
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

    QtPropertyData* rootItem() const
    {
        return propModel->rootItem();
    }

    QtPropertyData* itemFromIndex(const QModelIndex& index) const
    {
        return propModel->itemFromIndex(mapToSource(index));
    }

    QModelIndex indexFromItem(QtPropertyData* data) const
    {
        return mapFromSource(propModel->indexFromItem(data));
    }

    QModelIndex AppendProperty(std::unique_ptr<QtPropertyData>&& data, const QModelIndex& parent = QModelIndex())
    {
        return mapFromSource(propModel->AppendProperty(std::move(data), mapToSource(parent)));
    }

    QModelIndex InsertProperty(std::unique_ptr<QtPropertyData>&& data, int row, const QModelIndex& parent = QModelIndex())
    {
        return mapFromSource(propModel->InsertProperty(std::move(data), row, mapToSource(parent)));
    }

    bool GetEditTracking()
    {
        return propModel->GetEditTracking();
    }

    void SetEditTracking(bool enabled)
    {
        propModel->SetEditTracking(enabled);
    }

    void RemoveProperty(const QModelIndex& index)
    {
        propModel->RemoveProperty(mapToSource(index));
    }

    void RemovePropertyAll()
    {
        propModel->RemovePropertyAll();
    }

    void UpdateStructure(const QModelIndex& parent = QModelIndex())
    {
        propModel->UpdateStructure(mapToSource(parent));
    }

signals:
    void PropertyEdited(const QModelIndex& index);
    void PropertyChanged(const QModelIndex& index);

protected:
    QtPropertyModel* propModel;

    bool selfAcceptRow(int sourceRow, const QModelIndex& sourceParent) const;
    bool childrenAcceptRow(int sourceRow, const QModelIndex& sourceParent) const;

protected slots:
    void OnPropertyEdited(const QModelIndex& index);
    void OnPropertyChanged(const QModelIndex& index);
    void OnRowsInserted(const QModelIndex&, int, int);
    void OnRowsRemoved(const QModelIndex&, int, int);
};

#endif // __QT_PROPERTY_MODEL_FILTERING_H__
