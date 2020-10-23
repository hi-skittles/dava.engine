#include "SceneUtils.h"

#include "FileSystem/FileSystem.h"
#include "Utils/StringFormat.h"
#include "Logger/Logger.h"

#include "Render/RHI/rhi_Public.h"
#include "Render/Renderer.h"
#include "Render/RenderHelper.h"

using namespace DAVA;

SceneUtils::SceneUtils()
{
}

SceneUtils::~SceneUtils()
{
}

void SceneUtils::CleanFolder(const FilePath& folderPathname)
{
    bool ret = FileSystem::Instance()->DeleteDirectory(folderPathname);
    if (!ret)
    {
        bool folderExists = FileSystem::Instance()->IsDirectory(folderPathname);
        if (folderExists)
        {
            Logger::Error("[CleanFolder] ret = %d, folder = %s", ret, folderPathname.GetAbsolutePathname().c_str());
        }
    }
}

void SceneUtils::SetInFolder(const FilePath& folderPathname)
{
    DVASSERT(folderPathname.IsDirectoryPathname());
    dataSourceFolder = folderPathname;
}

void SceneUtils::SetOutFolder(const FilePath& folderPathname)
{
    DVASSERT(folderPathname.IsDirectoryPathname());
    dataFolder = folderPathname;
}

bool SceneUtils::CopyFile(const FilePath& filePathname)
{
    String workingPathname = filePathname.GetRelativePathname(dataSourceFolder);
    PrepareFolderForCopyFile(workingPathname);

    bool retCopy = FileSystem::Instance()->CopyFile(dataSourceFolder + workingPathname, dataFolder + workingPathname);
    if (!retCopy)
    {
        Logger::Error("Can't copy %s from %s to %s",
                      workingPathname.c_str(),
                      dataSourceFolder.GetAbsolutePathname().c_str(),
                      dataFolder.GetAbsolutePathname().c_str());
    }

    return retCopy;
}

void SceneUtils::PrepareFolderForCopyFile(const String& filename)
{
    FilePath newFolderPath = (dataFolder + filename).GetDirectory();

    if (!FileSystem::Instance()->IsDirectory(newFolderPath))
    {
        FileSystem::eCreateDirectoryResult retCreate = FileSystem::Instance()->CreateDirectory(newFolderPath, true);
        if (FileSystem::DIRECTORY_CANT_CREATE == retCreate)
        {
            Logger::Error("Can't create folder %s", newFolderPath.GetAbsolutePathname().c_str());
        }
    }

    FileSystem::Instance()->DeleteFile(dataFolder + filename);
}

DAVA::FilePath SceneUtils::GetNewFilePath(const DAVA::FilePath& oldPathname) const
{
    String workingPathname = oldPathname.GetRelativePathname(dataSourceFolder);
    return dataFolder + workingPathname;
}

void SceneUtils::AddFile(const DAVA::FilePath& sourcePath)
{
    String workingPathname = sourcePath.GetRelativePathname(dataSourceFolder);
    FilePath destinationPath = dataFolder + workingPathname;

    if (sourcePath != destinationPath)
    {
        DVASSERT(!sourcePath.IsEmpty());
        DVASSERT(!destinationPath.IsEmpty());

        filesForCopy[sourcePath] = destinationPath;
    }
}

void SceneUtils::CopyFiles()
{
    PrepareDestination();

    auto endIt = filesForCopy.end();
    for (auto it = filesForCopy.begin(); it != endIt; ++it)
    {
        bool retCopy = false;

        if (FileSystem::Instance()->Exists(it->first))
        {
            FileSystem::Instance()->DeleteFile(it->second);
            retCopy = FileSystem::Instance()->CopyFile(it->first, it->second);
        }

        if (!retCopy)
        {
            Logger::Error("Can't copy %s to %s",
                          it->first.GetAbsolutePathname().c_str(),
                          it->second.GetAbsolutePathname().c_str());
        }
    }
}

void SceneUtils::PrepareDestination()
{
    DAVA::Set<DAVA::FilePath> folders;

    DAVA::Map<DAVA::FilePath, DAVA::FilePath>::const_iterator endMapIt = filesForCopy.end();
    for (DAVA::Map<DAVA::FilePath, DAVA::FilePath>::const_iterator it = filesForCopy.begin(); it != endMapIt; ++it)
    {
        folders.insert(it->second.GetDirectory());
    }

    DAVA::Set<DAVA::FilePath>::const_iterator endSetIt = folders.end();
    for (DAVA::Set<DAVA::FilePath>::const_iterator it = folders.begin(); it != endSetIt; ++it)
    {
        if (!FileSystem::Instance()->Exists(*it))
        {
            FileSystem::eCreateDirectoryResult retCreate = FileSystem::Instance()->CreateDirectory((*it), true);
            if (FileSystem::DIRECTORY_CANT_CREATE == retCreate)
            {
                Logger::Error("Can't create folder %s", (*it).GetAbsolutePathname().c_str());
            }
        }
    }
}
