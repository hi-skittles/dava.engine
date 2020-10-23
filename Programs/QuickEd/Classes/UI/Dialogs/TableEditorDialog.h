#ifndef __TABLE_EDITOR_DIALOG_H__
#define __TABLE_EDITOR_DIALOG_H__

#include "ui_TableEditorDialog.h"

class QStandardItemModel;

namespace DAVA
{
class Font;
}

class TableEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TableEditorDialog(const QString& values, const QList<QString>& header, QWidget* parent = nullptr);
    ~TableEditorDialog() = default;

    const QString& GetValues() const;

private slots:
    void OnOk();
    void OnCancel();
    void OnAddRow();
    void OnRemoveRow();

private:
    Ui::TableEditorDialog ui;
    QList<QString> header;
    QString values;
    QStandardItemModel* model = nullptr;
};

#endif // __TABLE_EDITOR_DIALOG_H__
