#pragma once

#include "BasePropertyDelegate.h"
#include <QComboBox>
#include <QAbstractItemDelegate>
#include <QPointer>

#include "CompletionsProvider.h"
#include <memory>

class ComboPropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    ComboPropertyDelegate(PropertiesTreeItemDelegate* delegate, std::unique_ptr<CompletionsProvider> completionsProvider, bool isEditable);
    ~ComboPropertyDelegate();

    QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private slots:
    void OnCurrentIndexChanged();
    void OnActivated(const QString& text);

private:
    bool eventFilter(QObject* obj, QEvent* event) override;

    void CommitDataImpl(QWidget* comboBox);

    std::unique_ptr<CompletionsProvider> completionsProvider;
    bool isEditable = true;
};
