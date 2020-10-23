#include "UI/Dialogs/TableEditorDialog.h"
#include "FileSystem/LocalizationSystem.h"

#include <QStandardItemModel>

using namespace DAVA;

TableEditorDialog::TableEditorDialog(const QString& values_, const QList<QString>& header_, QWidget* parent)
    : QDialog(parent)
    , header(header_)
    , values(values_)
{
    ui.setupUi(this);

    ui.addButton->setIcon(QIcon(":/Icons/add.png"));
    ui.removeButton->setIcon(QIcon(":/Icons/editclear.png"));

    connect(ui.addButton, &QPushButton::clicked, this, &TableEditorDialog::OnAddRow);
    connect(ui.removeButton, &QPushButton::clicked, this, &TableEditorDialog::OnRemoveRow);
    connect(ui.pushButton_ok, &QPushButton::clicked, this, &TableEditorDialog::OnOk);
    connect(ui.pushButton_cancel, &QPushButton::clicked, this, &TableEditorDialog::OnCancel);

    model = new QStandardItemModel(0, header.size(), this);
    model->setHorizontalHeaderLabels(header);

    QStringList rows = values.split(";");
    for (QString& rowStr : rows)
    {
        QStringList row = rowStr.split(",");
        QList<QStandardItem*> items;

        for (int i = 0; i < header.size(); i++)
        {
            QString str = "";
            if (row.size() > i)
            {
                str = row[i].trimmed();
            }

            QStandardItem* item = new QStandardItem();
            item->setData(str, Qt::DisplayRole);
            items.push_back(item);
        }
        model->appendRow(items);
    }

    ui.tableView->setModel(model);
    ui.tableView->resizeColumnsToContents();
}

const QString& TableEditorDialog::GetValues() const
{
    return values;
}

void TableEditorDialog::OnOk()
{
    values.clear();
    bool firstRow = true;
    for (int row = 0; row < model->rowCount(); row++)
    {
        bool empty = true;
        for (int col = 0; col < header.size(); col++)
        {
            QString str = model->data(model->index(row, col), Qt::DisplayRole).toString();
            if (!str.trimmed().isEmpty())
            {
                empty = false;
                break;
            }
        }

        if (!empty)
        {
            if (firstRow)
            {
                firstRow = false;
            }
            else
            {
                values.append(";");
            }

            for (int col = 0; col < header.size(); col++)
            {
                if (col > 0)
                {
                    values.append(",");
                }
                values.append(model->data(model->index(row, col), Qt::DisplayRole).toString());
            }
        }
    }
    accept();
}

void TableEditorDialog::OnCancel()
{
    reject();
}

void TableEditorDialog::OnAddRow()
{
    QList<QStandardItem*> items;
    for (int i = 0; i < header.size(); i++)
    {
        QStandardItem* item = new QStandardItem();
        item->setData("");
        items.push_back(item);
    }
    model->appendRow(items);
}

void TableEditorDialog::OnRemoveRow()
{
    QModelIndex current = ui.tableView->selectionModel()->currentIndex();
    if (current.isValid())
    {
        model->removeRow(current.row());
    }
}
