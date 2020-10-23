#include "Classes/CommandLine/SceneSaverTool.h"

#include <REPlatform/CommandLine/OptionName.h>
#include <REPlatform/CommandLine/SceneConsoleHelper.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Scene/Utils/SceneSaver.h>

#include <TArc/Utils/ModuleCollection.h>

#include <Logger/Logger.h>

SceneSaverTool::SceneSaverTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-scenesaver")
{
    using namespace DAVA;

    options.AddOption(OptionName::Save, VariantType(false), "Saving scene from indir to outdir");
    options.AddOption(OptionName::Resave, VariantType(false), "Resave file into indir");
    options.AddOption(OptionName::Yaml, VariantType(false), "Target is *.yaml file");
    options.AddOption(OptionName::InDir, VariantType(String("")), "Path for Project/DataSource/3d/ folder");
    options.AddOption(OptionName::OutDir, VariantType(String("")), "Path for Project/Data/3d/ folder");
    options.AddOption(OptionName::ProcessFile, VariantType(String("")), "Filename from DataSource/3d/ for exporting");
    options.AddOption(OptionName::CopyConverted, VariantType(false), "Enables copying of converted image files");
    options.AddOption(OptionName::QualityConfig, VariantType(String("")), "Full path for quality.yaml file");
    options.AddOption(OptionName::TagList, VariantType(String("")), "Tags for textures and slots, Can be multiple: -taglist .china,.japan. Suitable only for saving of scene", true);
}

bool SceneSaverTool::PostInitInternal()
{
    using namespace DAVA;

    inFolder = options.GetOption(OptionName::InDir).AsString();
    if (inFolder.IsEmpty())
    {
        Logger::Error("Input folder was not selected");
        return false;
    }
    inFolder.MakeDirectoryPathname();

    dataSourceFolder = ProjectManagerData::GetDataSourcePath(inFolder);
    if (dataSourceFolder.IsEmpty())
    {
        DAVA::Logger::Error("DataSource folder was not found");
        return false;
    }

    outFolder = options.GetOption(OptionName::OutDir).AsString();
    filename = options.GetOption(OptionName::ProcessFile).AsString();
    copyConverted = options.GetOption(OptionName::CopyConverted).AsBool();

    if (options.GetOption(OptionName::Save).AsBool())
    {
        commandAction = ACTION_SAVE;
    }
    else if (options.GetOption(OptionName::Resave).AsBool())
    {
        if (options.GetOption(OptionName::Yaml).AsBool())
        {
            commandAction = ACTION_RESAVE_YAML;
        }
        else
        {
            commandAction = ACTION_RESAVE_SCENE;
        }
    }

    if (commandAction == ACTION_SAVE)
    {
        if (outFolder.IsEmpty())
        {
            Logger::Error("Output folder was not selected");
            return false;
        }
        outFolder.MakeDirectoryPathname();

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
    }
    else if (commandAction == ACTION_NONE)
    {
        Logger::Error("Wrong action was selected");
        return false;
    }

    if (filename.empty() && (commandAction != eAction::ACTION_RESAVE_YAML))
    {
        Logger::Error("Filename was not selected");
        return false;
    }

    if (commandAction != ACTION_RESAVE_YAML)
    {
        bool qualityInitialized = SceneConsoleHelper::InitializeQualitySystem(options, inFolder);
        if (!qualityInitialized)
        {
            DAVA::Logger::Error("Cannot create path to quality.yaml from %s", inFolder.GetAbsolutePathname().c_str());
            return false;
        }
    }

    return true;
}

DAVA::ConsoleModule::eFrameResult SceneSaverTool::OnFrameInternal()
{
    DAVA::FilePath::AddResourcesFolder(dataSourceFolder);
    switch (commandAction)
    {
    case SceneSaverTool::eAction::ACTION_SAVE:
    {
        DAVA::SceneSaver saver;
        saver.SetInFolder(inFolder);
        saver.SetOutFolder(outFolder);
        saver.EnableCopyConverted(copyConverted);
        saver.SetTags(tags);
        saver.SaveFile(filename);

        break;
    }
    case SceneSaverTool::eAction::ACTION_RESAVE_SCENE:
    {
        DAVA::SceneSaver saver;
        saver.SetInFolder(inFolder);
        saver.ResaveFile(filename);
        break;
    }
    case SceneSaverTool::eAction::ACTION_RESAVE_YAML:
    {
        DAVA::SceneSaver saver;
        saver.ResaveYamlFilesRecursive(inFolder);
        break;
    }

    default:
        DAVA::Logger::Error("Unhandled action!");
        break;
    }
    DAVA::FilePath::RemoveResourcesFolder(dataSourceFolder);

    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

void SceneSaverTool::BeforeDestroyedInternal()
{
    DAVA::SceneConsoleHelper::FlushRHI();
}

void SceneSaverTool::ShowHelpInternal()
{
    CommandLineModule::ShowHelpInternal();

    DAVA::Logger::Info("Examples:");
    DAVA::Logger::Info("\t-scenesaver -save -indir /Users/SmokeTest/DataSource/3d/ -outdir /Users/NewProject/Data/3d/ -processfile Maps/scene.sc2 -qualitycfgpath Users/SmokeTest/Data/quality.yaml");
    DAVA::Logger::Info("\t-scenesaver -save -indir /Users/SmokeTest/DataSource/3d/ -outdir /Users/NewProject/Data/3d/ -processfile Maps/scene.sc2 -qualitycfgpath Users/SmokeTest/Data/quality.yaml -copyconverted");
    DAVA::Logger::Info("\t-scenesaver -save -indir /Users/SmokeTest/DataSource/3d/ -outdir /Users/NewProject/Data/3d/ -processfile Maps/scene.sc2 -qualitycfgpath Users/SmokeTest/Data/quality.yaml -taglist .china,.japan");

    DAVA::Logger::Info("\t-scenesaver -resave -indir /Users/SmokeTest/DataSource/3d/ -processfile Maps/scene.sc2 -qualitycfgpath Users/SmokeTest/Data/quality.yaml");
    DAVA::Logger::Info("\t-scenesaver -resave -yaml -indir /Users/SmokeTest/Data/Configs/");
}

DECL_TARC_MODULE(SceneSaverTool);
