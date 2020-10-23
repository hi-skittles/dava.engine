#pragma once

#include "BasePropertyDelegate.h"

class QAction;
class TablePropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit TablePropertyDelegate(const QList<QString>& header, PropertiesTreeItemDelegate* delegate);
    ~TablePropertyDelegate();

    QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions) override;

private slots:
    void editTableClicked();
    void OnEditingFinished();

private:
    mutable QAction* editTableAction = nullptr;

    QList<QString> header;
};
