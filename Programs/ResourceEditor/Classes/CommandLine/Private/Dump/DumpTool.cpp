#include "Classes/CommandLine/DumpTool.h"

#include <REPlatform/CommandLine/OptionName.h>
#include <REPlatform/CommandLine/SceneConsoleHelper.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Scene/Utils/SceneDumper.h>

#include <TArc/Utils/ModuleCollection.h>

#include <FileSystem/FilePath.h>
#include <FileSystem/FileSystem.h>
#include <Logger/Logger.h>
#include <Render/GPUFamilyDescriptor.h>
#include <Utils/Utils.h>

bool DumpTool::DumpSingleFile(const DAVA::FilePath& inFile, const DAVA::FilePath& outFile) const
{
    DAVA::FileSystem::Instance()->CreateDirectory(outFile.GetDirectory(), true);
    DAVA::ScopedPtr<DAVA::File> file(DAVA::File::Create(outFile, DAVA::File::WRITE | DAVA::File::CREATE));
    if (file.get() == nullptr)
    {
        DAVA::Logger::Error("Cannot create file with links %s", outFile.GetAbsolutePathname().c_str());
        return false;
    }

    DAVA::FilePath::AddResourcesFolder(resourceDir);
    DAVA::Set<DAVA::FilePath> links = DAVA::SceneDumper::DumpLinks(inFile, mode, compressedGPUs, tags);
    for (const DAVA::FilePath& link : links)
    {
        if (link.IsEmpty() == true || link.GetType() == DAVA::FilePath::PATH_IN_MEMORY)
            continue;

        DAVA::String linkPath;
        if (mode == DAVA::SceneDumper::eMode::COMPRESSED)
        {
            linkPath = link.GetRelativePathname(resourceDir);
        }
        else
        {
            linkPath = link.GetAbsolutePathname();
        }
        file->WriteLine(linkPath);
    }
    DAVA::FilePath::RemoveResourcesFolder(resourceDir);
    return true;
}

DumpTool::DumpTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-dump")
{
    using namespace DAVA;

    options.AddOption(OptionName::Links, VariantType(false), "Target for dumping is links");
    options.AddOption(OptionName::InDir, VariantType(String("")), "Path for Project/DataSource/3d/ folder");
    options.AddOption(OptionName::OutDir, VariantType(String("")), "Path for Project/Data/3d/ folder");
    options.AddOption(OptionName::ResourceDir, VariantType(String("")), "Path to resource dir, only for compressed mode");
    options.AddOption(OptionName::ProcessFile, VariantType(String("")), "Filename from DataSource/3d/ for dumping");
    options.AddOption(OptionName::ProcessFileList, VariantType(String("")), "Absolute Path to file with filenames for dumping");
    options.AddOption(OptionName::QualityConfig, VariantType(String("")), "Full path for quality.yaml file");
    options.AddOption(OptionName::OutFile, VariantType(String("")), "Full path to file to write result of dumping");
    options.AddOption(OptionName::Mode, VariantType(String("")), "Mode of dumping: с - compressed, r - required, e - extended. Extended mode is default.");
    options.AddOption(OptionName::GPU, VariantType(String("all")), "GPU family: PowerVR_iOS, PowerVR_Android, tegra, mali, adreno, origin, dx11. Can be multiple: -gpu mali,adreno,origin", true);
    options.AddOption(OptionName::TagList, VariantType(String("")), "Tags for textures and slots, Can be multiple: -taglist .china,.japan", true);
}

bool DumpTool::SetResourceDir(const DAVA::FilePath& path)
{
    const DAVA::FilePath dir = path.GetDirectory();
    if (resourceDir.IsEmpty() == true)
    {
        resourceDir = DAVA::ProjectManagerData::GetDataSourcePath(dir);
        if (resourceDir.IsEmpty() == true)
        {
            resourceDir = DAVA::ProjectManagerData::GetDataPath(dir);
            if (resourceDir.IsEmpty() == true)
            {
                DAVA::Logger::Error("DataSource or Data folder was not found");
                return false;
            }
        }
    }
    resourceDir.MakeDirectoryPathname();
    return true;
}

bool DumpTool::CreateQualityYaml(const DAVA::FilePath& path)
{
    bool qualityInitialized = DAVA::SceneConsoleHelper::InitializeQualitySystem(options, path);
    if (qualityInitialized == false)
    {
        DAVA::Logger::Error("Cannot create path to quality.yaml from %s", path.GetDirectory().GetAbsolutePathname().c_str());
    }
    return qualityInitialized;
}

bool DumpTool::PostInitInternal()
{
    using namespace DAVA;

    resourceDir = options.GetOption(OptionName::ResourceDir).AsString();
    DAVA::String modeString = options.GetOption(OptionName::Mode).AsString();

    if (resourceDir.IsEmpty() == false && modeString != "c")
    {
        DAVA::Logger::Error("Resource directory can be used only for mode <c(REQUIRED)>");
        return false;
    }

    if (modeString == "r")
    {
        mode = DAVA::SceneDumper::eMode::REQUIRED;
    }
    else if (modeString == "c")
    {
        mode = DAVA::SceneDumper::eMode::COMPRESSED;
    }
    else
    { // now we use extended mode in case of empty string or in case of error
        mode = DAVA::SceneDumper::eMode::EXTENDED;
    }

    DAVA::String fileListPath = options.GetOption(OptionName::ProcessFileList).AsString();
    DAVA::String filename = options.GetOption(OptionName::ProcessFile).AsString();

    inDir = options.GetOption(OptionName::InDir).AsString();
    if (inDir.IsEmpty() == true)
    {
        DAVA::Logger::Error("Input folder was not selected");
        return false;
    }
    inDir.MakeDirectoryPathname();

    if (filename.empty() == false)
    {
        outFile = options.GetOption(OptionName::OutFile).AsString();
        if (outFile.IsEmpty())
        {
            DAVA::Logger::Error("Out file was not selected");
            return false;
        }

        processFileList.push_back(filename);
        commandAction = ACTION_DUMP_FILES;
    }
    else if (fileListPath.empty() == false)
    {
        outDir = options.GetOption(OptionName::OutDir).AsString();
        if (outDir.IsEmpty() == true)
        {
            DAVA::Logger::Error("Out dir path not specified");
            return false;
        }
        outDir.MakeDirectoryPathname();

        DAVA::ScopedPtr<DAVA::File> fileListFile(DAVA::File::Create(fileListPath, DAVA::File::OPEN | DAVA::File::READ));
        if (fileListFile.get() == nullptr)
        {
            DAVA::Logger::Error("Cannot open file list %s", fileListPath.c_str());
            return false;
        }
        do
        {
            DAVA::String path = fileListFile->ReadLine();
            if (path.empty())
            {
                DAVA::Logger::Warning("Empty string in file %s", fileListPath.c_str());
                continue;
            }
            processFileList.push_back(path);
        } while (fileListFile->IsEof() == false);
        commandAction = ACTION_DUMP_FILES;
    }
    else
    {
        DAVA::Logger::Error("Filename was not selected");
        return false;
    }

    DAVA::uint32 count = options.GetOptionValuesCount(OptionName::GPU);
    compressedGPUs.reserve(count);
    for (DAVA::uint32 i = 0; i < count; ++i)
    {
        DAVA::String gpuName = options.GetOption(OptionName::GPU, i).AsString();
        if (gpuName == "all")
        {
            compressedGPUs.clear();
            compressedGPUs.reserve(DAVA::eGPUFamily::GPU_DEVICE_COUNT);
            for (DAVA::int32 gpu = 0; gpu < DAVA::eGPUFamily::GPU_DEVICE_COUNT; ++gpu)
            {
                compressedGPUs.push_back(static_cast<DAVA::eGPUFamily>(gpu));
            }
            break;
        }
        else
        {
            DAVA::eGPUFamily gpu = DAVA::GPUFamilyDescriptor::GetGPUByName(gpuName);
            if (gpu == DAVA::eGPUFamily::GPU_INVALID)
            {
                DAVA::Logger::Error("Wrong gpu name: %s", gpuName.c_str());
            }
            else
            {
                compressedGPUs.push_back(gpu);
            }
        }
    }

    if (compressedGPUs.empty())
    {
        DAVA::Logger::Error("GPU was not selected");
        return false;
    }

    tags.push_back(""); // empty string means non-tagged behavior
    DAVA::uint32 tagsCount = options.GetOptionValuesCount(OptionName::TagList);
    if (tagsCount > 0)
    {
        tags.reserve(tagsCount + 1);
        for (DAVA::uint32 i = 0; i < tagsCount; ++i)
        {
            tags.push_back(options.GetOption(OptionName::TagList, i).AsString());
        }
    }

    DAVA::FindAndRemoveExchangingWithLast(compressedGPUs, DAVA::eGPUFamily::GPU_ORIGIN);

    if (options.GetOption(OptionName::Links).AsBool() == false)
    {
        DAVA::Logger::Error("Target for dumping was not selected");
        return false;
    }

    return true;
}

DAVA::ConsoleModule::eFrameResult DumpTool::OnFrameInternal()
{
    for (const DAVA::String& path : processFileList)
    {
        CreateQualityYaml(inDir + path);
        if (SetResourceDir(inDir + path) == false)
            continue;

        DAVA::FilePath out;
        if (outFile.IsEmpty() == true)
        {
            out = outDir + path;
            out.ReplaceExtension(".dump");
        }
        else
        {
            out = outFile;
        }

        if (DumpSingleFile(inDir + path, out) == false)
        {
            DAVA::Logger::Error("Failed to save links file %s", outFile.GetAbsolutePathname().c_str());
        }
    }
    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

void DumpTool::BeforeDestroyedInternal()
{
    DAVA::SceneConsoleHelper::FlushRHI();
}

void DumpTool::ShowHelpInternal()
{
    CommandLineModule::ShowHelpInternal();

    DAVA::Logger::Info("Examples:");
    DAVA::Logger::Info("\t-dump -indir /Users/SmokeTest/DataSource/3d/ -processfile Maps/11-grass/test_scene.sc2 -outfile /Users/Test/dump.txt -links -mode e -gpu all");
    DAVA::Logger::Info("\t-dump -indir /Users/SmokeTest/DataSource/3d/ -processfile Maps/11-grass/test_scene.sc2 -outfile /Users/Test/dump.txt -links -mode r -gpu mali,adreno");
    DAVA::Logger::Info("\t-dump -indir /Users/SmokeTest/DataSource/3d/ -processfile Maps/11-grass/test_scene.sc2 -outfile /Users/Test/dump.txt -links -mode e -gpu mali,adreno -taglist .china,.japan");
    DAVA::Logger::Info("\t-dump -indir /Users/resources/DataSource/3d/Maps/ -processfilelist /Users/scenes_to_dump -outdir /Users/resources/Data/ -links -gpu all");
}

DECL_TARC_MODULE(DumpTool);
