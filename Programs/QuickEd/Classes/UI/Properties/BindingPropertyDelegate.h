#pragma once

#include "BasePropertyDelegate.h"
#include "Base/BaseTypes.h"

class BindingPropertyDelegate final : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit BindingPropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~BindingPropertyDelegate() override = default;

    QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
};
