#include "BoolPropertyDelegate.h"
#include <QComboBox>
#include "DAVAEngine.h"
#include "PropertiesTreeItemDelegate.h"
#include "Utils/QtDavaConvertion.h"

BoolPropertyDelegate::BoolPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

BoolPropertyDelegate::~BoolPropertyDelegate()
{
}

QWidget* BoolPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QComboBox* comboBox = new QComboBox(parent);
    comboBox->addItem(QVariant(false).toString(), false);
    comboBox->addItem(QVariant(true).toString(), true);
    comboBox->setObjectName(QString::fromUtf8("comboBox"));
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCurrentIndexChanged()));

    return comboBox;
}

void BoolPropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QComboBox* editor = rawEditor->findChild<QComboBox*>("comboBox");

    editor->blockSignals(true);
    DAVA::Any variant = index.data(Qt::EditRole).value<DAVA::Any>();
    DVASSERT(variant.CanGet<bool>());
    int comboIndex = editor->findData(QVariant(variant.Get<bool>()));
    editor->setCurrentIndex(comboIndex);
    editor->blockSignals(false);

    BasePropertyDelegate::SetValueModified(editor, false);
}

bool BoolPropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QComboBox* comboBox = rawEditor->findChild<QComboBox*>("comboBox");

    DAVA::Any any(comboBox->itemData(comboBox->currentIndex()).toBool());
    QVariant variant;
    variant.setValue<DAVA::Any>(any);

    return model->setData(index, variant, Qt::EditRole);
}

void BoolPropertyDelegate::OnCurrentIndexChanged()
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
