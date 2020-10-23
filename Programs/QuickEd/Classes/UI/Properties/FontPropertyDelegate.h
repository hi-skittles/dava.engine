#pragma once

#include "BasePropertyDelegate.h"
class QAction;
class Project;
class FontPropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit FontPropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~FontPropertyDelegate();

    virtual QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    virtual void enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions) override;

private slots:
    void addPresetClicked();
    void configurePresetClicked();
    void valueChanged();

private:
    mutable QAction* configurePresetAction;
    const Project* project = nullptr;
};
