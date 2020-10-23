#include "Classes/Qt/Scene/Validation/SceneValidationDialog.h"
#include "Classes/Qt/Scene/Validation/SceneValidation.h"
#include "Classes/Qt/Scene/Validation/ValidationProgressConsumer.h"

#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Deprecated/EditorConfig.h>
#include <TArc/Core/Deprecated.h>
#include <QtTools/ConsoleWidget/LoggerOutputObject.h>

#include <QCloseEvent>

#include "ui_SceneValidationDialog.h"

SceneValidationDialog::SceneValidationDialog(DAVA::Scene* scene, QWidget* parent)
    : QDialog(parent)
    , scene(scene)
    , ui(new Ui::SceneValidationDialog)
{
    ui->setupUi(this);

    connect(ui->selectAllCheckBox, &QCheckBox::clicked, this, &SceneValidationDialog::OnSelectAllClicked);
    connect(ui->buttonGroup, SIGNAL(buttonToggled(int, bool)), this, SLOT(OnOptionToggled(int, bool)));
    connect(ui->validateButton, &QPushButton::clicked, this, &SceneValidationDialog::Validate);
    connect(ui->showConsoleCheckBox, &QCheckBox::toggled, this, &SceneValidationDialog::ShowConsole);

    LoadOptions();
    ui->validateButton->setEnabled(!AreAllOptionsSetTo(false));
    ShowConsole(ui->showConsoleCheckBox->isChecked());

    LoggerOutputObject* loggerOutput = new LoggerOutputObject(this);
    connect(loggerOutput, &LoggerOutputObject::OutputReady, ui->logWidget, &LogWidget::AddMessage, Qt::DirectConnection);
}

void SceneValidationDialog::LoadOptions()
{
    DAVA::CommonInternalSettings* settings = DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>();

    ui->matriciesCheckBox->setChecked(settings->validateMatrices);
    ui->sameNamesCheckBox->setChecked(settings->validateSameNames);
    ui->collisionsCheckBox->setChecked(settings->validateCollisionProperties);
    ui->relevanceCheckBox->setChecked(settings->validateTextureRelevance);
    ui->materialGroupsCheckBox->setChecked(settings->validateMaterialGroups);
    ui->showConsoleCheckBox->setChecked(settings->validateShowConsole);
}

void SceneValidationDialog::SaveOptions()
{
    DAVA::CommonInternalSettings* settings = DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>();
    settings->validateMatrices = ui->matriciesCheckBox->isChecked();
    settings->validateSameNames = ui->sameNamesCheckBox->isChecked();
    settings->validateCollisionProperties = ui->collisionsCheckBox->isChecked();
    settings->validateTextureRelevance = ui->relevanceCheckBox->isChecked();
    settings->validateMaterialGroups = ui->materialGroupsCheckBox->isChecked();
    settings->validateShowConsole = ui->showConsoleCheckBox->isChecked();
}

void SceneValidationDialog::Validate()
{
    DVASSERT(DAVA::Thread::IsMainThread());

    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data != nullptr);
    const DAVA::EditorConfig* conf = data->GetEditorConfig();
    DVASSERT(conf != nullptr);
    SceneValidation validation(data, conf->GetCollisionTypeMap("CollisionType"));

    ui->validateButton->setEnabled(false);

    ValidationProgressToLog progressToLog;

    if (ui->matriciesCheckBox->isChecked())
    {
        ValidationProgress validationProgress;
        validationProgress.SetProgressConsumer(&progressToLog);
        validation.ValidateMatrices(scene, validationProgress);
    }

    if (ui->sameNamesCheckBox->isChecked())
    {
        ValidationProgress validationProgress;
        validationProgress.SetProgressConsumer(&progressToLog);
        validation.ValidateSameNames(scene, validationProgress);
    }

    if (ui->collisionsCheckBox->isChecked())
    {
        ValidationProgress validationProgress;
        validationProgress.SetProgressConsumer(&progressToLog);
        validation.ValidateCollisionProperties(scene, validationProgress);
    }

    if (ui->relevanceCheckBox->isChecked())
    {
        ValidationProgress validationProgress;
        validationProgress.SetProgressConsumer(&progressToLog);
        validation.ValidateTexturesRelevance(scene, validationProgress);
    }

    if (ui->materialGroupsCheckBox->isChecked())
    {
        ValidationProgress validationProgress;
        validationProgress.SetProgressConsumer(&progressToLog);
        validation.ValidateMaterialsGroups(scene, validationProgress);
    }

    ui->validateButton->setEnabled(true);
}

void SceneValidationDialog::OnSelectAllClicked(bool clicked)
{
    foreach (QAbstractButton* optionCheckBox, ui->buttonGroup->buttons())
    {
        optionCheckBox->setChecked(clicked);
    }
}

bool SceneValidationDialog::AreAllOptionsSetTo(bool value)
{
    foreach (QAbstractButton* optionCheckBox, ui->buttonGroup->buttons())
    {
        if (optionCheckBox->isChecked() != value)
            return false;
    }
    return true;
}

void SceneValidationDialog::OnOptionToggled(int buttonId, bool toggled)
{
    if (toggled)
    {
        ui->selectAllCheckBox->setChecked(AreAllOptionsSetTo(true));
        ui->validateButton->setEnabled(true);
    }
    else
    {
        ui->selectAllCheckBox->setChecked(false);
        ui->validateButton->setEnabled(!AreAllOptionsSetTo(false));
    }
}

void SceneValidationDialog::ShowConsole(bool checked)
{
    ui->logWidget->setVisible(checked);
    ui->showConsoleCheckBox->setChecked(checked);
    if (!checked)
    {
        setFixedHeight(minimumSizeHint().height());
    }
    else
    {
        setMinimumHeight(minimumSizeHint().height());
        setMaximumHeight(QWIDGETSIZE_MAX);
    }
}

void SceneValidationDialog::closeEvent(QCloseEvent* event)
{
    SaveOptions();
    event->accept();
}
