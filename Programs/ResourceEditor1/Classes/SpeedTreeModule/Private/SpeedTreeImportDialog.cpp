#if defined(__DAVAENGINE_SPEEDTREE__)

#include "Classes/SpeedTreeModule/Private/SpeedTreeImportDialog.h"
#include "SpeedTree/SpeedTreeImporter.h"

#include "Classes/Qt/GlobalOperations.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/Project/ProjectManagerData.h"

#include "ui_treeimportdialog.h"


#include <TArc/DataProcessing/DataContext.h>
#include <QtTools/FileDialogs/FileDialog.h>

#include <QMessageBox>
#include <QWidget>

SpeedTreeImportDialog::SpeedTreeImportDialog(const std::shared_ptr<GlobalOperations>& globlaOperations_, QWidget* parent /*= 0*/)
    : QDialog(parent)
    , ui(new Ui::QtTreeImportDialog)
    , globalOperations(globlaOperations_)
{
    ui->setupUi(this);

    connect(this, SIGNAL(accepted()), this, SLOT(OnOk()));
    connect(this, SIGNAL(rejected()), this, SLOT(OnCancel()));
    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    connect(ui->xmlButton, SIGNAL(clicked()), this, SLOT(OnXMLSelect()));
    connect(ui->sc2Button, SIGNAL(clicked()), this, SLOT(OnSc2Select()));

    setWindowModality(Qt::WindowModal);
}

SpeedTreeImportDialog::~SpeedTreeImportDialog()
{
}

int SpeedTreeImportDialog::exec()
{
    OnXMLSelect();
    if (!xmlFiles.size())
        return 0;

    return QDialog::exec();
}

void SpeedTreeImportDialog::OnCancel()
{
}

void SpeedTreeImportDialog::OnOk()
{
    using namespace DAVA;

    sc2FolderPath = ui->sc2EditLine->text().toStdString();
    sc2FolderPath.MakeDirectoryPathname();

    FilePath texturesDirPath = sc2FolderPath + "images/";

    //make out files
    Vector<FilePath> outFiles = xmlFiles;
    for (size_t i = 0; i < outFiles.size(); ++i)
    {
        outFiles[i].ReplaceDirectory(sc2FolderPath);
        outFiles[i].ReplaceExtension(".sc2");
    }

    //import all trees
    {
        WaitDialogGuard guard(globalOperations, "Importing tree", "Please wait...", 0, 0);
        for (size_t i = 0; i < xmlFiles.size(); ++i)
        {
            SpeedTreeImporter::ImportSpeedTreeFromXML(xmlFiles[i], outFiles[i], texturesDirPath);
        }
    }

    //make info message
    QString message("SpeedTree models: \n");
    for (size_t i = 0; i < outFiles.size(); ++i)
        message += (outFiles[i].GetFilename() + "\n").c_str();
    message += "\nwas imported to:\n" + QString(sc2FolderPath.GetAbsolutePathname().c_str());

    QMessageBox::information(this, "SpeedTree Import", message, QMessageBox::Ok);

    //open imported trees
    if (ui->checkBox->isChecked())
    {
        for (size_t i = 0; i < outFiles.size(); ++i)
        {
            globalOperations->CallAction(GlobalOperations::OpenScene, outFiles[i].GetAbsolutePathname());
        }
    }
}

void SpeedTreeImportDialog::OnXMLSelect()
{
    QString dialogPath;
    if (xmlFiles.size())
        dialogPath = QString(xmlFiles[0].GetDirectory().GetAbsolutePathname().c_str());

    QStringList selectedFiles = FileDialog::getOpenFileNames(globalOperations->GetGlobalParentWidget(), "Import SpeedTree", dialogPath, "SpeedTree RAW File (*.xml)");
    if (!selectedFiles.size())
        return;

    xmlFiles.clear();
    for (DAVA::int32 i = 0; i < selectedFiles.size(); ++i)
        xmlFiles.push_back(DAVA::FilePath(selectedFiles.at(i).toStdString()));

    if (sc2FolderPath.IsEmpty())
    {
        SetSC2FolderValue(REGlobal::GetDataNode<ProjectManagerData>()->GetDataSource3DPath().GetAbsolutePathname().c_str());
    }

    ui->xmlListWidget->clear();
    ui->xmlListWidget->addItems(selectedFiles);
}

void SpeedTreeImportDialog::OnSc2Select()
{
    QString dialogPath = REGlobal::GetDataNode<ProjectManagerData>()->GetProjectPath().GetAbsolutePathname().c_str();
    if (!sc2FolderPath.IsEmpty())
        dialogPath = QString(sc2FolderPath.GetAbsolutePathname().c_str());

    QString selectedPath = FileDialog::getExistingDirectory(globalOperations->GetGlobalParentWidget(), "Select .sc2 file", dialogPath);
    if (selectedPath.isEmpty())
        return;

    SetSC2FolderValue(selectedPath);
}

void SpeedTreeImportDialog::SetSC2FolderValue(const QString& path)
{
    sc2FolderPath = path.toStdString();
    sc2FolderPath.MakeDirectoryPathname();
    ui->sc2EditLine->setText(QString(sc2FolderPath.GetAbsolutePathname().c_str()));
}

#endif //#if defined(__DAVAENGINE_SPEEDTREE__)
