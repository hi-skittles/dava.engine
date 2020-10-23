#include "FloatPropertyDelegate.h"
#include "PropertiesModel.h"
#include "PropertiesTreeItemDelegate.h"
#include "Utils/QtDavaConvertion.h"
#include <QLineEdit>
#include <QValidator>
#include <QLayout>

FloatPropertyDelegate::FloatPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

FloatPropertyDelegate::~FloatPropertyDelegate()
{
}

QWidget* FloatPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QLineEdit* lineEdit = new QLineEdit(parent);
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(OnEditingFinished()));
    lineEdit->setValidator(new QRegExpValidator(QRegExp("\\s*-?\\d*[,\\.]?\\d*\\s*")));

    return lineEdit;
}

void FloatPropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QLineEdit* editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    DAVA::Any variant = index.data(Qt::EditRole).value<DAVA::Any>();
    editor->setText(QString("%1").arg(variant.Get<DAVA::float32>()));

    BasePropertyDelegate::SetValueModified(editor, false);
}

bool FloatPropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QLineEdit* editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    QVariant variant;
    variant.setValue<DAVA::Any>(editor->text().toFloat());

    return model->setData(index, variant, Qt::EditRole);
}

void FloatPropertyDelegate::OnEditingFinished()
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
