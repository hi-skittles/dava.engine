#include "Classes/CommandLine/VersionTool.h"
#include "Logger/Logger.h"

#include <Version/Version.h>
#include "TArc/Utils/ModuleCollection.h"

#include <QtGlobal>

VersionTool::VersionTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-version")
{
}

DAVA::ConsoleModule::eFrameResult VersionTool::OnFrameInternal()
{
    DAVA::String title = DAVA::Version::CreateAppVersion("Resource Editor");
    DAVA::Logger::Info("========================================");
    DAVA::Logger::Info("Qt: %s", QT_VERSION_STR);
    DAVA::Logger::Info("Title: %s", title.c_str());
    DAVA::Logger::Info("%u bit", static_cast<DAVA::uint32>(sizeof(DAVA::pointer_size) * 8));
    DAVA::Logger::Info("========================================");

    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

DECL_TARC_MODULE(VersionTool);
