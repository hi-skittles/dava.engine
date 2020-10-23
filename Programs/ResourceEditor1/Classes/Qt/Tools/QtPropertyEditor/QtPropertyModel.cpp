#include "QtPropertyModel.h"
#include <QCoreApplication>

QtPropertyModel::QtPropertyModel(QWidget* viewport, QObject* parent /* = 0 */)
    : QAbstractItemModel(parent)
    , trackEdit(false)
    , needRefresh(false)
{
    root = new QtPropertyData(DAVA::FastName("root"));
    root->SetModel(this);
    root->SetOWViewport(viewport);
}

QtPropertyModel::~QtPropertyModel()
{
    delete root;
}

QModelIndex QtPropertyModel::index(int row, int column, const QModelIndex& parent /* = QModelIndex() */) const
{
    QModelIndex ret;

    QtPropertyData* data = itemFromIndexInternal(parent);
    if ((NULL != data) &&
        (row >= 0) &&
        (column >= 0) &&
        (row < data->ChildCount()) &&
        (column < 2))
    {
        ret = createIndex(row, column, data);
    }

    return ret;
}

QModelIndex QtPropertyModel::parent(const QModelIndex& index) const
{
    QModelIndex ret;

    if (index.isValid())
    {
        QtPropertyData* parent = static_cast<QtPropertyData*>(index.internalPointer());
        ret = indexFromItem(parent);
    }

    return ret;
}

int QtPropertyModel::rowCount(const QModelIndex& parent /* = QModelIndex() */) const
{
    int count = 0;

    QtPropertyData* data = itemFromIndexInternal(parent);
    if (NULL != data)
    {
        count = data->ChildCount();
    }

    return count;
}

int QtPropertyModel::columnCount(const QModelIndex& parent /* = QModelIndex() */) const
{
    return 2;
}

QVariant QtPropertyModel::data(const QModelIndex& index, int role /* = Qt::DisplayRole */) const
{
    QVariant ret;

    QtPropertyData* data = itemFromIndex(index);
    if (NULL != data)
    {
        if (index.column() == 0)
        {
            switch (role)
            {
            case Qt::DisplayRole:
            case Qt::ToolTipRole:
                ret = QString(data->GetName().c_str());
                break;
            case Qt::FontRole:
            case Qt::BackgroundRole:
            case Qt::ForegroundRole:
                ret = data->data(role);
                break;
            default:
                break;
            }
        }
        else if (index.column() == 1)
        {
            ret = data->data(role);
        }
    }

    return ret;
}

QVariant QtPropertyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant ret;

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0:
            ret = "Property";
            break;
        case 1:
            ret = "Value";
            break;
        case 3:
            ret = "#";
            break;
        default:
            break;
        }
    }

    return ret;
}

bool QtPropertyModel::setData(const QModelIndex& index, const QVariant& value, int role /* = Qt::EditRole */)
{
    bool ret = false;

    QtPropertyData* data = itemFromIndex(index);
    if (NULL != data && index.column() == 1)
    {
        ret = data->setData(value, role);
    }

    return ret;
}

Qt::ItemFlags QtPropertyModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags ret = 0;

    if (index.column() == 1)
    {
        QtPropertyData* data = itemFromIndex(index);
        if (NULL != data)
        {
            ret = Qt::ItemIsSelectable | data->GetFlags();
        }
    }
    else
    {
        ret = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    return ret;
}

bool QtPropertyModel::event(QEvent* e)
{
    if (e->type() == DataRefreshRequired)
    {
        emit dataChanged(QModelIndex(), QModelIndex());
        needRefresh = false;
    }

    return QAbstractItemModel::event(e);
}

QtPropertyData* QtPropertyModel::rootItem() const
{
    return root;
}

QtPropertyData* QtPropertyModel::itemFromIndex(const QModelIndex& index) const
{
    QtPropertyData* ret = NULL;

    if (index.isValid() && index.model() == this)
    {
        QtPropertyData* parent = static_cast<QtPropertyData*>(index.internalPointer());
        int row = index.row();
        if (parent != nullptr && parent->ChildCount() > row)
        {
            ret = parent->ChildGet(row);
        }
    }

    return ret;
}

QtPropertyData* QtPropertyModel::itemFromIndexInternal(const QModelIndex& index) const
{
    QtPropertyData* ret = NULL;

    if (!index.isValid())
    {
        ret = root;
    }
    else
    {
        ret = itemFromIndex(index);
    }

    return ret;
}

QModelIndex QtPropertyModel::indexFromItem(QtPropertyData* data) const
{
    QModelIndex ret;

    if (NULL != data)
    {
        QtPropertyData* parent = data->Parent();
        if (NULL != parent)
        {
            int row = parent->ChildIndex(data);
            if (row >= 0)
            {
                ret = createIndex(row, 0, parent);
            }
        }
    }

    return ret;
}

void QtPropertyModel::AppendProperties(DAVA::Vector<std::unique_ptr<QtPropertyData>>&& properties, const QModelIndex& parent /*= QModelIndex()*/)
{
    if (properties.empty())
        return;

    QtPropertyData* parentData = itemFromIndexInternal(parent);
    if (parentData != nullptr)
    {
        parentData->ChildrenAdd(std::move(properties));
    }
}

QModelIndex QtPropertyModel::AppendProperty(std::unique_ptr<QtPropertyData>&& data, const QModelIndex& parent /* = QModelIndex() */)
{
    QtPropertyData* item = data.get();
    if (NULL != data)
    {
        QtPropertyData* parentData = itemFromIndexInternal(parent);
        if (NULL != parentData)
        {
            parentData->ChildAdd(std::move(data));
        }
    }

    return indexFromItem(item);
}

void QtPropertyModel::MergeProperty(std::unique_ptr<QtPropertyData>&& data, QModelIndex const& parent)
{
    if (NULL != data)
    {
        QtPropertyData* parentData = itemFromIndexInternal(parent);
        if (NULL != parentData)
        {
            parentData->MergeChild(std::move(data));
        }
    }
}

QModelIndex QtPropertyModel::InsertProperty(std::unique_ptr<QtPropertyData>&& data, int row, const QModelIndex& parent /* = QModelIndex() */)
{
    QtPropertyData* item = data.get();
    if (NULL != data)
    {
        QtPropertyData* parentData = itemFromIndexInternal(parent);
        if (NULL != parentData)
        {
            parentData->ChildInsert(std::move(data), row);
        }
    }
    return indexFromItem(item);
}

void QtPropertyModel::RemoveProperty(const QModelIndex& index)
{
    QtPropertyData* data = itemFromIndex(index);
    if (NULL != data)
    {
        QtPropertyData* parentData = data->Parent();
        if (NULL != parentData)
        {
            parentData->ChildRemove(data);
        }
    }
}

void QtPropertyModel::RemovePropertyAll()
{
    beginResetModel();
    root->ResetChildren();
    endResetModel();
}

void QtPropertyModel::UpdateStructure(const QModelIndex& parent /* = QModelIndex */)
{
    UpdateStructureInternal(parent);
}

void QtPropertyModel::FinishTreeCreation()
{
    root->FinishTreeCreation();
}

void QtPropertyModel::UpdateStructureInternal(const QModelIndex& i)
{
    QtPropertyData* data = itemFromIndexInternal(i);
    if (NULL != data)
    {
        data->UpdateValue();

        for (int row = 0; row < rowCount(i); ++row)
        {
            UpdateStructureInternal(index(row, 0, i));
        }
    }
}

void QtPropertyModel::DataChanged(QtPropertyData* data, int reason)
{
    QModelIndex index = indexFromItem(data);
    if (index.isValid())
    {
        if (reason != QtPropertyData::VALUE_EDITED)
        {
            // Data was changed, so we should emit signal about this.
            // To be simple we will emit signal that all data was changed and it will cause
            // TreeView to refresh currently visible items. But we will not allow that refresh happen more than once per main loop.
            if (!needRefresh)
            {
                needRefresh = true;
                QCoreApplication::postEvent(this, new QEvent((QEvent::Type)DataRefreshRequired));
            }
        }
        else if (trackEdit)
        {
            emit PropertyEdited(index);
        }
    }
}

void QtPropertyModel::DataAboutToBeAdded(QtPropertyData* parent, int first, int last)
{
    QModelIndex index = indexFromItem(parent);
    beginInsertRows(index, first, last);
}

void QtPropertyModel::DataAdded()
{
    endInsertRows();
}

void QtPropertyModel::DataAboutToBeRemoved(QtPropertyData* parent, int first, int last)
{
    QModelIndex index = indexFromItem(parent);
    beginRemoveRows(index, first, last);
}

void QtPropertyModel::DataRemoved()
{
    endRemoveRows();
}

void QtPropertyModel::SetEditTracking(bool enabled)
{
    trackEdit = enabled;
}

bool QtPropertyModel::GetEditTracking()
{
    return trackEdit;
}

QtPropertyModel::InsertionGuard::InsertionGuard(QtPropertyModel* model_, QtPropertyData* parent, int first, int last)
    : model(model_)
{
    if (model != nullptr)
        model->DataAboutToBeAdded(parent, first, last);
}

QtPropertyModel::InsertionGuard::~InsertionGuard()
{
    if (model != nullptr)
        model->DataAdded();
}

QtPropertyModel::DeletionGuard::DeletionGuard(QtPropertyModel* model_, QtPropertyData* parent, int first, int last)
    : model(model_)
{
    if (model != nullptr)
        model->DataAboutToBeRemoved(parent, first, last);
}

QtPropertyModel::DeletionGuard::~DeletionGuard()
{
    if (model != nullptr)
        model->DataRemoved();
}
