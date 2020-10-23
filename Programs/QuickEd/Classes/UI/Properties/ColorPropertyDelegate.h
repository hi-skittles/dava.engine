#pragma once

#include "BasePropertyDelegate.h"

class QToolButton;

class ColorPropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit ColorPropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~ColorPropertyDelegate();

    virtual QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    virtual void enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions) override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private slots:
    void OnChooseColorClicked();
    void OnEditingFinished();
    void OnTextChanged(const QString& text);

private:
    QPointer<QLineEdit> lineEdit = nullptr;
    QPointer<QAction> chooseColorAction = nullptr;
};
