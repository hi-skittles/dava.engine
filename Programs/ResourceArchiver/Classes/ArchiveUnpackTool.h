#pragma once

#include "CommandLineTool.h"
#include <FileSystem/FilePath.h>

class ArchiveUnpackTool final : public CommandLineTool
{
public:
    ArchiveUnpackTool();

private:
    bool ConvertOptionsToParamsInternal() final;
    int ProcessInternal() final;

    DAVA::FilePath dstDir;
    DAVA::FilePath packFilename;
    bool extractInDvplFormat = false;
};
