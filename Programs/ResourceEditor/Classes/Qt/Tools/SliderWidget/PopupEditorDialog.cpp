#include "PopupEditorDialog.h"

#include <QLineEdit>
#include <QLayout>
#include <QIntValidator>

PopupEditorDialog::PopupEditorDialog(int initialValue,
                                     int rangeMin, int rangeMax,
                                     const QWidget* widget /* = 0 */,
                                     QWidget* parent /* = 0 */)
    : QDialog(parent, Qt::Popup)
    , widget(widget)
    , value(initialValue)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setMaximumWidth(50);

    editValue = new QLineEdit(this);
    editValue->setText(QString::number(initialValue));

    QIntValidator* validator = new QIntValidator(rangeMin, rangeMax, editValue);
    editValue->setValidator(validator);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(editValue);

    setLayout(layout);

    connect(editValue, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    connect(editValue, SIGNAL(editingFinished()), this, SLOT(OnEditingFinished()));
}

PopupEditorDialog::~PopupEditorDialog()
{
    emit ValueReady(widget, value);
}

void PopupEditorDialog::showEvent(QShowEvent* event)
{
    editValue->setFocus();
    editValue->selectAll();
    QDialog::showEvent(event);
}

void PopupEditorDialog::OnReturnPressed()
{
    value = editValue->text().toInt();
}

void PopupEditorDialog::OnEditingFinished()
{
    close();
}
