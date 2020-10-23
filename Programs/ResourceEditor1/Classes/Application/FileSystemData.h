#pragma once

#include <TArc/DataProcessing/DataNode.h>

#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectionRegistrator.h>

class FileSystemData : public DAVA::TArc::DataNode
{
public:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(FileSystemData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<FileSystemData>::Begin()
        .Field("tag", [](FileSystemData* data) { return DAVA::GetEngineContext()->fileSystem->GetFilenamesTag(); }, nullptr)
        .End();
    }
};
