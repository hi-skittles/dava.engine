#include "ArchivePackTool.h"

#include <FileSystem/FileSystem.h>
#include <ResourceArchiverModule/ResourceArchiver.h>
#include <Utils/StringUtils.h>
#include <Logger/Logger.h>
#include <Engine/Engine.h>
#include "ResultCodes.h"

namespace OptionNames
{
const DAVA::String Compression = "-compression";
const DAVA::String BaseDir = "-basedir";
const DAVA::String MetaDbFile = "-metadb";
const DAVA::String DummyFileData = "-dummyFileData";
}

ArchivePackTool::ArchivePackTool()
    : CommandLineTool("pack")
{
    using namespace DAVA;

    options.AddOption(OptionNames::Compression, VariantType(String("lz4hc")), "default compression method, lz4hc - default");
    options.AddOption(OptionNames::BaseDir, VariantType(String("")), "source base directory");
    options.AddOption(OptionNames::MetaDbFile, VariantType(String("")), "sqlite db with metadata");
    options.AddOption(OptionNames::DummyFileData, VariantType(false), "write dummy single-byte files instead of actual file data, useful if you are interested in pack footer only");
    options.AddArgument("packfile");
}

bool ArchivePackTool::ConvertOptionsToParamsInternal()
{
    using namespace DAVA;
    compressionStr = options.GetOption(OptionNames::Compression).AsString();

    int type;
    if (!GlobalEnumMap<Compressor::Type>::Instance()->ToValue(compressionStr.c_str(), type))
    {
        Logger::Error("Invalid compression type: '%s'", compressionStr.c_str());
        return false;
    }
    compressionType = static_cast<Compressor::Type>(type);

    dummyFileData = options.GetOption(OptionNames::DummyFileData).AsBool();

    baseDir = options.GetOption(OptionNames::BaseDir).AsString();
    if (baseDir.empty())
    {
        Logger::Error("%s - param is not specified", OptionNames::BaseDir.c_str());
        return false;
    }
    metaDbPath = options.GetOption(OptionNames::MetaDbFile).AsString();
    if (metaDbPath.empty())
    {
        Logger::Error("%s - param is not specified", OptionNames::MetaDbFile.c_str());
        return false;
    }

    packFileName = options.GetArgument("packfile");
    if (packFileName.empty())
    {
        Logger::Error("packfile param is not specified");
        return false;
    }

    return true;
}

int ArchivePackTool::ProcessInternal()
{
    using namespace DAVA;

    ResourceArchiver::Params params;
    params.compressionType = compressionType;
    params.archivePath = packFileName;
    params.baseDirPath = (baseDir.empty() ? FileSystem::Instance()->GetCurrentWorkingDirectory() : baseDir);
    params.metaDbPath = metaDbPath;
    params.dummyFileData = dummyFileData;

    if (!CreateArchive(params))
    {
        return ERROR_CANT_WRITE_FILE;
    }

    return OK;
}
