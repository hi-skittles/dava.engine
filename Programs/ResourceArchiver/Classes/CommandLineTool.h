#pragma once

#include "Base/BaseTypes.h"
#include "CommandLine/ProgramOptions.h"

class CommandLineTool
{
public:
    CommandLineTool(const DAVA::String& toolName);
    virtual ~CommandLineTool()
    {
    }

    void SetParseErrorCode(int errorCode);
    void SetOkCode(int errorCode);

    DAVA::String GetToolKey() const;
    bool ParseOptions(const DAVA::Vector<DAVA::String>& cmdline);
    bool ParseOptions(DAVA::uint32 argc, char* argv[]);
    void PrintUsage() const;
    DAVA::String GetUsageString() const;
    int Process();

protected:
    virtual bool ConvertOptionsToParamsInternal() = 0;
    virtual int ProcessInternal() = 0;

private:
    void PrepareEnvironment() const;

protected:
    int codeParseError = -1;
    int codeOk = 0;
    DAVA::ProgramOptions options;
};
