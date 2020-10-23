#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FilePath.h"
#include "Utils/Utils.h"

namespace DAVA
{
/**
	\brief Class used to enumerate files in directories
*/
class FileList : public BaseObject
{
protected:
    virtual ~FileList();

public:
    FileList(const FilePath& filepath, bool includeHidden = true);

    /**
		\brief Get total item count in current list
		This function return total number of items in directory including "." and ".." items
		\returns number of items in this directory,
	 */
    uint32 GetCount() const;

    /**
		\brief Get total item count in current list
		This function return number of files in directory
		\returns number of files in this directory
	 */
    uint32 GetFileCount() const;
    /**
		\brief Get total directory count in current list
		This function return number of files in directory
		\returns number of subdirectories in this directory
	 */
    uint32 GetDirectoryCount() const;

    //! Get path name
    const FilePath& GetPathname(uint32 index) const;

    //! Get file or folder name
    const String& GetFilename(uint32 index) const;

    uint32 GetFileSize(uint32 index) const;

    /**
		\brief is file with given index in this list is a directory
		\return true if this is directory
	 */
    bool IsDirectory(uint32 index) const;

    /*
		\brief is file with given index, is navigation directory.
		This funciton checks is directory == "." or directory == ".."
		\return true if this is ".", or ".." directory
	 */
    bool IsNavigationDirectory(uint32 index) const;

    bool IsHidden(uint32 index) const;

    void Sort();

private:
    struct FileEntry
    {
        FilePath path;
        String name;
        uint32 size;
        bool isDirectory;
        bool isHidden;

        bool operator<(const FileEntry& other) const
        {
            if (!isDirectory && other.isDirectory)
            {
                return false;
            }
            else if (isDirectory && !other.isDirectory)
            {
                return true;
            }

            return (CompareCaseInsensitive(name, other.name) < 0);
        }
    };
    FilePath path;
    Vector<FileEntry> fileList;
    int32 fileCount;
    int32 directoryCount;
};

}; // end of namespace DAVA
