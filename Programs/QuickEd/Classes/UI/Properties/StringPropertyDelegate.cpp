#include "StringPropertyDelegate.h"
#include <QLineEdit>
#include <QLayout>
#include "DAVAEngine.h"
#include "PropertiesModel.h"
#include "Utils/QtDavaConvertion.h"
#include "PropertiesTreeItemDelegate.h"

#include <TArc/Utils/Utils.h>

StringPropertyDelegate::StringPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

QWidget* StringPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QLineEdit* lineEdit = new QLineEdit(parent);
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(OnEditingFinished()));

    return lineEdit;
}

void StringPropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QLineEdit* editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    DAVA::Any value = index.data(Qt::EditRole).value<DAVA::Any>();
    QString stringValue;
    if (value.CanGet<DAVA::String>())
    {
        stringValue = StringToQString(value.Get<DAVA::String>());
    }
    else if (value.CanGet<DAVA::FastName>())
    {
        const DAVA::FastName& fn = value.Get<DAVA::FastName>();
        stringValue = fn.empty() ? "" : StringToQString(fn.c_str());
    }
    else
    {
        DVASSERT(false);
        if (value.CanGet<DAVA::WideString>())
        {
            stringValue = WideStringToQString(value.Get<DAVA::WideString>());
        }
    }
    DAVA::UnescapeString(stringValue);

    editor->blockSignals(true);
    editor->setText(stringValue);
    editor->blockSignals(false);
}

bool StringPropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QLineEdit* editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    DAVA::Any value = index.data(Qt::EditRole).value<DAVA::Any>();

    QString stringValue = DAVA::EscapeString(editor->text());
    if (value.CanGet<DAVA::String>())
    {
        value.Set<DAVA::String>(QStringToString(stringValue));
    }
    else if (value.CanGet<DAVA::FastName>())
    {
        value.Set<DAVA::FastName>(DAVA::FastName(QStringToString(stringValue)));
    }
    else
    {
        DVASSERT(false);
        value.Set<DAVA::WideString>(QStringToWideString(stringValue));
    }

    QVariant variant;
    variant.setValue<DAVA::Any>(value);

    return model->setData(index, variant, Qt::EditRole);
}

void StringPropertyDelegate::OnEditingFinished()
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
