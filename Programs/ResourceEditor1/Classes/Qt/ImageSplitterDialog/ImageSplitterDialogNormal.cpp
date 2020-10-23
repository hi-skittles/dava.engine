#include "ImageSplitterDialog/ImageSplitterDialogNormal.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/SceneManager/SceneData.h"

#include "Render/PixelFormatDescriptor.h"
#include "Render/RenderBase.h"
#include "Scene3D/Components/ComponentHelpers.h"

#include "ImageTools/ImageTools.h"
#include "Main/mainwindow.h"
#include "Main/QtUtils.h"

#include <QMessageBox>
#include "ui_ImageSplitterNormal.h"

ImageSplitterDialogNormal::ImageSplitterDialogNormal(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ImageSplitterNormal())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    imageArreas[0] = ui->redImgLbl;
    imageArreas[1] = ui->greenImgLbl;
    imageArreas[2] = ui->blueImgLbl;
    imageArreas[3] = ui->alphaImgLbl;

    for (auto& imageArea : imageArreas)
    {
        imageArea->SetRequestedImageFormat(DAVA::FORMAT_RGBA8888);
    }

    connect(ui->saveBtn, &QPushButton::clicked, this, &ImageSplitterDialogNormal::OnSaveClicked);
}

ImageSplitterDialogNormal::~ImageSplitterDialogNormal()
{
}

void ImageSplitterDialogNormal::OnSaveClicked()
{
    SceneData* data = REGlobal::GetActiveDataNode<SceneData>();
    if (data == nullptr || data->GetScene().Get() == nullptr)
    {
        return;
    }

    SceneEditor2* scene = data->GetScene().Get();
    DAVA::Landscape* landscape = nullptr;
    if (scene != nullptr)
    {
        landscape = DAVA::FindLandscape(scene);
    }

    if (nullptr == landscape)
    {
        QMessageBox::warning(this, "Save error", "Scene has no landscape. Cannot create normals.", QMessageBox::Ok);
        return;
    }

    for (size_t i = 1; i < imageArreas.size(); ++i)
    {
        auto image = imageArreas[i]->GetImage();
        auto prevImage = imageArreas[i - 1]->GetImage();

        if ((image->GetWidth() != prevImage->GetWidth()) || (image->GetHeight() != prevImage->GetHeight()))
        {
            QMessageBox::warning(this, "Save error", QString("Images [%1] and [%2] have different size").arg(i - 1).arg(i), QMessageBox::Ok);
            return;
        }
    }

    auto heightmapPath = landscape->GetHeightmapPathname();

    auto normal1Path(heightmapPath);
    normal1Path.ReplaceFilename("Normal1.png");
    SaveAndReloadNormal(normal1Path, RED, GREEN);

    auto normal2Path(heightmapPath);
    normal2Path.ReplaceFilename("Normal2.png");
    SaveAndReloadNormal(normal2Path, BLUE, ALPHA);
}

void ImageSplitterDialogNormal::SaveAndReloadNormal(const DAVA::FilePath& pathname, int first, int second)
{
    DAVA::ScopedPtr<DAVA::Image> mergedImage(CreateMergedImage(imageArreas[first]->GetImage(), imageArreas[second]->GetImage()));
    SaveImageToFile(mergedImage, pathname);

    auto texture = DAVA::Texture::Get(DAVA::TextureDescriptor::GetDescriptorPathname(pathname));
    if (texture)
    {
        texture->Reload();
        texture->Release();
    }
}

DAVA::Image* ImageSplitterDialogNormal::CreateMergedImage(DAVA::Image* firstImage, DAVA::Image* secondImage)
{
    DVASSERT(firstImage->format == DAVA::FORMAT_RGBA8888);
    DVASSERT(secondImage->format == DAVA::FORMAT_RGBA8888);

    auto mergedImage = DAVA::Image::Create(firstImage->width, firstImage->height, DAVA::FORMAT_RGBA8888);

    DAVA::uint32 size = firstImage->width * firstImage->height;
    static const DAVA::uint32 bytesInPixel = 4;
    DVASSERT(CHANNELS_COUNT == bytesInPixel);

    for (DAVA::uint32 i = 0; i < size; ++i)
    {
        DAVA::uint32 offset = i * bytesInPixel;

        mergedImage->data[offset + RED] = firstImage->data[offset + RED];
        mergedImage->data[offset + GREEN] = firstImage->data[offset + GREEN];

        mergedImage->data[offset + BLUE] = secondImage->data[offset + RED];
        mergedImage->data[offset + ALPHA] = secondImage->data[offset + GREEN];
    }

    return mergedImage;
}
