#pragma once

#include "AbstractPropertyDelegate.h"

#include <TArc/Core/ContextAccessor.h>

#include <QObject>
#include <QWidget>
#include <QModelIndex>
#include <QStyleOptionViewItem>

class PropertiesTreeItemDelegate;

class BasePropertyDelegate : public QObject,
                             public AbstractPropertyDelegate
{
    Q_OBJECT
public:
    explicit BasePropertyDelegate(PropertiesTreeItemDelegate* delegate = NULL);
    virtual ~BasePropertyDelegate();
    virtual void enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions) override;
    virtual bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

    static bool IsValueModified(QWidget* editor)
    {
        QVariant data = editor->property("valueModified");
        return data.toBool();
    }

    static void SetValueModified(QWidget* editor, bool value)
    {
        editor->setProperty("valueModified", QVariant(value));
    }

    static bool IsValueReseted(QWidget* editor)
    {
        QVariant data = editor->property("valueReseted");
        return data.toBool();
    }

    static void SetValueReseted(QWidget* editor, bool value)
    {
        editor->setProperty("valueReseted", QVariant(value));
    }

    static QString GetBindingExpression(QWidget* editor)
    {
        QVariant data = editor->property("bindingExpression");
        return data.toString();
    }

    static bool HasBindingExpression(QWidget* editor)
    {
        QVariant data = editor->property("bindingExpression");
        return data.type() == QVariant::String;
    }

    static void SetBindingExpression(QWidget* editor, QString expr)
    {
        editor->setProperty("bindingExpression", QVariant(expr));
    }

    static void ResetBindingExpression(QWidget* editor)
    {
        editor->setProperty("bindingExpression", QVariant());
    }

private slots:
    void resetClicked();
    void bindClicked();

protected:
    DAVA::ContextAccessor* accessor = nullptr;
};
