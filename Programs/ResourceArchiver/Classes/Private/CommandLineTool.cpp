#include "CommandLineTool.h"
#include "Engine/Engine.h"
#include "Logger/Logger.h"
#include "Logger/TeamcityOutput.h"
#include "CommandLine/CommandLineParser.h"

using namespace DAVA;

CommandLineTool::CommandLineTool(const DAVA::String& toolName)
    : options(toolName)
{
    options.AddOption("-v", VariantType(false), "Verbose output");
    options.AddOption("-h", VariantType(false), "Help for command");
    options.AddOption("-teamcity", VariantType(false), "Extra output in teamcity format");
}

void CommandLineTool::SetParseErrorCode(int code)
{
    codeParseError = code;
}

void CommandLineTool::SetOkCode(int code)
{
    codeOk = code;
}

bool CommandLineTool::ParseOptions(const DAVA::Vector<DAVA::String>& cmdline)
{
    return options.Parse(cmdline);
}

bool CommandLineTool::ParseOptions(uint32 argc, char* argv[])
{
    return options.Parse(argc, argv);
}

void CommandLineTool::PrintUsage() const
{
    Logger::Warning("%s", options.GetUsageString().c_str());
}

String CommandLineTool::GetUsageString() const
{
    return options.GetUsageString();
}

DAVA::String CommandLineTool::GetToolKey() const
{
    return options.GetCommand();
}

int CommandLineTool::Process()
{
    const bool printUsage = options.GetOption("-h").AsBool();
    if (printUsage)
    {
        PrintUsage();
        return codeOk;
    }

    PrepareEnvironment();

    if (ConvertOptionsToParamsInternal())
    {
        return ProcessInternal();
    }

    PrintUsage();
    return codeParseError;
}

void CommandLineTool::PrepareEnvironment() const
{
    const bool verboseMode = options.GetOption("-v").AsBool();
    if (verboseMode)
    {
        CommandLineParser::Instance()->SetVerbose(true);
        GetEngineContext()->logger->SetLogLevel(Logger::LEVEL_DEBUG);
    }

    const bool useTeamcity = options.GetOption("-teamcity").AsBool();
    if (useTeamcity)
    {
        CommandLineParser::Instance()->SetUseTeamcityOutput(true);
        Logger::AddCustomOutput(new TeamcityOutput());
    }
}
