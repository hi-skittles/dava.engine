#pragma once

#include "DAVAEngine.h"

#include "BasePropertyDelegate.h"
class PropertiesTreeItemDelegate;

class FloatPropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit FloatPropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~FloatPropertyDelegate();

    virtual QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
private slots:
    void OnEditingFinished();
};
