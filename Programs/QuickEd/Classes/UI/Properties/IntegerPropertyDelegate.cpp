#include "IntegerPropertyDelegate.h"
#include <QSpinBox>
#include <QLayout>
#include "PropertiesModel.h"
#include "PropertiesTreeItemDelegate.h"
#include "Utils/QtDavaConvertion.h"

IntegerPropertyDelegate::IntegerPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

IntegerPropertyDelegate::~IntegerPropertyDelegate()
{
}

QWidget* IntegerPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QSpinBox* spinBox = new QSpinBox(parent);
    spinBox->setObjectName(QString::fromUtf8("spinBox"));
    connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged()));

    return spinBox;
}

void IntegerPropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QSpinBox* editor = rawEditor->findChild<QSpinBox*>("spinBox");

    editor->blockSignals(true);
    DAVA::Any value = index.data(Qt::EditRole).value<DAVA::Any>();
    editor->setMinimum(-99999);
    editor->setMaximum(99999);
    if (value.CanGet<DAVA::int8>() || value.CanGet<DAVA::int16>() || value.CanGet<DAVA::int32>() || value.CanGet<DAVA::int64>())
    {
        editor->setValue(value.Cast<DAVA::int32>());
    }
    else if (value.CanGet<DAVA::uint8>() || value.CanGet<DAVA::uint16>() || value.CanGet<DAVA::uint32>() || value.CanGet<DAVA::uint64>())
    {
        editor->setMinimum(0);
        editor->setValue(value.Cast<DAVA::int32>());
    }
    else
    {
        DVASSERT(false);
    }

    editor->blockSignals(false);
    BasePropertyDelegate::SetValueModified(editor, false);
}

bool IntegerPropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QSpinBox* editor = rawEditor->findChild<QSpinBox*>("spinBox");

    DAVA::Any value = index.data(Qt::EditRole).value<DAVA::Any>();

    if (value.CanGet<DAVA::int8>())
    {
        value.Set<DAVA::int8>(editor->value());
    }
    else if (value.CanGet<DAVA::uint8>())
    {
        value.Set<DAVA::uint8>(editor->value());
    }
    else if (value.CanGet<DAVA::int16>())
    {
        value.Set<DAVA::int16>(editor->value());
    }
    else if (value.CanGet<DAVA::uint16>())
    {
        value.Set<DAVA::uint16>(editor->value());
    }
    else if (value.CanGet<DAVA::int32>())
    {
        value.Set<DAVA::int32>(editor->value());
    }
    else if (value.CanGet<DAVA::uint32>())
    {
        value.Set<DAVA::uint32>(editor->value());
    }
    else if (value.CanGet<DAVA::int64>())
    {
        value.Set<DAVA::int64>(editor->value());
    }
    else if (value.CanGet<DAVA::uint64>())
    {
        value.Set<DAVA::uint64>(editor->value());
    }

    QVariant variant;
    variant.setValue<DAVA::Any>(value);

    return model->setData(index, variant, Qt::EditRole);
}

void IntegerPropertyDelegate::OnValueChanged()
{
    QWidget* spinBox = qobject_cast<QWidget*>(sender());
    if (!spinBox)
        return;

    QWidget* editor = spinBox->parentWidget();
    if (!editor)
        return;

    BasePropertyDelegate::SetValueModified(editor, true);
    itemDelegate->emitCommitData(editor);
}
