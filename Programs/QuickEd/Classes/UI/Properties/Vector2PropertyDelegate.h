#pragma once

#include "BasePropertyDelegate.h"

class Vector2PropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit Vector2PropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~Vector2PropertyDelegate();

    QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private slots:
    void OnEditingFinished();
};
