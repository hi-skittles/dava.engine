#pragma once

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "FileSystem/File.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/ResourceArchive.h"
#include "Concurrency/Mutex.h"

/**
	\defgroup filesystem File System
 */
namespace DAVA
{
/**
	\ingroup filesystem
	\brief FileSystem is a wrapper class that allow to perform all basic filesystem operations

	Class is platform dependent but it must used in all places where you want to be sure that portability is an issue

	Supported platforms:
		Windows, MacOS X, iPhone OS

	\todo add functions to enumerate files in directories to be full functional FileSystem
	\todo refactoring of utils and ~res:/ ~doc:/ access for the project files
	\todo add support for pack files
*/
class FileSystemDelegate;
class FileSystem : public Singleton<FileSystem>
{
public:
    FileSystem();
    virtual ~FileSystem();

    /**
		\brief Function to delete file from filesystem
		\param[in] filePath full path for the file we want to delete
		\returns true if deletion was successful
	 */
    virtual bool DeleteFile(const FilePath& filePath);

    /*
		\brief Function to delete directory

		If isRecursive variable is false, function will succeed only in case if directory is empty.

		\param[in] path full path to the directory you want to delete
		\param[in] isRecursive if true trying to delete all subfolders, if not just trying to delete this directory
		\returns true if this directory was deleted
	 */
    virtual bool DeleteDirectory(const FilePath& path, bool isRecursive = true);

    /*
		\brief Deletes all files in given directory
		if isRecursive is set function walks into all child directories and delete files there also.
		This funciton do not delete directoris, it delete only files
		\param[in] isRecursive if true go into child directories and delete files there also, false by default
		\returns number of deleted files
	*/
    virtual uint32 DeleteDirectoryFiles(const FilePath& path, bool isRecursive = false);

    /*
        \brief Enumerate all files in specific directory
        \param[in] path full path to the directory you want to enumerate
        \param[in] isRecursive if true go into child directories and enumarate files there also, true by default
        \returns list of files
    */
    virtual Vector<FilePath> EnumerateFilesInDirectory(const FilePath& path, bool isRecursive = true);

    enum eCreateDirectoryResult
    {
        DIRECTORY_CANT_CREATE = 0,
        DIRECTORY_EXISTS = 1,
        DIRECTORY_CREATED = 2,
    };
    /**
		\brief Function to create directory at filePath you've requested
		\param[in] filePath where you want to create a directory
        \param[in] isRecursive create requiried
		\returns true if directory created successfully
	 */
    virtual eCreateDirectoryResult CreateDirectory(const FilePath& filePath, bool isRecursive = false);

    /**
		\brief Function to retrieve current working directory
		\returns current working directory
	 */
    virtual FilePath GetCurrentWorkingDirectory();

    /**
		\brief Function to retrieve directory, which contain executable binary file
		\returns current directory, with  executable file
	 */
    virtual FilePath GetCurrentExecutableDirectory();

    /**
     \brief Function to retrieve directory, which contain plugins files
     \returns plugin directory
     */
    virtual FilePath GetPluginDirectory();

    /**
		\brief Function to set current working directory
		\param[in] newWorkingDirectory new working directory to be set
		\returns true if directory set successfully
	 */
    virtual bool SetCurrentWorkingDirectory(const FilePath& newWorkingDirectory);

    /**
        \brief Function to retrieve current documents directory
        \returns current documents directory
     */
    virtual const FilePath& GetCurrentDocumentsDirectory();

    /**
         \brief Function to set current documents directory
         \param[in] newDocDirectory new documents directory to be set
     */
    virtual void SetCurrentDocumentsDirectory(const FilePath& newDocDirectory);

    /**
         \brief Function to set current documents directory to default
     */
    virtual void SetDefaultDocumentsDirectory();

    /**
         \brief Function to retrieve user's documents path
         \returns user's documents path
     */
    static const FilePath GetUserDocumentsPath();

    /**
         \brief Function to retrieve public documents path
         \returns public documents path
     */
    virtual const FilePath GetPublicDocumentsPath();

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
    /**
        \brief Function to retrieve user’s home path
        \returns user’s home path
    */
    const FilePath GetHomePath();
#endif

    /**
		\brief Function check if specified path is a regular file
	*/
    bool IsFile(const FilePath& pathToCheck) const;

    /**
		\brief Function check if specified path is a directory
	 */
    bool IsDirectory(const FilePath& pathToCheck) const;

    /**
        \brief Function checks if specifed path or dir is hidden
    */
    bool IsHidden(const FilePath& pathToCheck) const;

    /**
     \brief Function sets/removes exclusive lock to/from file.
     \param[in] filePath The name of the file to be locked/unlocked.
     \param[in] isLock true to lock file, false to unlock.
     \returns true if file was successfully locked/unlocked, false otherwise
	 */
    virtual bool LockFile(const FilePath& filePath, bool isLock);

    /**
     \brief Function checks whether the file is locked.
     \param[in] filePath The name of the file to be checked for lock.
     \returns true if file is locked, false if not locked
	 */
    virtual bool IsFileLocked(const FilePath& filePath) const;

    /**
		\brief Copies an existing file to a new file.
		\param[in] existingFile The name of an existing file.
		\param[out] newFile The name of the new file.
		\returns true if file was successfully copied, false otherwise
	*/
    virtual bool CopyFile(const FilePath& existingFile, const FilePath& newFile, bool overwriteExisting = false);

    /**
		\brief Moves an existing file to a new file.
		\param[in] existingFile The name of an existing file.
		\param[out] newFile The name of the new file.
		\param[in] overwriteExisting signal to overwrite existing file with name newFile.
		\returns true if file was successfully moved, false otherwise
	*/
    virtual bool MoveFile(const FilePath& existingFile, const FilePath& newFile, bool overwriteExisting = false);

    /**
		\brief Copies directory to another existing directory.
		\param[in] sourceDirectory The name of an existing file.
		\param[out] destinationDirectory The name of the new file.
		\returns true if all files were successfully copied, false otherwise.
	*/
    virtual bool CopyDirectoryFiles(const FilePath& sourceDirectory, const FilePath& destinationDirectory, bool overwriteExisting = false);

    /**
        \brief Read whole file contents into new buffer.
        If function returns zero error happened and it haven't loaded the file
        After you'll finish using the date you should DELETE returned buffer using function SafeDeleteArray.

        \param[in] pathname path to the file we want to read
        \param[out] fileSize
        \returns pointer to newly created buffer with file contents
     */
    uint8* ReadFileContents(const FilePath& pathname, uint64& fileSize);

    /**
        \brief Read whole file contents into string.
        \param[in] pathname path to the file we want to read
        \returns string with whole file contents
     */
    String ReadFileContents(const FilePath& pathname);

    bool ReadFileContents(const FilePath& pathname, Vector<uint8>& buffer);

    /**
		\brief Function to attach ResourceArchive to filesystem

		\param[in] archiveName pathname or local filename of archive we want to attach
		\param[in] attachPath path we attach our archive

        can throw std::runtime_exception in case of error
        thread safe
	*/
    virtual void Mount(const FilePath& archiveName, const String& attachPath);

    /**
        \brief Function to detach ResourceArchive from filesystem

        \param[in] archiveName pathname or local filename of archive we want to attach
        thread safe
    */
    virtual void Unmount(const FilePath& arhiveName);

    /**
        \brief Function to check if ResourceArchive is mounted

        \param[in] archiveName filename of archive we want to attach
        thread safe
    */
    virtual bool IsMounted(const FilePath& archiveName) const;

    /**
	 \brief Invokes the command processor to execute a command
	 \param[in] command contains the system command to be executed
	 \returns platform-dependent
	 */
    int32 Spawn(const String& command);

    /**
	 \brief Marks folder as contains no media files to exclude it from index
	 */
    void MarkFolderAsNoMedia(const FilePath& folder);

    /**
    \brief Compares two files to check if theirs content is same. Ignores lineendings
    \param[in] filePath1 - path to one of files to compare
    \param[in] filePath2 - path to one of files to compare
    \param[in] ignoreEmptyLines - ignores any empty lines
    \returns true if files are equals and false if not
    */
    bool CompareTextFiles(const FilePath& filePath1, const FilePath& filePath2);

    /**
    \brief Compares two files to check if theirs content is same.
    \param[in] filePath1 - path to one of files to compare
    \param[in] filePath2 - path to one of files to compare
    \returns true if files are equals and false if not
    */
    bool CompareBinaryFiles(const FilePath& filePath1, const FilePath& filePath2);

    DAVA_DEPRECATED(bool GetFileSize(const FilePath&, uint32&));

    bool GetFileSize(const FilePath& path, uint64& size);

    /**
     \brief Function check if specified path exists on file system (on Android also check in assets dir)
     */
    bool Exists(const FilePath& filePath) const;

    /**
    * \brief Search for file in android APK in assets folder
    */
    bool ExistsInAndroidAssets(const FilePath& path) const;

    /**
    \brief Copies one folder into another recursively
    */
    bool RecursiveCopy(const FilePath& src, const FilePath& dst);

    File* CreateFileForFrameworkPath(const FilePath& frameworkPath, uint32 attributes);

    FilePath GetTempDirectoryPath() const;

    void SetFilenamesTag(const String& newTag);
    const String& GetFilenamesTag() const;

    void SetDelegate(FileSystemDelegate* delegate);
    FileSystemDelegate* GetDelegate() const;

private:
    bool HasLineEnding(File* f);

    virtual eCreateDirectoryResult CreateExactDirectory(const FilePath& filePath);

    FilePath currentDocDirectory; // TODO how it influence on multithreading with FS?

    struct ResourceArchiveItem
    {
        ResourceArchiveItem() = default;
        ResourceArchiveItem(const ResourceArchiveItem&) = delete;
        ResourceArchiveItem(ResourceArchiveItem&& other) noexcept
        : archive(std::move(other.archive))
          ,
          attachPath(std::move(other.attachPath))
          ,
          archiveFilePath(std::move(other.archiveFilePath))
        {
        }

        std::unique_ptr<ResourceArchive> archive;
        String attachPath;
        FilePath archiveFilePath;
    };

    mutable Mutex accessArchiveMap;
    UnorderedMap<String, ResourceArchiveItem> resArchiveMap;
    Map<String, void*> lockedFileHandles;

    String filenamesTag;

    FileSystemDelegate* fsDelegate = nullptr;

    friend class File;
    friend class FilePath;
    Vector<FilePath> resourceFolders;
};
}
