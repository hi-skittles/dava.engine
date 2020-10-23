#include "CubemapEditor/CubemapEditorDialog.h"
#include "CubemapEditor/ClickableQLabel.h"
#include "CubemapEditor/CubemapUtils.h"
#include "Qt/Main/QtUtils.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Project/ProjectManagerData.h"

#include "ui_CubemapEditorDialog.h"

#include "Tools/PathDescriptor/PathDescriptor.h"
#include "ImageTools/ImageTools.h"
#include "QtTools/FileDialogs/FileDialog.h"

#include <TArc/DataProcessing/DataContext.h>

#include <QMouseEvent>
#include <QMessageBox>

using namespace DAVA;

const String CUBEMAP_LAST_FACE_DIR_KEY = "cubemap_last_face_dir";

CubemapEditorDialog::CubemapEditorDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::CubemapEditorDialog)
{
    ui->setupUi(this);

    ui->lblSaving->setVisible(false);

    facesInfo.width = facesInfo.height = 0;
    facesInfo.format = FORMAT_INVALID;

    facePathes.resize(Texture::CUBE_FACE_COUNT, FilePath());

    faceChanged = false;

    ConnectSignals();

    ui->labelPX->SetVisualRotation(90);
    ui->labelNX->SetVisualRotation(90);
    ui->labelPY->SetVisualRotation(90);
    ui->labelNY->SetVisualRotation(90);
    ui->labelPZ->SetVisualRotation(90);
    ui->labelNZ->SetVisualRotation(90);

    setMouseTracking(true);
}

CubemapEditorDialog::~CubemapEditorDialog()
{
    delete ui;
}

void CubemapEditorDialog::ConnectSignals()
{
    connect(ui->labelPX, &ClickableQLabel::OnLabelClicked, this, &CubemapEditorDialog::OnPXClicked);
    connect(ui->labelNX, &ClickableQLabel::OnLabelClicked, this, &CubemapEditorDialog::OnNXClicked);
    connect(ui->labelPY, &ClickableQLabel::OnLabelClicked, this, &CubemapEditorDialog::OnPYClicked);
    connect(ui->labelNY, &ClickableQLabel::OnLabelClicked, this, &CubemapEditorDialog::OnNYClicked);
    connect(ui->labelPZ, &ClickableQLabel::OnLabelClicked, this, &CubemapEditorDialog::OnPZClicked);
    connect(ui->labelNZ, &ClickableQLabel::OnLabelClicked, this, &CubemapEditorDialog::OnNZClicked);

    connect(ui->labelPX, &ClickableQLabel::OnRotationChanged, this, &CubemapEditorDialog::OnRotationChanged);
    connect(ui->labelNX, &ClickableQLabel::OnRotationChanged, this, &CubemapEditorDialog::OnRotationChanged);
    connect(ui->labelPY, &ClickableQLabel::OnRotationChanged, this, &CubemapEditorDialog::OnRotationChanged);
    connect(ui->labelNY, &ClickableQLabel::OnRotationChanged, this, &CubemapEditorDialog::OnRotationChanged);
    connect(ui->labelPZ, &ClickableQLabel::OnRotationChanged, this, &CubemapEditorDialog::OnRotationChanged);
    connect(ui->labelNZ, &ClickableQLabel::OnRotationChanged, this, &CubemapEditorDialog::OnRotationChanged);

    connect(ui->buttonSave, &QPushButton::clicked, this, &CubemapEditorDialog::OnSave);
    connect(ui->buttonClose, &QPushButton::clicked, this, &CubemapEditorDialog::close);
}

void CubemapEditorDialog::LoadImageFromUserFile(float rotation, int face)
{
    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data != nullptr);

    CommonInternalSettings* settings = REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>();
    FilePath projectPath = settings->cubemapLastFaceDir;
    if (projectPath.IsEmpty() == true)
    {
        projectPath = data->GetDataSource3DPath();
    }

    QString fileName = FileDialog::getOpenFileName(this,
                                                   tr("Open Cubemap Face Image"),
                                                   QString::fromStdString(projectPath.GetAbsolutePathname()),
                                                   PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter);

    if (!fileName.isNull())
    {
        String stdFilePath = fileName.toStdString();
        FilePath path(stdFilePath);
        LoadImageTo(path, face, false);

        projectPath = stdFilePath;
        CommonInternalSettings* settings = REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>();
        settings->cubemapLastFaceDir = projectPath.GetDirectory();

        if (AllFacesLoaded())
        {
            ui->legend->setVisible(false);
        }
    }
}

bool CubemapEditorDialog::LoadImageTo(const DAVA::FilePath& filePath, int face, bool silent)
{
    DVASSERT(face >= 0 && face <= Texture::CUBE_FACE_COUNT);

    bool result = true;

    QString fileName = filePath.GetAbsolutePathname().c_str();
    QString errorString;
    ImageInfo loadedImageInfo = ImageSystem::GetImageInfo(filePath);

    bool verified = false;
    bool isFirstImage = false;
    auto loaded = GetLoadedFaceCount();
    if (loaded > 1 || (loaded == 1 && facePathes[face].IsEmpty()))
    {
        verified = VerifyNextImage(loadedImageInfo, errorString);
    }
    else
    {
        isFirstImage = true;
        verified = VerifyFirstImage(loadedImageInfo, errorString);
    }

    if (verified)
    {
        QImage faceImage = ImageTools::FromDavaImage(filePath);

        ClickableQLabel* label = GetLabelForFace(face);
        QImage scaledFace = faceImage.scaled(label->width(), label->height());
        label->setPixmap(QPixmap::fromImage(scaledFace));
        label->SetFaceLoaded(true);
        label->SetRotation(0);

        facePathes[face] = filePath;

        if (isFirstImage)
        {
            facesInfo = loadedImageInfo;
            UpdateFaceInfo();
        }

        faceChanged = true;

        UpdateButtonState();
    }
    else
    {
        if (!silent)
        {
            QString message = QString("%1\n is not suitable as current cubemap face!\n%2").arg(fileName).arg(errorString);
            DAVA::Logger::Error(message.toStdString().c_str());
        }

        result = false;
    }

    return result;
}

ClickableQLabel* CubemapEditorDialog::GetLabelForFace(int face)
{
    ClickableQLabel* labels[] =
    {
      ui->labelPX,
      ui->labelNX,
      ui->labelPY,
      ui->labelNY,
      ui->labelPZ,
      ui->labelNZ
    };

    return labels[face];
}

bool CubemapEditorDialog::VerifyFirstImage(ImageInfo imgInfo, QString& errorString)
{
    if (imgInfo.width != imgInfo.height)
    {
        errorString = QString("Width and height are not equal");
        return false;
    }
    else if (!IsPowerOf2(imgInfo.width))
    {
        errorString = QString("Width or height are not power of two");
        return false;
    }

    return true;
}

bool CubemapEditorDialog::VerifyNextImage(ImageInfo imgInfo, QString& errorString)
{
    if (imgInfo.height != facesInfo.height || imgInfo.width != facesInfo.width)
    {
        errorString = QString("Image size is %1 x %2, should be %3 x %4")
                      .arg(QString::number(imgInfo.width))
                      .arg(QString::number(imgInfo.height))
                      .arg(QString::number(facesInfo.width))
                      .arg(QString::number(facesInfo.height));
        return false;
    }
    else if (imgInfo.format != facesInfo.format)
    {
        errorString = QString("Image format is %1, should be %3")
                      .arg(GlobalEnumMap<PixelFormat>::Instance()->ToString(imgInfo.format))
                      .arg(GlobalEnumMap<PixelFormat>::Instance()->ToString(facesInfo.format));
        return false;
    }
    else
    {
        return true;
    }
}

void CubemapEditorDialog::UpdateFaceInfo()
{
    ui->labelFaceHeight->setText(QString::number(facesInfo.height));
    ui->labelFaceWidth->setText(QString::number(facesInfo.width));
}

void CubemapEditorDialog::UpdateButtonState()
{
    //check if all files are present.
    //while file formats specs allow to specify cubemaps partially actual implementations don't allow that
    bool enableSave = AllFacesLoaded() && IsCubemapEdited();
    ui->buttonSave->setEnabled(enableSave);
}

bool CubemapEditorDialog::AnyFaceLoaded()
{
    bool faceLoaded = false;
    for (auto& nextFacePath : facePathes)
    {
        if (!nextFacePath.IsEmpty())
        {
            faceLoaded = true;
            break;
        }
    }

    return faceLoaded;
}

bool CubemapEditorDialog::AllFacesLoaded()
{
    bool faceLoaded = true;
    for (auto& nextFacePath : facePathes)
    {
        if (nextFacePath.IsEmpty())
        {
            faceLoaded = false;
            break;
        }
    }

    return faceLoaded;
}

int CubemapEditorDialog::GetLoadedFaceCount()
{
    int faceLoaded = 0;
    for (auto& nextFacePath : facePathes)
    {
        if (!nextFacePath.IsEmpty())
        {
            ++faceLoaded;
        }
    }

    return faceLoaded;
}

void CubemapEditorDialog::LoadCubemap(const QString& path)
{
    FilePath filePath(path.toStdString());
    std::unique_ptr<TextureDescriptor> texDescriptor(TextureDescriptor::CreateFromFile(filePath));

    if (texDescriptor && texDescriptor->IsCubeMap())
    {
        Vector<FilePath> faceNames;
        texDescriptor->GetFacePathnames(faceNames);
        bool cubemapLoadResult = true;

        for (auto i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
        {
            bool faceLoadResult = LoadImageTo(faceNames[i].GetAbsolutePathname(), i, true);
            cubemapLoadResult = cubemapLoadResult && faceLoadResult;
        }

        if (!cubemapLoadResult)
        {
            DAVA::Logger::Error("This cubemap texture seems to be damaged.\nPlease repair it by setting image(s) to empty face(s) and save to disk.");
        }
    }
    else if (!texDescriptor)
    {
        DAVA::Logger::Error("Failed to load cubemap texture %s", path.toStdString().c_str());
    }
    else
    {
        DAVA::Logger::Error("Failed to load cubemap texture %s. Seems this is not a cubemap texture.", path.toStdString().c_str());
    }
}

void CubemapEditorDialog::SaveCubemap(const QString& path)
{
    FilePath filePath(path.toStdString());
    DAVA::uint8 faceMask = GetFaceMask();

    std::unique_ptr<TextureDescriptor> descriptor(new TextureDescriptor());
    bool descriptorReady = false;
    if (FileSystem::Instance()->Exists(filePath))
    {
        descriptorReady = descriptor->Load(filePath);
    }

    if (!descriptorReady)
    {
        descriptor->SetDefaultValues();
        descriptor->drawSettings.wrapModeS = descriptor->drawSettings.wrapModeT = rhi::TEXADDR_CLAMP;
        descriptor->pathname = filePath;
    }

    descriptor->dataSettings.cubefaceFlags = faceMask;

    Vector<FilePath> targetFacePathes;
    descriptor->GetFacePathnames(targetFacePathes);

    //copy file to the location where .tex will be put. Add suffixes to file names to distinguish faces
    for (int i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
    {
        if (!facePathes[i].IsEmpty())
        {
            DVASSERT(!targetFacePathes[i].IsEmpty());

            String ext = facePathes[i].GetExtension();
            descriptor->dataSettings.cubefaceExtensions[i] = ext;

            if (!targetFacePathes[i].IsEqualToExtension(ext))
            {
                targetFacePathes[i].ReplaceExtension(ext);
            }

            auto facePathString = facePathes[i].GetAbsolutePathname();
            auto targetFacePathString = targetFacePathes[i].GetAbsolutePathname();

            if (facePathes[i] != targetFacePathes[i])
            {
                if (QFile::exists(targetFacePathString.c_str()))
                {
                    int answer = ShowQuestion("File overwrite",
                                              "File " + targetFacePathString + " already exist. Do you want to overwrite it with " + facePathString,
                                              MB_FLAG_YES | MB_FLAG_NO, MB_FLAG_NO);

                    if (MB_FLAG_YES == answer)
                    {
                        bool removeResult = QFile::remove(targetFacePathString.c_str());

                        if (!removeResult)
                        {
                            DAVA::Logger::Error("Failed to copy texture %s to %s", facePathString.c_str(), targetFacePathString.c_str());
                            return;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }

                bool copyResult = QFile::copy(facePathString.c_str(), targetFacePathString.c_str());

                if (!copyResult)
                {
                    DAVA::Logger::Error("Failed to copy texture %s to %s", facePathString.c_str(), targetFacePathString.c_str());
                    return;
                }
            }

            ClickableQLabel* faceLabel = GetLabelForFace(i);
            if (faceLabel->GetRotation() != 0)
            {
                ScopedPtr<Image> image(DAVA::ImageSystem::LoadSingleMip(targetFacePathes[i]));
                image->RotateDeg(faceLabel->GetRotation());
                ImageSystem::Save(targetFacePathes[i], image);
                faceLabel->SetRotation(0);
            }
        }
    }

    descriptor->Save(filePath);

    QMessageBox::information(this, "Cubemap texture save result", "Cubemap texture was saved successfully!");
}

DAVA::uint8 CubemapEditorDialog::GetFaceMask()
{
    DAVA::uint8 mask = 0;
    for (int i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
    {
        if (!facePathes[i].IsEmpty())
        {
            mask |= 1 << i;
        }
    }

    return mask;
}

void CubemapEditorDialog::InitForEditing(DAVA::FilePath& textureDescriptorPath, DAVA::FilePath& root)
{
    targetFile = textureDescriptorPath;
    editorMode = CubemapEditorDialog::eEditorModeEditing;
    rootPath = QString::fromStdString(root.GetAbsolutePathname());

    LoadCubemap(targetFile.GetAbsolutePathname().c_str());

    ui->buttonSave->setEnabled(false);
    ui->legend->setVisible(false);

    faceChanged = false;
}

void CubemapEditorDialog::InitForCreating(DAVA::FilePath& textureDescriptorPath, DAVA::FilePath& root)
{
    targetFile = textureDescriptorPath;
    editorMode = CubemapEditorDialog::eEditorModeCreating;
    rootPath = QString::fromStdString(root.GetAbsolutePathname());
    ui->legend->setVisible(true);

    faceChanged = false;
}

////////////////////////////////////////////////////

void CubemapEditorDialog::OnPXClicked()
{
    LoadImageFromUserFile(0, rhi::TEXTURE_FACE_POSITIVE_X);
}

void CubemapEditorDialog::OnNXClicked()
{
    LoadImageFromUserFile(0, rhi::TEXTURE_FACE_NEGATIVE_X);
}

void CubemapEditorDialog::OnPYClicked()
{
    LoadImageFromUserFile(0, rhi::TEXTURE_FACE_POSITIVE_Y);
}

void CubemapEditorDialog::OnNYClicked()
{
    LoadImageFromUserFile(0, rhi::TEXTURE_FACE_NEGATIVE_Y);
}

void CubemapEditorDialog::OnPZClicked()
{
    LoadImageFromUserFile(0, rhi::TEXTURE_FACE_POSITIVE_Z);
}

void CubemapEditorDialog::OnNZClicked()
{
    LoadImageFromUserFile(0, rhi::TEXTURE_FACE_NEGATIVE_Z);
}

void CubemapEditorDialog::OnLoadTexture()
{
    int answer = MB_FLAG_YES;
    if (AnyFaceLoaded())
    {
        answer = ShowQuestion("Warning",
                              "Do you really want to load new cubemap texture over currently loaded one?",
                              MB_FLAG_YES | MB_FLAG_NO,
                              MB_FLAG_NO);
    }

    if (MB_FLAG_YES == answer)
    {
        QString fileName = FileDialog::getOpenFileName(this,
                                                       tr("Open Cubemap Texture"),
                                                       rootPath,
                                                       tr("Tex File (*.tex)"));

        if (!fileName.isNull())
        {
            LoadCubemap(fileName);
        }
    }
}

void CubemapEditorDialog::OnSave()
{
    //check if all files are present.
    //while file formats specs allows to specify cubemaps partially actual implementations don't allow that
    if (!AllFacesLoaded())
    {
        DAVA::Logger::Error("Please specify at least one cube face.");
        return;
    }

    ui->lblSaving->setVisible(true);

    this->paintEvent(NULL);
    ui->lblSaving->update();
    QApplication::processEvents();
    QApplication::flush();

    this->setUpdatesEnabled(false);

    SaveCubemap(targetFile.GetAbsolutePathname().c_str());

    faceChanged = false;

    this->setUpdatesEnabled(true);
    ui->lblSaving->setVisible(false);
    QDialog::accept();
}

void CubemapEditorDialog::done(int result)
{
    if (IsCubemapEdited())
    {
        int answer = ShowQuestion("Warning",
                                  "Cubemap texture was edited. Do you want to close it without saving?",
                                  MB_FLAG_YES | MB_FLAG_NO,
                                  MB_FLAG_NO);

        if (answer != MB_FLAG_YES)
        {
            return;
        }
    }
    QDialog::done(QDialog::Accepted);
}

bool CubemapEditorDialog::IsCubemapEdited()
{
    bool edited = faceChanged;

    if (!edited)
    {
        ClickableQLabel* labels[] =
        {
          ui->labelPX,
          ui->labelNX,
          ui->labelPY,
          ui->labelNY,
          ui->labelPZ,
          ui->labelNZ
        };

        for (int i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
        {
            if (labels[i]->GetRotation() != 0)
            {
                edited = true;
                break;
            }
        }
    }

    return edited;
}

void CubemapEditorDialog::OnRotationChanged()
{
}

void CubemapEditorDialog::mouseMoveEvent(QMouseEvent* ev)
{
    ClickableQLabel* labels[] =
    {
      ui->labelPX,
      ui->labelNX,
      ui->labelPY,
      ui->labelNY,
      ui->labelPZ,
      ui->labelNZ
    };

    for (int i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
    {
        labels[i]->OnParentMouseMove(ev);
    }

    QDialog::mouseMoveEvent(ev);
}
