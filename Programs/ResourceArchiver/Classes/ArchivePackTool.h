#pragma once

#include "CommandLineTool.h"

#include <Compression/Compressor.h>

class ArchivePackTool : public CommandLineTool
{
public:
    ArchivePackTool();

private:
    enum class Source
    {
        UseListFiles,
        UseSrc,
        UseMetaDB,
        Unknown
    };

    bool ConvertOptionsToParamsInternal() override;
    int ProcessInternal() override;

    DAVA::String compressionStr;
    DAVA::Compressor::Type compressionType;
    bool dummyFileData = false;
    DAVA::String packFileName;
    DAVA::String baseDir;
    DAVA::String metaDbPath;
};
