#pragma once

#include "BasePropertyDelegate.h"
#include "Base/BaseTypes.h"

class StringPropertyDelegate final : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit StringPropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~StringPropertyDelegate() override = default;

    QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private slots:
    void OnEditingFinished();
};
