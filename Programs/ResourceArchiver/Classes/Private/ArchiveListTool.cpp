#include "ArchiveListTool.h"

#include <Base/EnumMap.h>
#include <Base/GlobalEnum.h>
#include <Logger/Logger.h>
#include <Utils/StringFormat.h>
#include <FileSystem/ResourceArchive.h>

#include "ResultCodes.h"

ArchiveListTool::ArchiveListTool()
    : CommandLineTool("list")
{
    options.AddArgument("packfile");
}

bool ArchiveListTool::ConvertOptionsToParamsInternal()
{
    packFilePath = options.GetArgument("packfile");
    if (packFilePath.IsEmpty())
    {
        DAVA::Logger::Error("packfile param is not specified");
        return false;
    }

    return true;
}

int ArchiveListTool::ProcessInternal()
{
    using namespace DAVA;

    std::unique_ptr<ResourceArchive> archive;

    try
    {
        archive = std::make_unique<ResourceArchive>(packFilePath);
    }
    catch (std::exception ex)
    {
        Logger::Error("Can't open archive %s: %s", packFilePath.GetAbsolutePathname().c_str(), ex.what());
        return ERROR_CANT_OPEN_ARCHIVE;
    }

    String out = Format("Dumping contents of archive %s", packFilePath.GetFilename().c_str());
    Logger::Info("%s", out.c_str());

    const EnumMap* const map = GlobalEnumMap<Compressor::Type>::Instance();
    DVASSERT(map != nullptr);

    for (const ResourceArchive::FileInfo& info : archive->GetFilesInfo())
    {
        String compressionStr = map->ToString(static_cast<int>(info.compressionType));
        out = Format("%s: orig size %u, compressed size %u, type %s",
                     info.relativeFilePath.c_str(), info.originalSize, info.compressedSize, compressionStr.c_str());

        Logger::Info("%s", out.c_str());
    }

    return OK;
}
