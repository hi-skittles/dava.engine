#include "Classes/Utils/FileSystemUtils/FileSystemTagGuard.h"

#include <Engine/Engine.h>
#include <FileSystem/FileSystem.h>

FileSystemTagGuard::FileSystemTagGuard(const DAVA::String newFilenamesTag)
{
    DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
    oldFilenamesTag = fs->GetFilenamesTag();
    fs->SetFilenamesTag(newFilenamesTag);
}

FileSystemTagGuard::~FileSystemTagGuard()
{
    DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
    fs->SetFilenamesTag(oldFilenamesTag);
}
