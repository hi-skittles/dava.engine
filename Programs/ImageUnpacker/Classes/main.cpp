#include "ResaveUtility.h"

#include <DocDirSetup/DocDirSetup.h>

#include <Engine/Engine.h>
#include <CommandLine/CommandLineParser.h>
#include <Render/Image/Image.h>
#include <Render/RenderBase.h>
#include <Render/Image/ImageSystem.h>
#include <Render/Image/ImageConvert.h>
#include <Render/PixelFormatDescriptor.h>
#include <Render/Texture.h>

#include <Debug/DVAssertDefaultHandlers.h>
#include <FileSystem/FileList.h>
#include <Base/BaseTypes.h>

using namespace DAVA;

void PrintUsage()
{
    printf("Usage:\n");

    printf("\t-usage or --help to display this help\n");
    printf("\t-file - pvr or dds file to unpack as png\n");
    printf("\t-folder - folder with pvr or dds files to unpack as png\n");
    printf("\t-saveas -ext -folder - will open png files from folder and save as ext parameter mean\n");
    printf("\t-resave -ext -folder/-file/-filelist - will open and save *.ext files\n");

    printf("\nExample:\n");
    printf("\t-saveas -ext .tga -folder /Users/nickname/test/\n");
    printf("\t-resave -ext .png -folder /Users/TestData/\n");
    printf("\t-resave -file /Users/TestData/test.png\n");
    printf("\t-resave -filelist /Users/TestData/filelist.txt\n");
}

void SaveSingleImage(const FilePath& newImagePath, Image* image)
{
    if ((FORMAT_RGBA8888 == image->format) || (FORMAT_A8 == image->format) || (FORMAT_A16 == image->format))
    {
        ImageSystem::Save(newImagePath, image, image->format);
    }
    else
    {
        ScopedPtr<Image> savedImage(Image::Create(image->width, image->height, FORMAT_RGBA8888));

        bool unpacked = ImageConvert::ConvertImage(image, savedImage);

        if (unpacked)
        {
            ImageSystem::Save(newImagePath, savedImage);
        }
        else
        {
            Logger::Error("Cannot unpack image from %s and save it as %s", PixelFormatDescriptor::GetPixelFormatString(image->format), newImagePath.GetStringValue().c_str());
        }
    }
}

void GenerateFacePathnames(const FilePath& filePath, Vector<FilePath>& faceNames, const String& extension)
{
    faceNames.resize(Texture::CUBE_FACE_COUNT, FilePath());

    String baseName = filePath.GetBasename();
    for (auto face = 0; face < Texture::CUBE_FACE_COUNT; ++face)
    {
        faceNames[face] = filePath;
        faceNames[face].ReplaceFilename(baseName + Texture::FACE_NAME_SUFFIX[face] + extension);
    }
}

void SaveCubemap(const FilePath& newImagePath, const Vector<Image*>& images)
{
    Vector<FilePath> faceNames;
    GenerateFacePathnames(newImagePath, faceNames, ".png");

    for (auto image : images)
    {
        if (0 == image->mipmapLevel)
        {
            SaveSingleImage(faceNames[image->cubeFaceID], image);
        }
    }
}

void UnpackFile(const FilePath& sourceImagePath)
{
    Vector<Image*> images;
    ImageSystem::Load(sourceImagePath, images);

    if (images.size() != 0)
    {
        FilePath imagePathname = FilePath::CreateWithNewExtension(sourceImagePath, ".png");

        Image* image = images[0];

        if (image->cubeFaceID == Texture::INVALID_CUBEMAP_FACE)
        {
            SaveSingleImage(imagePathname, image);
        }
        else
        {
            SaveCubemap(imagePathname, images);
        }

        for_each(images.begin(), images.end(), SafeRelease<Image>);
    }
    else
    {
        Logger::Error("Cannot load file: ", sourceImagePath.GetStringValue().c_str());
    }
}

void UnpackFolder(const FilePath& folderPath)
{
    ScopedPtr<FileList> fileList(new FileList(folderPath));
    for (uint32 fi = 0; fi < fileList->GetCount(); ++fi)
    {
        const FilePath& pathname = fileList->GetPathname(fi);
        if (fileList->IsDirectory(fi) && !fileList->IsNavigationDirectory(fi))
        {
            UnpackFolder(pathname);
        }
        else
        {
            if (pathname.IsEqualToExtension(".pvr") || pathname.IsEqualToExtension(".dds"))
            {
                UnpackFile(pathname);
            }
        }
    }
}

void ResavePNG(const FilePath& folderPath, const String& extension)
{
    ScopedPtr<FileList> fileList(new FileList(folderPath));

    for (uint32 fi = 0; fi < fileList->GetCount(); ++fi)
    {
        const FilePath& pathname = fileList->GetPathname(fi);
        if (fileList->IsDirectory(fi) && !fileList->IsNavigationDirectory(fi))
        {
            ResavePNG(pathname, extension);
        }
        else
        {
            if (pathname.IsEqualToExtension(".png"))
            {
                Vector<Image*> images;
                ImageSystem::Load(pathname, images);

                FilePath tgaPathname = FilePath::CreateWithNewExtension(pathname, extension);
                ImageSystem::Save(tgaPathname, images);

                for_each(images.begin(), images.end(), SafeRelease<Image>);
            }
        }
    }
}

void ProcessImageUnpacker()
{
    FilePath sourceFolderPath = CommandLineParser::GetCommandParam(String("-folder"));
    FilePath sourceFilePath = CommandLineParser::GetCommandParam(String("-file"));

    bool needShowUsage = true;
    if (CommandLineParser::CommandIsFound("-resave"))
    {
        ResaveUtility utility;
        utility.Resave();

        needShowUsage = false;
    }
    else if (CommandLineParser::CommandIsFound("-saveas") && sourceFolderPath.IsEmpty() == false)
    {
        String ext = CommandLineParser::GetCommandParam(String("-ext"));
        if (!ext.empty())
        {
            if (ext[0] != '.')
            {
                ext = "." + ext;
            }

            sourceFolderPath.MakeDirectoryPathname();
            ResavePNG(sourceFolderPath, ext);

            needShowUsage = false;
        }
    }
    else if (sourceFolderPath.IsEmpty() == false)
    {
        sourceFolderPath.MakeDirectoryPathname();
        UnpackFolder(sourceFolderPath);
        needShowUsage = false;
    }
    else if (sourceFilePath.IsEmpty() == false)
    {
        UnpackFile(sourceFilePath);
        needShowUsage = false;
    }

    if (needShowUsage)
    {
        PrintUsage();
    }
}

void Process(Engine& e)
{
    const EngineContext* context = e.GetContext();

    DocumentsDirectorySetup::SetApplicationDocDirectory(context->fileSystem, "AssetCacheClient");

    context->logger->SetLogLevel(Logger::LEVEL_INFO);
    DVASSERT(e.IsConsoleMode() == true);

    if (CommandLineParser::GetCommandsCount() < 2
        || (CommandLineParser::CommandIsFound(String("-usage")))
        || (CommandLineParser::CommandIsFound(String("-help")))
        )
    {
        PrintUsage();
        return;
    }

    ProcessImageUnpacker();
}

int DAVAMain(Vector<String> cmdline)
{
    Assert::AddHandler(Assert::DefaultLoggerHandler);
    Assert::AddHandler(Assert::DefaultDebuggerBreakHandler);

    Engine e;
    e.Init(eEngineRunMode::CONSOLE_MODE, {}, nullptr);

    e.update.Connect([&e](float32)
                     {
                         Process(e);
                         e.QuitAsync(0);
                     });

    return e.Run();
}
