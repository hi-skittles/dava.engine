#include "DialogAddPreset.h"

#include "Modules/ProjectModule/Private/EditorFontSystem.h"
#include "ui_DialogAddPreset.h"

#include <QCompleter>
#include <QPushButton>

using namespace DAVA;

DialogAddPreset::DialogAddPreset(EditorFontSystem* editorFontSystem_, const QString& originalPresetName_, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DialogAddPreset())
    , editorFontSystem(editorFontSystem_)
{
    ui->setupUi(this);
    ui->comboBox_baseFontPresetName->addItem("");
    ui->comboBox_baseFontPresetName->addItems(editorFontSystem->GetDefaultPresetNames());
    ui->comboBox_baseFontPresetName->setCurrentText(originalPresetName_);
    ui->lineEdit_newFontPresetName->setText(originalPresetName_);

    ui->lineEdit_newFontPresetName->setCompleter(new QCompleter(editorFontSystem->GetDefaultPresetNames(), ui->lineEdit_newFontPresetName));

    connect(ui->lineEdit_newFontPresetName, &QLineEdit::textChanged, this, &DialogAddPreset::OnNewPresetNameChanged);
    connect(ui->comboBox_baseFontPresetName, &QComboBox::currentTextChanged, this, &DialogAddPreset::OnNewPresetNameChanged);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DialogAddPreset::reject);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogAddPreset::OnAccept);

    OnNewPresetNameChanged();
}

DialogAddPreset::~DialogAddPreset() = default;

QString DialogAddPreset::GetPresetName() const
{
    return ui->lineEdit_newFontPresetName->text();
}

void DialogAddPreset::OnNewPresetNameChanged()
{
    QString baseName = ui->comboBox_baseFontPresetName->currentText();
    QString newName = ui->lineEdit_newFontPresetName->text();
    QString resultText;
    bool enabled = false;
    if (newName.isEmpty())
    {
        enabled = false;
        resultText = tr("enter new preset name");
    }
    else if (baseName == newName)
    {
        enabled = false;
        resultText = tr("names match!");
    }
    else if (editorFontSystem->GetDefaultPresetNames().contains(newName))
    {
        enabled = false;
        resultText = tr("This preset name already exists in the system");
    }
    else
    {
        enabled = true;
        resultText = tr("New font preset will be created");
    }
    ui->label_info->setText(resultText);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

void DialogAddPreset::OnAccept()
{
    if (!editorFontSystem->GetDefaultPresetNames().contains(ui->lineEdit_newFontPresetName->text()))
    {
        editorFontSystem->CreateNewPreset(ui->comboBox_baseFontPresetName->currentText().toStdString(),
                                          ui->lineEdit_newFontPresetName->text().toStdString());
        editorFontSystem->SaveLocalizedFonts();
    }
    accept();
}