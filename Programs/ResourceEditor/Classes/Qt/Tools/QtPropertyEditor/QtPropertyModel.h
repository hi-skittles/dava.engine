#ifndef __QT_PROPERTY_MODEL_H__
#define __QT_PROPERTY_MODEL_H__

#include "Base/Introspection.h"
#include "QtPropertyData.h"

#include <TArc/WindowSubSystem/QtTArcEvents.h>

#include <QAbstractItemModel>
#include <QEvent>

class QtPropertyModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    QtPropertyModel(QWidget* _viewport, QObject* parent = 0);
    ~QtPropertyModel();

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex& index) const;
    bool event(QEvent* e);

    QtPropertyData* rootItem() const;

    QtPropertyData* itemFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromItem(QtPropertyData* data) const;

    void AppendProperties(DAVA::Vector<std::unique_ptr<QtPropertyData>>&& properties, const QModelIndex& parent = QModelIndex());
    QModelIndex AppendProperty(std::unique_ptr<QtPropertyData>&& data, const QModelIndex& parent = QModelIndex());
    void MergeProperty(std::unique_ptr<QtPropertyData>&& data, const QModelIndex& parent = QModelIndex());
    QModelIndex InsertProperty(std::unique_ptr<QtPropertyData>&& data, int row, const QModelIndex& parent = QModelIndex());

    bool GetEditTracking();
    void SetEditTracking(bool enabled);

    void RemoveProperty(const QModelIndex& index);
    void RemovePropertyAll();

    void UpdateStructure(const QModelIndex& parent = QModelIndex());
    void FinishTreeCreation();

signals:
    void PropertyEdited(const QModelIndex& index);

protected:
    enum
    {
        DataRefreshRequired = static_cast<DAVA::int32>(DAVA::EventsTable::End)
    };

    friend class QtPropertyData;

    QtPropertyData* root;
    bool trackEdit;
    bool needRefresh;

    class InsertionGuard
    {
    public:
        InsertionGuard(QtPropertyModel* model_, QtPropertyData* parent, int first, int last);
        ~InsertionGuard();

    private:
        QtPropertyModel* model;
    };

    class DeletionGuard
    {
    public:
        DeletionGuard(QtPropertyModel* model_, QtPropertyData* parent, int first, int last);
        ~DeletionGuard();

    private:
        QtPropertyModel* model;
    };

    QtPropertyData* itemFromIndexInternal(const QModelIndex& index) const;

    void DataChanged(QtPropertyData* data, int reason);
    void DataAboutToBeAdded(QtPropertyData* parent, int first, int last);
    void DataAdded();
    void DataAboutToBeRemoved(QtPropertyData* parent, int first, int last);
    void DataRemoved();

    void UpdateStructureInternal(const QModelIndex& index);
};

#endif // __QT_PROPERTY_MODEL_H__
