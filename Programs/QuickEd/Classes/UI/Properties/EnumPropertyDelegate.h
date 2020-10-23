#pragma once

#include "BasePropertyDelegate.h"
#include <QComboBox>
#include <QAbstractItemDelegate>
#include <QPointer>

class PropertiesTreeItemDelegate;

class EnumPropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    EnumPropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~EnumPropertyDelegate();

    QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private slots:
    void OnCurrentIndexChanged();
};
