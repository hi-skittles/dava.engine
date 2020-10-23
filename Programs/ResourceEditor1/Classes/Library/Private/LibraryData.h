#pragma once

#include "TArc/DataProcessing/DataNode.h"
#include "FileSystem/FilePath.h"

class LibraryData : public DAVA::TArc::DataNode
{
public:
    static const char* selectedPathProperty;

    const DAVA::FilePath& GetSelectedPath() const;

private:
    friend class LibraryWidget;

    DAVA::FilePath selectedPath;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(LibraryData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<LibraryData>::Begin()
        .Field(selectedPathProperty, &LibraryData::GetSelectedPath, nullptr)
        .End();
    }
};
