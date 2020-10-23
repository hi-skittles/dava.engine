#include "EnumPropertyDelegate.h"
#include <QComboBox>
#include <QLayout>
#include "Model/ControlProperties/AbstractProperty.h"
#include "PropertiesTreeItemDelegate.h"
#include "Utils/QtDavaConvertion.h"
#include "PropertiesModel.h"

EnumPropertyDelegate::EnumPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

EnumPropertyDelegate::~EnumPropertyDelegate()
{
}

QWidget* EnumPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QComboBox* comboBox = new QComboBox(parent);
    comboBox->setObjectName(QString::fromUtf8("comboBox"));
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCurrentIndexChanged()));

    AbstractProperty* property = static_cast<AbstractProperty*>(index.internalPointer());
    const EnumMap* enumMap = property->GetEnumMap();
    DVASSERT(enumMap);

    comboBox->blockSignals(true);
    for (size_t i = 0; i < enumMap->GetCount(); ++i)
    {
        int value = 0;
        if (enumMap->GetValue(i, value))
        {
            QVariant variantValue;
            variantValue.setValue<DAVA::Any>(DAVA::Any(value));
            comboBox->addItem(QString(enumMap->ToString(value)), variantValue);
        }
    }
    comboBox->blockSignals(false);

    return comboBox;
}

void EnumPropertyDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QComboBox* comboBox = editor->findChild<QComboBox*>("comboBox");

    comboBox->blockSignals(true);
    int comboIndex = comboBox->findText(index.data(Qt::DisplayRole).toString());
    comboBox->setCurrentIndex(comboIndex);
    comboBox->blockSignals(false);

    BasePropertyDelegate::SetValueModified(editor, false);
}

bool EnumPropertyDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(editor, model, index))
        return true;

    QComboBox* comboBox = editor->findChild<QComboBox*>("comboBox");
    return model->setData(index, comboBox->itemData(comboBox->currentIndex()), Qt::EditRole);
}

void EnumPropertyDelegate::OnCurrentIndexChanged()
{
    QWidget* comboBox = qobject_cast<QWidget*>(sender());
    if (!comboBox)
        return;

    QWidget* editor = comboBox->parentWidget();
    if (!editor)
        return;

    BasePropertyDelegate::SetValueModified(editor, true);
    itemDelegate->emitCommitData(editor);
}
