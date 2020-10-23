#pragma once

#include <QList>
#include <QPointer>

class PropertiesContext;
class PropertiesTreeItemDelegate;
class QAction;
class QWidget;
class QModelIndex;
class QStyleOptionViewItem;
class QAbstractItemModel;

class AbstractPropertyDelegate
{
public:
    explicit AbstractPropertyDelegate(PropertiesTreeItemDelegate* delegate = NULL);
    virtual ~AbstractPropertyDelegate();

    virtual QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) = 0;
    virtual void enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions) = 0;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const = 0;
    virtual bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const = 0;

protected:
    QPointer<PropertiesTreeItemDelegate> itemDelegate;
};
