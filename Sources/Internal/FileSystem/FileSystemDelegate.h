#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
/**
	\ingroup filesystem
    FileSystemDelegate is a delegate to FileSystem that allows to control execution of FileSystem operations
 
    Delegate control next operations
    - FileSystem::IsFile()
    - FileSystem::IsDirectory()
    - File::Create()
*/

class FileSystemDelegate
{
public:
    virtual ~FileSystemDelegate() = default;

    /**
        Return true if delegate allows execution of FileSystem::IsFile() for specified 'absoluteFilePath'.
        Result of FileSystem::IsFile() will be false if delegate returns false.
    */
    virtual bool IsFileExists(const String& absoluteFilePath) const = 0;

    /**
        Return true if delegate allows execution of FileSystem::IsDirectory() for specified 'absoluteDirectoryPath'.
        Result of FileSystem::IsDirectory() will be false if delegate returns false.
    */
    virtual bool IsDirectoryExists(const String& absoluteDirectoryPath) const = 0;

    /**
        Return true if delegate allows execution of File::Create() for specified 'absoluteFilePath' and 'attributes'.
        Result of File::Create() will be nullptr if delegate returns false.
     */
    virtual bool CanCreateFile(const String& absoluteFilePath, uint32 attributes) const = 0;
};
}
