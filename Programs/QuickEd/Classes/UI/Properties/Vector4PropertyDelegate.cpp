#include "Vector4PropertyDelegate.h"
#include <QLineEdit>
#include <QLayout>
#include "PropertiesModel.h"
#include "Utils/QtDavaConvertion.h"
#include "Utils/Utils.h"
#include "PropertiesTreeItemDelegate.h"
#include <TArc/Utils/Utils.h>

using namespace DAVA;

Vector4PropertyDelegate::Vector4PropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

Vector4PropertyDelegate::~Vector4PropertyDelegate()
{
}

QWidget* Vector4PropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QLineEdit* lineEdit = new QLineEdit(parent);
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    connect(lineEdit, &QLineEdit::editingFinished, this, &Vector4PropertyDelegate::OnEditingFinished);
    return lineEdit;
}

void Vector4PropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QLineEdit* editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    DAVA::Any variant = index.data(Qt::EditRole).value<DAVA::Any>();
    QString stringValue;
    if (variant.CanGet<DAVA::Vector4>())
    {
        const Vector4& v = variant.Get<DAVA::Vector4>();
        stringValue.QString::sprintf("%g; %g; %g; %g", v.x, v.y, v.z, v.w);
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

bool Vector4PropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QLineEdit* editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    Vector4 vector = DAVA::StringToVector<Vector4>(editor->text());

    QVariant variant;
    variant.setValue<DAVA::Any>(Any(vector));

    return model->setData(index, variant, Qt::EditRole);
}

void Vector4PropertyDelegate::OnEditingFinished()
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
