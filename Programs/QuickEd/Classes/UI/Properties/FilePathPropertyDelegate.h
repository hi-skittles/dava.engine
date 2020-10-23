#pragma once

#include "BasePropertyDelegate.h"

class FilePathPropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit FilePathPropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~FilePathPropertyDelegate();

    virtual QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
private slots:
    void OnEditingFinished();
    void OnTextChanged(const QString& text);

private:
    bool IsPathValid(const QString& path) const;
    QPointer<QLineEdit> lineEdit = nullptr;
};
