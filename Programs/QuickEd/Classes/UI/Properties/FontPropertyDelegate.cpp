#include "FontPropertyDelegate.h"
#include <DAVAEngine.h>
#include "PropertiesTreeItemDelegate.h"
#include "Utils/QtDavaConvertion.h"
#include "UI/Dialogs/DialogConfigurePreset.h"
#include "UI/Dialogs/DialogAddPreset.h"
#include "Modules/LegacySupportModule/Private/Project.h"

#include <QAction>
#include <QComboBox>
#include <QApplication>

using namespace DAVA;

FontPropertyDelegate::FontPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

FontPropertyDelegate::~FontPropertyDelegate()
{
}

QWidget* FontPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QComboBox* comboBox = new QComboBox(parent);
    comboBox->setObjectName("comboBox");
    comboBox->addItem("");
    DVASSERT(context.project != nullptr);
    project = context.project;
    comboBox->addItems(project->GetDefaultPresetNames());
    connect(comboBox, &QComboBox::currentTextChanged, this, &FontPropertyDelegate::valueChanged);
    return comboBox;
}

void FontPropertyDelegate::setEditorData(QWidget* rawEditor, const QModelIndex& index) const
{
    QComboBox* editor = rawEditor->findChild<QComboBox*>("comboBox");

    Any variant = index.data(Qt::EditRole).value<Any>();
    editor->blockSignals(true);
    editor->setCurrentText(QString::fromStdString(variant.Cast<String>()));
    editor->blockSignals(false);
    configurePresetAction->setDisabled(editor->currentText().isEmpty());
}

bool FontPropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    QComboBox* editor = rawEditor->findChild<QComboBox*>("comboBox");

    Any value = index.data(Qt::EditRole).value<Any>();
    String str = QStringToString(editor->currentText());
    value.Set(str);

    QVariant variant;
    variant.setValue<Any>(value);

    return model->setData(index, variant, Qt::EditRole);
}

void FontPropertyDelegate::enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions)
{
    configurePresetAction = new QAction(QIcon(":/Icons/configure.png"), tr("configure"), parent);
    configurePresetAction->setEnabled(false);
    configurePresetAction->setToolTip(tr("configure preset"));
    actions.push_back(configurePresetAction);
    connect(configurePresetAction, &QAction::triggered, this, &FontPropertyDelegate::configurePresetClicked);

    QAction* addPresetAction = new QAction(QIcon(":/Icons/add.png"), tr("configure"), parent);
    addPresetAction->setToolTip(tr("add preset"));
    actions.push_back(addPresetAction);
    connect(addPresetAction, &QAction::triggered, this, &FontPropertyDelegate::addPresetClicked);

    BasePropertyDelegate::enumEditorActions(parent, index, actions);
}

void FontPropertyDelegate::addPresetClicked()
{
    QAction* editPresetAction = qobject_cast<QAction*>(sender());
    if (!editPresetAction)
        return;

    QWidget* editor = editPresetAction->parentWidget();
    if (!editor)
        return;
    QComboBox* comboBox = editor->findChild<QComboBox*>("comboBox");
    DialogAddPreset dialogAddPreset(project->GetEditorFontSystem(), comboBox->currentText(), qApp->activeWindow());
    if (dialogAddPreset.exec())
    {
        comboBox->clear();
        DVASSERT(project != nullptr);
        comboBox->addItems(project->GetDefaultPresetNames());
        comboBox->setCurrentText(dialogAddPreset.GetPresetName());

        BasePropertyDelegate::SetValueModified(editor, true);
        itemDelegate->emitCommitData(editor);
    }
}

void FontPropertyDelegate::configurePresetClicked()
{
    QAction* editPresetAction = qobject_cast<QAction*>(sender());
    if (!editPresetAction)
        return;

    QWidget* editor = editPresetAction->parentWidget();
    if (!editor)
        return;

    QComboBox* comboBox = editor->findChild<QComboBox*>("comboBox");
    DVASSERT(project);
    DialogConfigurePreset dialogConfigurePreset(project->GetEditorFontSystem(), comboBox->currentText(), qApp->activeWindow());
    if (dialogConfigurePreset.exec())
    {
        BasePropertyDelegate::SetValueModified(editor, true);
        itemDelegate->emitCommitData(editor);
    }
}

void FontPropertyDelegate::valueChanged()
{
    QComboBox* comboBox = qobject_cast<QComboBox*>(sender());
    if (nullptr == comboBox)
    {
        return;
    }
    configurePresetAction->setDisabled(comboBox->currentText().isEmpty());
    QWidget* editor = comboBox->parentWidget();
    if (nullptr == editor)
    {
        return;
    }

    BasePropertyDelegate::SetValueModified(editor, true);
    itemDelegate->emitCommitData(editor);
}
