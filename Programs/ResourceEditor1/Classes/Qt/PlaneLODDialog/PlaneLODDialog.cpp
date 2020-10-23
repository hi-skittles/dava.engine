#include <QMessageBox>
#include "PlaneLODDialog.h"
#include "ui_planeloddialog.h"

#include "Qt/Main/mainwindow.h"
#include "Qt/Main/QtUtils.h"
#include "Tools/PathDescriptor/PathDescriptor.h"

#include "QtTools/FileDialogs/FileDialog.h"

using namespace DAVA;

PlaneLODDialog::PlaneLODDialog(DAVA::uint32 layersCount, const DAVA::FilePath& defaultTexturePath, QWidget* parent /*= 0*/)
    : QDialog(parent)
    , ui(new Ui::QtPlaneLODDialog)
    , selectedLayer(-1)
    , selectedTextureSize(0)
{
    ui->setupUi(this);

    connect(this, SIGNAL(accepted()), this, SLOT(OnOk()));
    connect(this, SIGNAL(rejected()), this, SLOT(OnCancel()));
    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    connect(ui->textureButton, SIGNAL(clicked()), this, SLOT(OnTextureSelect()));

    for (uint32 i = 0; i < layersCount; i++)
        ui->lodLevelBox->addItem(QString("LOD %1").arg(i));
    ui->lodLevelBox->setCurrentIndex(layersCount - 1);

    texturePath = QString(defaultTexturePath.GetAbsolutePathname().c_str());
    ui->textureLineEdit->setText(texturePath);

    setWindowModality(Qt::WindowModal);
}

PlaneLODDialog::~PlaneLODDialog()
{
}

void PlaneLODDialog::OnCancel()
{
}

void PlaneLODDialog::OnOk()
{
    selectedLayer = ui->lodLevelBox->currentIndex();

    bool isOK = false;
    uint32 selectedSize = ui->textureSizeBox->currentText().toUInt(&isOK);
    if (isOK)
        selectedTextureSize = selectedSize;
}

void PlaneLODDialog::OnTextureSelect()
{
    QString selectedPath = FileDialog::getSaveFileName(this, QString("Save texture"), texturePath, PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter);
    if (selectedPath.isEmpty())
        return;

    texturePath = selectedPath;
    ui->textureLineEdit->setText(texturePath);
}

int32 PlaneLODDialog::GetSelectedLayer()
{
    return selectedLayer;
}

FilePath PlaneLODDialog::GetSelectedTexturePath()
{
    return FilePath(texturePath.toStdString());
}

uint32 PlaneLODDialog::GetSelectedTextureSize()
{
    return selectedTextureSize;
}