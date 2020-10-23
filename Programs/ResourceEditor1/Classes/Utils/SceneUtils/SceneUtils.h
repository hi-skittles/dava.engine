#ifndef __SCENE_UTILS_H__
#define __SCENE_UTILS_H__

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"

class SceneUtils
{
public:
    SceneUtils();
    ~SceneUtils();

    void CleanFolder(const DAVA::FilePath& folderPathname);

    void SetInFolder(const DAVA::FilePath& folderPathname);
    void SetOutFolder(const DAVA::FilePath& folderPathname);

    bool CopyFile(const DAVA::FilePath& filePathname);
    void PrepareFolderForCopyFile(const DAVA::String& filename);

    DAVA::FilePath GetNewFilePath(const DAVA::FilePath& oldPathname) const;

    void AddFile(const DAVA::FilePath& sourcePath);
    void CopyFiles();

private:
    void PrepareDestination();

public:
    DAVA::FilePath dataFolder;
    DAVA::FilePath dataSourceFolder;
    DAVA::String workingFolder;

    DAVA::Map<DAVA::FilePath, DAVA::FilePath> filesForCopy;
};

namespace RenderObjectsFlusher
{
DAVA_DEPRECATED(void Flush());
}

#endif // __SCENE_UTILS_H__