#include "Classes/CommandLine/ConsoleHelpTool.h"
#include "Logger/Logger.h"

#include "TArc/Utils/ModuleCollection.h"

ConsoleHelpTool::ConsoleHelpTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-help")
{
}

DAVA::ConsoleModule::eFrameResult ConsoleHelpTool::OnFrameInternal()
{
    ShowHelpInternal();
    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

void ConsoleHelpTool::ShowHelpInternal()
{
    DAVA::Logger::Info("List of available commands: -sceneexporter, -scenesaver, -texdescriptor, -staticocclusion, -beast, -dump, -imagesplitter, -version, -sceneimagedump, -help");

    DAVA::Logger::Info("\t-sceneexporter - set of tools to prepare resources for game");
    DAVA::Logger::Info("\t-scenesaver - set of tools to save, resave or save scenes with references");
    DAVA::Logger::Info("\t-texdescriptor - set of tools to manipulate with textures and their descriptors");
    DAVA::Logger::Info("\t-staticocclusion - tool for building the static occlusion in console mode");
    DAVA::Logger::Info("\t-beast - tool for beasting scene in console mode");

    DAVA::Logger::Info("\t-dump - tool for saving scene references to file");
    DAVA::Logger::Info("\t-imagesplitter - set of tools to split or merge images");
    DAVA::Logger::Info("\t-version - show the version info of the current build of ResourceEditor");
    DAVA::Logger::Info("\t-sceneimagedump - tool for save screenshots from camera");
    DAVA::Logger::Info("\t-help - show this help");

    DAVA::Logger::Info("\nSee \'ResourceEditor <command> -h\' to read about a specific command.");

    CommandLineModule::ShowHelpInternal();
}

DECL_TARC_MODULE(ConsoleHelpTool);
