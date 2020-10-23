#include "CommandLine/ConsoleHelpTool.h"
#include "Logger/Logger.h"

#include "TArc/Utils/ModuleCollection.h"

ConsoleHelpTool::ConsoleHelpTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-help")
{
}

DAVA::TArc::ConsoleModule::eFrameResult ConsoleHelpTool::OnFrameInternal()
{
    ShowHelpInternal();
    return DAVA::TArc::ConsoleModule::eFrameResult::FINISHED;
}

void ConsoleHelpTool::ShowHelpInternal()
{
#if defined(__DAVAENGINE_BEAST__)
    DAVA::Logger::Info("List of available commands: -sceneexporter, -scenesaver, -texdescriptor, -staticocclusion, -beast, -dump, -imagesplitter, -version, -sceneimagedump, -help");
#else
    DAVA::Logger::Info("List of available commands: -sceneexporter, -scenesaver, -texdescriptor, -staticocclusion, -dump, -imagesplitter, -version, -sceneimagedump, -help");
#endif //__DAVAENGINE_BEAST__

    DAVA::Logger::Info("\t-sceneexporter - set of tools to prepare resources for game");
    DAVA::Logger::Info("\t-scenesaver - set of tools to save, resave or save scenes with references");
    DAVA::Logger::Info("\t-texdescriptor - set of tools to manipulate with textures and their descriptors");
    DAVA::Logger::Info("\t-staticocclusion - tool for building the static occlusion in console mode");
    
#if defined(__DAVAENGINE_BEAST__)
    DAVA::Logger::Info("\t-beast - tool for beasting scene in console mode");
#endif //__DAVAENGINE_BEAST__

    DAVA::Logger::Info("\t-dump - tool for saving scene references to file");
    DAVA::Logger::Info("\t-imagesplitter - set of tools to split or merge images");
    DAVA::Logger::Info("\t-version - show the version info of the current build of ResourceEditor");
    DAVA::Logger::Info("\t-sceneimagedump - tool for save screenshots from camera");
    DAVA::Logger::Info("\t-help - show this help");

    DAVA::Logger::Info("\nSee \'ResourceEditor <command> -h\' to read about a specific command.");

    CommandLineModule::ShowHelpInternal();
}

DECL_CONSOLE_MODULE(ConsoleHelpTool, "-help");
