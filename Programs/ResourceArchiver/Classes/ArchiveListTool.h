#pragma once

#include "CommandLineTool.h"
#include <FileSystem/FilePath.h>

class ArchiveListTool final : public CommandLineTool
{
public:
    ArchiveListTool();

private:
    bool ConvertOptionsToParamsInternal() final;
    int ProcessInternal() final;

    DAVA::FilePath packFilePath;
};
