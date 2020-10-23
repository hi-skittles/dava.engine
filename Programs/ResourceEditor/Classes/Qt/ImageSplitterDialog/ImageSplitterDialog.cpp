#include "ui_ImageSplitter.h"

#include "Classes/Qt/ImageSplitterDialog/ImageSplitterDialog.h"
#include "Classes/Qt/ImageSplitterDialog/SizeDialog.h"
#include "Classes/Qt/Tools/PathDescriptor/PathDescriptor.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Scene/Utils/ImageTools.h>

#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Core/Deprecated.h>

#include <Render/Image/ImageSystem.h>

#include <QMessageBox>
#include <QFileInfo>

ImageSplitterDialog::ImageSplitterDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ImageSplitter())
    , acceptableSize(0, 0)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->path->SetFilter(PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter);

    DAVA::FilePath currentPath = DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->imageSplitterPath;
    if (currentPath.IsEmpty())
    {
        currentPath = GetDefaultPath();
    }

    ui->path->SetCurrentFolder(QString::fromStdString(currentPath.GetDirectory().GetAbsolutePathname()));
    ui->path->SetPath(QString::fromStdString(currentPath.GetAbsolutePathname()));
    ui->saveBtn->setFocus();
    lastSelectedFile = "";
    ConnectSignals();
    rgbaControls.push_back(ui->redImgLbl);
    rgbaControls.push_back(ui->greenImgLbl);
    rgbaControls.push_back(ui->blueImgLbl);
    rgbaControls.push_back(ui->alphaImgLbl);
}

ImageSplitterDialog::~ImageSplitterDialog()
{
}

void ImageSplitterDialog::ConnectSignals()
{
    connect(ui->path, SIGNAL(pathChanged(const QString&)), SLOT(PathChanged(const QString&)));

    connect(ui->redImgLbl, SIGNAL(changed()), SLOT(ImageAreaChanged()));
    connect(ui->greenImgLbl, SIGNAL(changed()), SLOT(ImageAreaChanged()));
    connect(ui->blueImgLbl, SIGNAL(changed()), SLOT(ImageAreaChanged()));
    connect(ui->alphaImgLbl, SIGNAL(changed()), SLOT(ImageAreaChanged()));

    connect(ui->restoreBtn, SIGNAL(clicked()), SLOT(OnRestoreClicked()));

    connect(ui->saveAsBtn, SIGNAL(clicked()), SLOT(OnSaveAsClicked()));
    connect(ui->saveBtn, SIGNAL(clicked()), SLOT(OnSaveClicked()));
    connect(ui->saveChannelsBtn, SIGNAL(clicked()), SLOT(OnSaveChannelsClicked()));

    connect(ui->redFillBtn, SIGNAL(clicked()), SLOT(OnFillBtnClicked()));
    connect(ui->greenFillBtn, SIGNAL(clicked()), SLOT(OnFillBtnClicked()));
    connect(ui->blueFillBtn, SIGNAL(clicked()), SLOT(OnFillBtnClicked()));
    connect(ui->alphaFillBtn, SIGNAL(clicked()), SLOT(OnFillBtnClicked()));

    connect(ui->reload, SIGNAL(clicked()), SLOT(OnReload()));
    connect(ui->reloadSpecular, SIGNAL(clicked()), SLOT(OnReloadSpecularMap()));
}

void ImageSplitterDialog::PathChanged(const QString& path)
{
    if (path.isEmpty())
    {
        return;
    }

    DAVA::FilePath defaultPath = ui->path->text().toStdString();
    if (defaultPath.IsEmpty())
    {
        defaultPath = GetDefaultPath();
    }
    DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->imageSplitterPath = defaultPath.GetAbsolutePathname();

    DAVA::FilePath imagePath(path.toStdString());
    DAVA::ScopedPtr<DAVA::Image> image(DAVA::ImageSystem::LoadSingleMip(imagePath));
    if (image && image->GetPixelFormat() == DAVA::FORMAT_RGBA8888)
    {
        lastSelectedFile = imagePath.GetAbsolutePathname();
        SetAcceptableImageSize(DAVA::Vector2(image->GetWidth(), image->GetHeight()));

        DAVA::Channels channels = DAVA::ImageTools::CreateSplittedImages(image);
        ui->redImgLbl->SetImage(channels.red);
        ui->greenImgLbl->SetImage(channels.green);
        ui->blueImgLbl->SetImage(channels.blue);
        ui->alphaImgLbl->SetImage(channels.alpha);
    }
    else
    {
        ui->path->SetCurrentFolder(QString::fromStdString(defaultPath.GetDirectory().GetAbsolutePathname()));
        ui->path->SetPath(QString());
        if (!image)
        {
            QMessageBox::warning(this, "File error", "Couldn't load image.", QMessageBox::Ok);
        }
        else if (image->GetPixelFormat() != DAVA::FORMAT_RGBA8888)
        {
            QMessageBox::warning(this, "File error", "Image must be in RGBA8888 format.", QMessageBox::Ok);
        }
    }
}

void ImageSplitterDialog::ImageAreaChanged()
{
    ImageArea* sender = dynamic_cast<ImageArea*>(QObject::sender());
    DAVA::Image* image = sender->GetImage();
    DAVA::Vector2 senderImageSize;
    if (image != nullptr)
    {
        senderImageSize.Set(image->GetWidth(), image->GetHeight());
    }
    bool isSomeAreaSet = false;
    foreach (ImageArea* control, rgbaControls)
    {
        if (control != sender && control->GetImage())
        {
            isSomeAreaSet = true;
            break;
        }
    }

    SetAcceptableImageSize(senderImageSize);
    // size restriction for current area must be removed
    // in case of all another ones are empty
    if (!isSomeAreaSet)
    {
        sender->SetAcceptableSize(DAVA::Vector2());
    }
}

void ImageSplitterDialog::OnRestoreClicked()
{
    SetAcceptableImageSize(DAVA::Vector2(0, 0));
    ui->redImgLbl->ClearArea();
    ui->greenImgLbl->ClearArea();
    ui->blueImgLbl->ClearArea();
    ui->alphaImgLbl->ClearArea();
    ui->redSpinBox->setValue(0);
    ui->greenSpinBox->setValue(0);
    ui->blueSpinBox->setValue(0);
    ui->alphaSpinBox->setValue(0);

    PathChanged(QString::fromStdString(lastSelectedFile));
}

void ImageSplitterDialog::OnSaveAsClicked(bool saveSplittedImages)
{
    DAVA::FileDialogParams params;
    params.dir = QString::fromStdString(GetDefaultPath());
    params.title = "Select image";
    params.filters = PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter;
    DAVA::FilePath retPath = DAVA::Deprecated::GetUI()->GetSaveFileName(DAVA::mainWindowKey, params).toStdString();
    if (!retPath.IsEmpty())
    {
        Save(retPath, saveSplittedImages);
    }
}

void ImageSplitterDialog::OnSaveClicked()
{
    DAVA::FilePath presentPath = ui->path->text().toStdString();
    if (!DAVA::FileSystem::Instance()->Exists(presentPath))
    {
        OnSaveAsClicked();
        return;
    }

    Save(presentPath, false);
}

void ImageSplitterDialog::OnSaveChannelsClicked()
{
    DAVA::FilePath savePath = ui->path->text().toStdString();
    if (!DAVA::FileSystem::Instance()->Exists(savePath))
    {
        DAVA::DirectoryDialogParams params;
        params.dir = QString::fromStdString(GetDefaultPath());
        params.title = "Select folder to save images";

        QString folder = DAVA::Deprecated::GetUI()->GetExistingDirectory(DAVA::mainWindowKey, params);
        if (folder.isEmpty() || folder.isNull())
        {
            return;
        }

        savePath = folder.toStdString();
        savePath.MakeDirectoryPathname();
    }

    Save(savePath, true);
}

void ImageSplitterDialog::OnFillBtnClicked()
{
    QPushButton* sender = dynamic_cast<QPushButton*>(QObject::sender());
    ImageArea* targetImageArea = NULL;
    QSpinBox* sourceSpinBox = NULL;

    if (sender == ui->redFillBtn)
    {
        targetImageArea = ui->redImgLbl;
        sourceSpinBox = ui->redSpinBox;
    }
    else if (sender == ui->greenFillBtn)
    {
        targetImageArea = ui->greenImgLbl;
        sourceSpinBox = ui->greenSpinBox;
    }
    else if (sender == ui->blueFillBtn)
    {
        targetImageArea = ui->blueImgLbl;
        sourceSpinBox = ui->blueSpinBox;
    }
    else if (sender == ui->alphaFillBtn)
    {
        targetImageArea = ui->alphaImgLbl;
        sourceSpinBox = ui->alphaSpinBox;
    }
    else
    {
        return;
    }
    DAVA::Image* targetImage = targetImageArea->GetImage();
    DAVA::uint8 value = (DAVA::uint8)sourceSpinBox->value();

    if (acceptableSize.IsZero())
    {
        SizeDialog sizeDlg(this);
        if (sizeDlg.exec() == QDialog::Rejected)
        {
            return;
        }
        acceptableSize = sizeDlg.GetSize();
    }

    DAVA::uint32 width = acceptableSize.x;
    DAVA::uint32 height = acceptableSize.y;
    DAVA::Vector<DAVA::uint8> buffer(width * height, 0);
    buffer.assign(buffer.size(), value);
    DAVA::ScopedPtr<DAVA::Image> bufferImg(DAVA::Image::CreateFromData(width, height, DAVA::FORMAT_A8, &buffer[0]));
    if (targetImage == NULL)
    {
        targetImageArea->SetImage(bufferImg);
    }
    else
    {
        targetImage->InsertImage(bufferImg, 0, 0);
        targetImageArea->UpdatePreviewPicture();
    }
}

void ImageSplitterDialog::OnReload()
{
    const DAVA::FilePath path = ui->path->text().toStdString();
    PathChanged(QString::fromStdString(path.GetAbsolutePathname()));
}

void ImageSplitterDialog::OnReloadSpecularMap()
{
    const DAVA::FilePath path = ui->alphaImgLbl->GetImagePath();
    ui->alphaImgLbl->SetImage(path);
}

void ImageSplitterDialog::SetAcceptableImageSize(const DAVA::Vector2& newSize)
{
    acceptableSize = newSize;
    DAVA::String lblText = acceptableSize.IsZero() ? "" : DAVA::Format("%.f * %.f", acceptableSize.x, acceptableSize.y);
    ui->ImageSizeLbl->setText(lblText.c_str());
    ui->redImgLbl->SetAcceptableSize(acceptableSize);
    ui->greenImgLbl->SetAcceptableSize(acceptableSize);
    ui->blueImgLbl->SetAcceptableSize(acceptableSize);
    ui->alphaImgLbl->SetAcceptableSize(acceptableSize);
}

void ImageSplitterDialog::Save(const DAVA::FilePath& filePath, bool saveSplittedImagesSeparately)
{
    if (!DAVA::TextureDescriptor::IsSourceTextureExtension(filePath.GetExtension())
        && !saveSplittedImagesSeparately)
    {
        QMessageBox::warning(this, "Save error", "Wrong file name.", QMessageBox::Ok);
        return;
    }

    DAVA::Channels channels(ui->redImgLbl->GetImage(),
                            ui->greenImgLbl->GetImage(),
                            ui->blueImgLbl->GetImage(),
                            ui->alphaImgLbl->GetImage());

    if (channels.IsEmpty())
    {
        QMessageBox::warning(this, "Save error", "One or more channel is incorrect.", QMessageBox::Ok);
        return;
    }

    if (saveSplittedImagesSeparately)
    {
        DAVA::String directory = filePath.GetDirectory().GetAbsolutePathname();
        DAVA::String baseName = filePath.GetBasename();

        DAVA::ImageSystem::Save(directory + baseName + "_red.png", channels.red, channels.red->format);
        DAVA::ImageSystem::Save(directory + baseName + "_green.png", channels.green, channels.green->format);
        DAVA::ImageSystem::Save(directory + baseName + "_blue.png", channels.blue, channels.blue->format);
        DAVA::ImageSystem::Save(directory + baseName + "_alpha.png", channels.alpha, channels.alpha->format);
    }
    else
    {
        DAVA::Image* mergedImage = DAVA::ImageTools::CreateMergedImage(channels);
        DAVA::ImageSystem::Save(filePath, mergedImage, mergedImage->format);
        DAVA::SafeRelease(mergedImage);
        ui->path->SetPath(QString::fromStdString(filePath.GetAbsolutePathname()));
    }
    QMessageBox::information(this, "Save success", "Save successfull", QMessageBox::Ok);
}

DAVA::String ImageSplitterDialog::GetDefaultPath() const
{
    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data != nullptr);
    return data->GetProjectPath().GetAbsolutePathname();
}
