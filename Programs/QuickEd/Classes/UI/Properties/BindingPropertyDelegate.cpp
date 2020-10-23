
#include "UI/DataBinding/UIDataBindingComponent.h"
#include "PropertiesModel.h"
#include "Utils/QtDavaConvertion.h"
#include "PropertiesTreeItemDelegate.h"

#include "BindingPropertyDelegate.h"

#include <TArc/Utils/Utils.h>

#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>

BindingPropertyDelegate::BindingPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

QWidget* BindingPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QWidget* lineWidget = new QWidget(parent);
    QHBoxLayout* horizontalLayout = new QHBoxLayout(lineWidget);
    horizontalLayout->setSpacing(1);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setObjectName("horizontalLayout");
    lineWidget->setLayout(horizontalLayout);

    QLineEdit* lineEdit = new QLineEdit();
    lineEdit->setObjectName("lineEdit");

    connect(lineEdit, &QLineEdit::editingFinished, [this, parent, lineEdit]() {
        BasePropertyDelegate::SetValueModified(parent, lineEdit->isModified());
        itemDelegate->emitCommitData(parent);
    });

    QComboBox* comboBox = new QComboBox();
    comboBox->addItems(QStringList() << "r"
                                     << "w"
                                     << "rw"); // sync with UIDataBindingComponent::UpdateMode
    comboBox->setObjectName("comboBox");
    connect(comboBox, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), [this, parent](const QString&) {
        BasePropertyDelegate::SetValueModified(parent, true);
        itemDelegate->emitCommitData(parent);
    });

    lineWidget->layout()->addWidget(comboBox);
    lineWidget->layout()->addWidget(lineEdit);

    return lineWidget;
}

void BindingPropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QLineEdit* lineEdit = rawEditor->findChild<QLineEdit*>("lineEdit");
    QComboBox* comboBox = rawEditor->findChild<QComboBox*>("comboBox");

    QMap<QString, QVariant> map = index.data(PropertiesModel::BindingRole).toMap();
    auto modeIt = map.find("mode");
    auto valueIt = map.find("value");
    if (modeIt != map.end() && valueIt != map.end())
    {
        QString value = DAVA::UnescapeString((*valueIt).toString());

        lineEdit->blockSignals(true);
        lineEdit->setText(value);
        lineEdit->blockSignals(false);

        comboBox->blockSignals(true);
        comboBox->setCurrentIndex((*modeIt).toInt());
        comboBox->blockSignals(false);
    }
}

bool BindingPropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QLineEdit* lineEditor = rawEditor->findChild<QLineEdit*>("lineEdit");
    QComboBox* comboEditor = rawEditor->findChild<QComboBox*>("comboBox");
    DAVA::int32 bindingMode = 0;
    if (comboEditor != nullptr)
    {
        bindingMode = comboEditor->currentIndex();
    }

    QString text = "";
    if (lineEditor != nullptr)
    {
        text = lineEditor->text();
    }

    QMap<QString, QVariant> map;
    map.insert("value", DAVA::EscapeString(text));
    map.insert("mode", bindingMode);
    BasePropertyDelegate::SetValueModified(rawEditor, false);
    return model->setData(index, QVariant(map), PropertiesModel::BindingRole);
}
