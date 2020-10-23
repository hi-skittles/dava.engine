#include "Classes/Qt/TextureBrowser/TextureConvertor.h"
#include "Classes/Qt/Tools/QtWaitDialog/QtWaitDialog.h"

#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Deprecated/SceneValidator.h>
#include <REPlatform/Scene/SceneHelper.h>
#include <REPlatform/Scene/Utils/ImageTools.h>

#include <TArc/Core/Deprecated.h>

#include <TextureCompression/TextureConverter.h>

#include <Render/Image/LibDdsHelper.h>
#include <FileSystem/FileSystem.h>

#include <QPainter>
#include <QProcess>
#include <QTextOption>
#include <QPushButton>
#include <QLabel>
#include <QFileInfo>
#include <QtConcurrent>
#include <QImage>

TextureConvertor::TextureConvertor()
    : jobIdCounter(1)
    , convertJobQueueSize(0)
    , waitingComletion(0)
    , watcherThumbnail(DAVA::GetEngineContext()->jobManager)
    , watcherOriginal(DAVA::GetEngineContext()->jobManager)
    , watcherConverted(DAVA::GetEngineContext()->jobManager)
{
    // slots will be called in connector(this) thread
    watcherConverted.Init(DAVA::MakeFunction(this, &TextureConvertor::GetConvertedThread), DAVA::MakeFunction(this, &TextureConvertor::ThreadConvertedFinished));
    watcherOriginal.Init(DAVA::MakeFunction(this, &TextureConvertor::GetOriginalThread), DAVA::MakeFunction(this, &TextureConvertor::ThreadOriginalFinished));
    watcherThumbnail.Init(DAVA::MakeFunction(this, &TextureConvertor::GetThumbnailThread), DAVA::MakeFunction(this, &TextureConvertor::ThreadThumbnailFinished));
}

TextureConvertor::~TextureConvertor()
{
    CancelConvert();
}

int TextureConvertor::GetThumbnail(const DAVA::TextureDescriptor* descriptor)
{
    int ret = 0;

    if (descriptor != nullptr)
    {
        // check if requested texture isn't the same that is loading now
        if (watcherThumbnail.IsFinished() || watcherThumbnail.GetCurrentJobItem()->descriptor != descriptor)
        {
            JobItem newJob;
            newJob.id = jobIdCounter++;
            newJob.descriptor = descriptor;

            jobStackThumbnail.push(newJob);
            jobRunNextThumbnail();

            ret = newJob.id;
        }
    }

    return ret;
}

int TextureConvertor::GetOriginal(const DAVA::TextureDescriptor* descriptor)
{
    int ret = 0;

    if (descriptor != nullptr)
    {
        // check if requested texture isn't the same that is loading now
        if (watcherOriginal.IsFinished() || watcherOriginal.GetCurrentJobItem()->descriptor != descriptor)
        {
            JobItem newJob;
            newJob.id = jobIdCounter++;
            newJob.descriptor = descriptor;

            jobStackOriginal.push(newJob);
            jobRunNextOriginal();

            ret = newJob.id;
        }
    }

    return ret;
}

int TextureConvertor::GetConverted(const DAVA::TextureDescriptor* descriptor, DAVA::eGPUFamily gpu,
                                   eTextureConvertMode convertMode /* = CONVERT_NOT_EXISTENT */)
{
    int ret = 0;

    if (descriptor != nullptr)
    {
        JobItem newJob;
        newJob.id = jobIdCounter++;
        newJob.convertMode = convertMode;
        newJob.type = gpu;
        newJob.descriptor = descriptor;

        if (jobStackConverted.push(newJob))
        {
            convertJobQueueSize++;
        }

        jobRunNextConvert();

        ret = newJob.id;
    }

    return ret;
}

int TextureConvertor::Reconvert(DAVA::Scene* scene, eTextureConvertMode convertMode)
{
    int ret = 0;

    if (NULL != scene)
    {
        // get list of all scenes textures
        DAVA::SceneHelper::TextureCollector collector;
        DAVA::SceneHelper::EnumerateSceneTextures(scene, collector);
        DAVA::TexturesMap& allTextures = collector.GetTextures();

        // add jobs to convert every texture
        if (allTextures.size() > 0)
        {
            DAVA::TexturesMap::iterator begin = allTextures.begin();
            DAVA::TexturesMap::iterator end = allTextures.end();

            for (; begin != end; begin++)
            {
                DAVA::TextureDescriptor* descriptor = begin->second->GetDescriptor();

                if (NULL != descriptor)
                {
                    DVASSERT(descriptor->compression);
                    for (int gpu = 0; gpu < DAVA::GPU_DEVICE_COUNT; ++gpu)
                    {
                        if (!DAVA::GPUFamilyDescriptor::IsFormatSupported(static_cast<DAVA::eGPUFamily>(gpu), static_cast<DAVA::PixelFormat>(descriptor->compression[gpu].format)))
                        {
                            continue;
                        }

                        JobItem newJob;
                        newJob.id = jobIdCounter++;
                        newJob.descriptor = descriptor;
                        newJob.convertMode = convertMode;
                        newJob.type = gpu;

                        if (jobStackConverted.push(newJob))
                        {
                            convertJobQueueSize++;
                        }

                        jobRunNextConvert();

                        ret = newJob.id;
                    }
                }
            }
        }
    }

    // 0 means no job were created
    return ret;
}

void TextureConvertor::WaitConvertedAll(QWidget* parent)
{
    DVASSERT(parent != nullptr);
    if (convertJobQueueSize > 0)
    {
        waitDialog = new QtWaitDialog(parent);
        bool hasCancel = false;

        if (jobStackConverted.size() > 0)
        {
            hasCancel = true;
        }

        connect(waitDialog, &QtWaitDialog::canceled, this, &TextureConvertor::waitCanceled);

        waitDialog->SetRange(0, convertJobQueueSize);
        waitDialog->SetValue(convertJobQueueSize - jobStackConverted.size());
        waitDialog->SetMessage(waitStatusText);

        waitingComletion = true;
        waitDialog->Exec("Waiting for conversion completion", waitStatusText, true, hasCancel);

        waitDialog->deleteLater();
        waitDialog = nullptr;
    }
}

void TextureConvertor::CancelConvert()
{
    JobItem* item = jobStackConverted.pop();

    while (NULL != item)
    {
        delete item;
        item = jobStackConverted.pop();
    }
}

void TextureConvertor::jobRunNextThumbnail()
{
    if (watcherThumbnail.IsFinished())
    {
        std::unique_ptr<JobItem> item(jobStackThumbnail.pop());
        if (item != nullptr)
        {
            watcherThumbnail.RunJob(std::move(item));
        }
    }
}

void TextureConvertor::jobRunNextOriginal()
{
    if (watcherOriginal.IsFinished())
    {
        std::unique_ptr<JobItem> item(jobStackOriginal.pop());
        if (item != nullptr)
        {
            watcherOriginal.RunJob(std::move(item));
        }
    }
}

void TextureConvertor::jobRunNextConvert()
{
    if (watcherConverted.IsFinished())
    {
        std::unique_ptr<JobItem> item(jobStackConverted.pop());
        if (item != nullptr)
        {
            const DAVA::TextureDescriptor* desc = item->descriptor;
            int jobType = item->type;
            watcherConverted.RunJob(std::move(item));

            emit ConvertStatusImg(desc->pathname.GetAbsolutePathname().c_str(), jobType);
            emit ConvertStatusQueue(convertJobQueueSize - jobStackConverted.size(), convertJobQueueSize);

            // generate current wait message, that can be displayed by wait dialog
            waitStatusText = "Path: ";
            waitStatusText += desc->pathname.GetAbsolutePathname().c_str();
            waitStatusText += "\n\nGPU: ";
            waitStatusText += GlobalEnumMap<DAVA::eGPUFamily>::Instance()->ToString(jobType);

            if (NULL != waitDialog)
            {
                waitDialog->SetValue(convertJobQueueSize - jobStackConverted.size());
                waitDialog->SetMessage(waitStatusText);
            }
        }
        else
        {
            waitStatusText = "";

            // if no job in stack, emit signal that all jobs are finished
            if (jobStackOriginal.size() == 0 && jobStackConverted.size() == 0)
            {
                emit ReadyConvertedAll();
            }

            convertJobQueueSize = 0;

            emit ConvertStatusImg("", DAVA::GPU_ORIGIN);
            emit ConvertStatusQueue(0, 0);

            if (NULL != waitDialog)
            {
                // close wait dialog
                waitDialog->Reset();
            }
        }
    }
    else
    {
        emit ConvertStatusQueue(convertJobQueueSize - jobStackConverted.size(), convertJobQueueSize);

        if (NULL != waitDialog)
        {
            waitDialog->SetRangeMax(convertJobQueueSize);
        }
    }
}

void TextureConvertor::ThreadThumbnailFinished(const TextureInfo& info, const JobItem* item)
{
    DVASSERT(watcherThumbnail.IsFinished());
    const DAVA::TextureDescriptor* thumbnailDescriptor = item->descriptor;
    emit ReadyThumbnail(thumbnailDescriptor, info);
    jobRunNextThumbnail();
}

void TextureConvertor::ThreadOriginalFinished(const TextureInfo& info, const JobItem* item)
{
    DVASSERT(watcherOriginal.IsFinished());
    const DAVA::TextureDescriptor* originalDescriptor = item->descriptor;
    emit ReadyOriginal(originalDescriptor, info);
    jobRunNextOriginal();
}

void TextureConvertor::ThreadConvertedFinished(const TextureInfo& info, const JobItem* item)
{
    DVASSERT(watcherConverted.IsFinished());
    const DAVA::TextureDescriptor* convertedDescriptor = item->descriptor;
    emit ReadyConverted(convertedDescriptor, static_cast<DAVA::eGPUFamily>(item->type), info);
    jobRunNextConvert();
}

void TextureConvertor::waitCanceled()
{
    CancelConvert();
}

TextureInfo TextureConvertor::GetThumbnailThread(const JobItem* item)
{
    TextureInfo result;

    if (NULL != item && NULL != item->descriptor)
    {
        const DAVA::TextureDescriptor* descriptor = item->descriptor;

        DAVA::uint32 fileSize = 0;
        if (descriptor->IsCubeMap())
        {
            DAVA::Vector<DAVA::FilePath> cubeFaceNames;
            descriptor->GetFacePathnames(cubeFaceNames);

            for (auto& faceName : cubeFaceNames)
            {
                if (faceName.IsEmpty())
                    continue;

                QImage img = DAVA::ImageTools::FromDavaImage(faceName);
                result.images.push_back(img);
                fileSize += QFileInfo(faceName.GetAbsolutePathname().c_str()).size();
            }
        }
        else
        {
            QImage img = DAVA::ImageTools::FromDavaImage(descriptor->GetSourceTexturePathname());
            result.images.push_back(img);
            fileSize = QFileInfo(descriptor->GetSourceTexturePathname().GetAbsolutePathname().c_str()).size();
        }

        result.dataSize = DAVA::ImageTools::GetTexturePhysicalSize(descriptor, DAVA::GPU_ORIGIN);
        result.fileSize = fileSize;
    }

    return result;
}

TextureInfo TextureConvertor::GetOriginalThread(const JobItem* item)
{
    TextureInfo result;

    if (NULL != item && NULL != item->descriptor)
    {
        const DAVA::TextureDescriptor* descriptor = item->descriptor;

        DAVA::uint32 fileSize = 0;
        if (descriptor->IsCubeMap())
        {
            DAVA::Vector<DAVA::FilePath> cubeFaceNames;
            descriptor->GetFacePathnames(cubeFaceNames);

            for (auto& faceName : cubeFaceNames)
            {
                if (faceName.IsEmpty())
                    continue;

                QImage img = DAVA::ImageTools::FromDavaImage(faceName);
                result.images.push_back(img);

                fileSize += QFileInfo(faceName.GetAbsolutePathname().c_str()).size();
            }
        }
        else
        {
            QImage img = DAVA::ImageTools::FromDavaImage(descriptor->GetSourceTexturePathname());
            result.images.push_back(img);
            fileSize = QFileInfo(descriptor->GetSourceTexturePathname().GetAbsolutePathname().c_str()).size();
        }

        result.dataSize = DAVA::ImageTools::GetTexturePhysicalSize(descriptor, DAVA::GPU_ORIGIN);
        result.fileSize = fileSize;

        if (result.images.size())
        {
            result.imageSize.setWidth(result.images[0].width());
            result.imageSize.setHeight(result.images[0].height());
        }
    }

    return result;
}

TextureInfo TextureConvertor::GetConvertedThread(const JobItem* item)
{
    TextureInfo result;

    DAVA::Vector<DAVA::Image*> convertedImages;

    if (NULL != item)
    {
        DAVA::TextureDescriptor* descriptor = (DAVA::TextureDescriptor*)item->descriptor;
        DAVA::eGPUFamily gpu = (DAVA::eGPUFamily)item->type;

        if (NULL != descriptor &&
            gpu >= 0 && gpu < DAVA::GPU_FAMILY_COUNT &&
            descriptor->compression[gpu].format > DAVA::FORMAT_INVALID && descriptor->compression[gpu].format < DAVA::FORMAT_COUNT)
        {
            DAVA::ImageFormat compressedFormat = DAVA::GPUFamilyDescriptor::GetCompressedFileFormat(gpu, (DAVA::PixelFormat)descriptor->compression[gpu].format);
            if (compressedFormat == DAVA::IMAGE_FORMAT_PVR || compressedFormat == DAVA::IMAGE_FORMAT_DDS)
            {
                DAVA::Logger::FrameworkDebug("Starting %s conversion (%s), id %d..., (%s)",
                                             (compressedFormat == DAVA::IMAGE_FORMAT_PVR ? "PVR" : "DDS"),
                                             GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(descriptor->compression[gpu].format),
                                             item->id,
                                             descriptor->pathname.GetAbsolutePathname().c_str());
                convertedImages = ConvertFormat(descriptor, gpu, item->convertMode);
                DAVA::Logger::FrameworkDebug("Done, id %d", item->id);
            }
            else
            {
                DVASSERT(false);
            }

            result.dataSize = DAVA::ImageTools::GetTexturePhysicalSize(descriptor, gpu);

            result.fileSize = QFileInfo(descriptor->CreateMultiMipPathnameForGPU(gpu).GetAbsolutePathname().c_str()).size();

            if (convertedImages.size() && convertedImages[0])
            {
                result.imageSize.setWidth(convertedImages[0]->GetWidth());
                result.imageSize.setHeight(convertedImages[0]->GetHeight());
            }
        }
        else
        {
            if (descriptor)
            {
                DAVA::Logger::FrameworkDebug("%s has no converted image for %s", descriptor->pathname.GetStringValue().c_str(), DAVA::GPUFamilyDescriptor::GetGPUName(gpu).c_str());
            }
            else
            {
                DAVA::Logger::Error("[TextureConvertor::GetConvertedThread] NULL descriptor for job(%d)", item->id);
            }
        }
    }

    if (convertedImages.size() > 0)
    {
        for (size_t i = 0; i < convertedImages.size(); ++i)
        {
            if (convertedImages[i] != NULL)
            {
                QImage img = DAVA::ImageTools::FromDavaImage(convertedImages[i]);
                result.images.push_back(img);

                convertedImages[i]->Release();
            }
            else
            {
                QImage img;
                result.images.push_back(img);
            }
        }
    }
    else
    {
        int stubImageCount = DAVA::Texture::CUBE_FACE_COUNT;
        if (NULL != item)
        {
            DAVA::TextureDescriptor* descriptor = (DAVA::TextureDescriptor*)item->descriptor;
            if (NULL != descriptor &&
                !descriptor->IsCubeMap())
            {
                stubImageCount = 1;
            }
        }

        for (int i = 0; i < stubImageCount; ++i)
        {
            QImage img;
            result.images.push_back(img);
        }
    }

    return result;
}

DAVA::Vector<DAVA::Image*> TextureConvertor::ConvertFormat(DAVA::TextureDescriptor* descriptor, DAVA::eGPUFamily gpu,
                                                           eTextureConvertMode convertMode)
{
    DAVA::Vector<DAVA::Image*> resultImages;
    DAVA::FilePath outputPath = DAVA::TextureConverter::GetOutputPath(*descriptor, gpu);
    if (!outputPath.IsEmpty())
    {
        bool convert = false;

        switch (convertMode)
        {
        case CONVERT_FORCE:
            convert = true;
            break;

        case CONVERT_MODIFIED:
            convert = !descriptor->IsCompressedTextureActual(gpu);
            break;

        case CONVERT_NOT_EXISTENT:
            convert = !DAVA::FileSystem::Instance()->IsFile(outputPath);
            break;

        case CONVERT_NOT_REQUESTED:
            convert = false;
            break;

        default:
            DVASSERT(false && "Invalid case");
            break;
        }

        if (convert)
        {
            DAVA::GeneralSettings* settings = DAVA::Deprecated::GetDataNode<DAVA::GeneralSettings>();
            outputPath = DAVA::TextureConverter::ConvertTexture(*descriptor, gpu, true, settings->compressionQuality);
        }

        DAVA::Vector<DAVA::Image*> davaImages;
        DAVA::ImageSystem::Load(outputPath, davaImages);

        if (davaImages.size() > 0)
        {
            if (!descriptor->IsCubeMap())
            {
                DAVA::Image* image = davaImages[0];
                image->Retain();

                resultImages.push_back(image);
            }
            else
            {
                //select images with mipmap level = 0 for cube map display
                for (size_t i = 0; i < davaImages.size(); ++i)
                {
                    DAVA::Image* image = davaImages[i];
                    if (0 == image->mipmapLevel)
                    {
                        image->Retain();
                        resultImages.push_back(image);
                    }
                }

                if (resultImages.size() < DAVA::Texture::CUBE_FACE_COUNT)
                {
                    DAVA::uint32 imagesToAdd = DAVA::Texture::CUBE_FACE_COUNT - static_cast<DAVA::uint32>(resultImages.size());
                    for (DAVA::uint32 i = 0; i < imagesToAdd; ++i)
                    {
                        resultImages.push_back(NULL);
                    }
                }
            }

            for_each(davaImages.begin(), davaImages.end(), DAVA::SafeRelease<DAVA::Image>);
        }
        else
        {
            int stubImageCount = (descriptor->IsCubeMap()) ? DAVA::Texture::CUBE_FACE_COUNT : 1;
            for (int i = 0; i < stubImageCount; ++i)
            {
                resultImages.push_back(NULL);
            }
        }
    }

    return resultImages;
}
