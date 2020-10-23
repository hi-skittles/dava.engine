#pragma once

#include "Base/BaseTypes.h"
#include "CommandLine/ProgramOptions.h"
#include "CommandLineTool.h"

// the idea is to place CommandLineApplication and CommandLineTool into Sources/Tools in near future,
// combine them with CommandToolManager, CommandLineTool of ResourceEditor/Classes/CommandLine/
// and use that classes througout all our command line tools
class CommandLineApplication
{
public:
    CommandLineApplication(DAVA::String appName);
    void AddTool(std::unique_ptr<CommandLineTool> tool);

    int Process(const DAVA::Vector<DAVA::String>& cmdline);

private:
    void PrintUsage();

    const DAVA::String appName;
    DAVA::Vector<std::unique_ptr<CommandLineTool>> tools;
    DAVA::ProgramOptions helpOption;
};
