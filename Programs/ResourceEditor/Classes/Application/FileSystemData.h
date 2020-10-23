#pragma once

#include <TArc/DataProcessing/TArcDataNode.h>

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectionRegistrator.h>

class FileSystemData : public DAVA::TArcDataNode
{
public:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(FileSystemData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<FileSystemData>::Begin()
        .Field("tag", [](FileSystemData* data) { return DAVA::GetEngineContext()->fileSystem->GetFilenamesTag(); }, nullptr)
        .End();
    }
};
