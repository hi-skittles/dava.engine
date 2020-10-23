#include "CommandLine/CommandLineParser.h"
#include "Logger/Logger.h"

#include "Engine/Engine.h"

#include <stdlib.h>
#include <cerrno>

namespace DAVA
{
const int32 INVALID_POSITION = -1;

CommandLineParser::CommandLineParser()
    : isVerbose(false)
    , isExtendedOutput(false)
    , useTeamcityOutput(false)
{
}

void CommandLineParser::SetFlags(const Vector<String>& tokens)
{
    ClearFlags();

    for (auto& token : tokens)
    {
        if ((token.length() >= 1) && (token[0] == '-'))
        {
            flags.emplace_back(token);
        }
        else
        {
            if (!flags.empty())
            {
                flags.back().params.push_back(token);
            }
            else
            {
                Logger::Warning("argument '%s' must stay after any -flag token", token.c_str());
            }
        }
    }
}

void CommandLineParser::ClearFlags()
{
    flags.clear();
}

void CommandLineParser::SetVerbose(bool _isVerbose)
{
    isVerbose = _isVerbose;
}

bool CommandLineParser::GetVerbose() const
{
    return isVerbose;
}

void CommandLineParser::SetExtendedOutput(bool isExO)
{
    isExtendedOutput = isExO;
}

bool CommandLineParser::IsExtendedOutput() const
{
    return isExtendedOutput;
}

void CommandLineParser::SetUseTeamcityOutput(bool use)
{
    useTeamcityOutput = use;
}

bool CommandLineParser::UseTeamcityOutput() const
{
    return useTeamcityOutput;
}

CommandLineParser::~CommandLineParser()
{
}

bool CommandLineParser::IsFlagSet(const String& s) const
{
    for (auto& flag : flags)
    {
        if (flag.name == s)
            return true;
    }
    return false;
}

String CommandLineParser::GetParamForFlag(const String& flag)
{
    Vector<String> params = GetParamsForFlag(flag);
    if (!params.empty())
    {
        return params[0];
    }
    else
        return String();
}

Vector<String> CommandLineParser::GetParamsForFlag(const String& flagname)
{
    for (auto& flag : flags)
    {
        if (flag.name == flagname)
            return flag.params;
    }
    return Vector<String>();
}

bool CommandLineParser::CommandIsFound(const DAVA::String& command)
{
    return (INVALID_POSITION != GetCommandPosition(command));
}

DAVA::String CommandLineParser::GetCommand(DAVA::uint32 commandPosition)
{
    const Vector<String>& commandLine = Engine::Instance()->GetCommandLine();
    if (commandPosition < commandLine.size())
    {
        return commandLine[commandPosition];
    }

    return String();
}

DAVA::int32 CommandLineParser::GetCommandPosition(const DAVA::String& command)
{
    int32 position = INVALID_POSITION;

    const Vector<String>& commandLine = Engine::Instance()->GetCommandLine();
    for (size_t i = 0; i < commandLine.size(); ++i)
    {
        if (command == commandLine[i])
        {
            position = static_cast<int32>(i);
            break;
        }
    }

    return position;
}

DAVA::String CommandLineParser::GetCommandParam(const DAVA::String& command)
{
    auto pos = GetCommandPosition(command);
    if (INVALID_POSITION != pos)
    {
        return GetCommand(pos + 1);
    }
    else
    {
        return String();
    }
}

String CommandLineParser::GetCommandParamAdditional(const String& command, const int32 paramIndex) //TODO: remove this method after fix of DF-1584
{
    int32 commandPosition = GetCommandPosition(command);
    int32 firstParamPosition = commandPosition + 1;
    return GetCommand(firstParamPosition + paramIndex);
}

int32 CommandLineParser::GetCommandsCount()
{
    const Vector<String>& commandLine = Engine::Instance()->GetCommandLine();
    return static_cast<int32>(commandLine.size());
}
}
