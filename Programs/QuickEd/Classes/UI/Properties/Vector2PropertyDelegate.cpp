#include "Vector2PropertyDelegate.h"

#include <QLineEdit>
#include <QLayout>
#include "PropertiesModel.h"
#include "Utils/QtDavaConvertion.h"
#include "Utils/Utils.h"
#include "PropertiesTreeItemDelegate.h"
#include <TArc/Utils/Utils.h>

using namespace DAVA;

Vector2PropertyDelegate::Vector2PropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

Vector2PropertyDelegate::~Vector2PropertyDelegate()
{
}

QWidget* Vector2PropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QLineEdit* lineEdit = new QLineEdit(parent);
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    connect(lineEdit, &QLineEdit::editingFinished, this, &Vector2PropertyDelegate::OnEditingFinished);
    return lineEdit;
}

void Vector2PropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QLineEdit* editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    DAVA::Any variant = index.data(Qt::EditRole).value<DAVA::Any>();
    QString stringValue;
    if (variant.CanGet<DAVA::Vector2>())
    {
        const Vector2& v = variant.Get<Vector2>();
        stringValue.QString::sprintf("%g; %g", v.x, v.y);
    }
    else if (variant.CanGet<DAVA::String>())
    {
        stringValue = QString::fromStdString(variant.Get<DAVA::String>());
    }
    else
    {
        stringValue = "?";
        DVASSERT(false);
    }
    editor->blockSignals(true);
    editor->setText(stringValue);
    editor->blockSignals(false);
}

bool Vector2PropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QLineEdit* editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    Vector2 vector = DAVA::StringToVector<Vector2>(editor->text());

    QVariant variant;
    variant.setValue<DAVA::Any>(vector);

    return model->setData(index, variant, Qt::EditRole);
}

void Vector2PropertyDelegate::OnEditingFinished()
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
    if (!lineEdit)
        return;

    QWidget* editor = lineEdit->parentWidget();
    if (!editor)
        return;

    BasePropertyDelegate::SetValueModified(editor, lineEdit->isModified());
    itemDelegate->emitCommitData(editor);
}
