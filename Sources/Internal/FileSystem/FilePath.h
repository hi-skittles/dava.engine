#pragma once

#include "Base/BaseTypes.h"
#include "Base/Any.h"

namespace DAVA
{
static const char8* localResourcesPath = "/mnt/sdcard/DavaProject/";

/**
    \ingroup filesystem
    \brief class to work with file pathname
    */
class FilePath
{
public:
    enum ePathType : int32
    {
        PATH_EMPTY = -1, // empty path, newly created with empty string
        PATH_IN_FILESYSTEM = 0, // not framework path /Users/... or c:/...
        PATH_IN_RESOURCES, // ~res:/...
        PATH_IN_DOCUMENTS, // ~doc:/...
        PATH_IN_MEMORY // FBO, TEXT, memory file
    };

    FilePath();
    FilePath(const FilePath& path);
    FilePath(FilePath&& path);

    FilePath(const String& sourcePath);
    FilePath(const WideString& sourcePath);

    FilePath(const char8* sourcePath);
    FilePath(const char16* sourcePath);

    FilePath(const FilePath& directory, const String& filename);
    FilePath(const FilePath& directory, const WideString& filename);

    FilePath(const String& directory, const String& filename);
    FilePath(const WideString& directory, const WideString& filename);

    FilePath(const char8* directory, const String& filename);
    FilePath(const char16* directory, const WideString& filename);

    ~FilePath();

    /**
        \brief Function to retrieve FilePath with new extension without changing of source FilePath object
        \param[in] pathname is source FilePath object
        \param[in] extension is new extension
        \returns resolved FilePath object with new extension
        */
    static FilePath CreateWithNewExtension(const FilePath& pathname, const String& extension);

    FilePath& operator=(const FilePath& path);
    FilePath& operator=(FilePath&& path);
    FilePath operator+(const String& path) const;
    FilePath& operator+=(const String& path);

    bool operator==(const FilePath& path) const;
    bool operator!=(const FilePath& path) const;

    /*
        \brief Function to check is filepath empty or no
        \returns true if absolutePathname is not empty
        */
    inline bool IsEmpty() const;

    /*
        \brief Function to check is filepath represent folder path
        \returns true if absolutePathname has '/' as last character
        */
    bool IsDirectoryPathname() const;

    /**
        \brief Function to retrieve pathname
        \returns pathname value
        */
    String GetAbsolutePathname() const;

#if defined(__DAVAENGINE_WINDOWS__)
    using NativeStringType = WideString;
#else
    using NativeStringType = String;
#endif
    /**
        \brief Function to retrieve pathname
        \returns pathname value in native string type
        */
    DAVA_DEPRECATED(NativeStringType GetNativeAbsolutePathname() const);

    /**
        \brief Function to create an object from native string
        \returns FilePath object
		deprecated - always use String with utf8 on all platforms
        */
    DAVA_DEPRECATED(static FilePath FromNativeString(const NativeStringType& path));

    /**
        \brief Function to retrieve filename from pathname. Filename for path "/Users/Folder/image.png" is "image.png".
        \returns filename value
        */
    DAVA_DEPRECATED(String GetFilename() const);

    /**
        \brief Function to retrieve basename from pathname without extension. 
        Basename for path "/Users/Folder/image.png" is "image".
        Basename for path "/Users/Folder/" is "Folder".
        \returns basename value
        */
    String GetBasename() const;

    /**
        \brief Function to retrieve name from pathname with extension.
        Name for path "/Users/Folder/image.png" is "image.png".
        Name for path "/Users/Folder/" is "Folder".
        \returns name value
    */
    String GetName() const;

    /**
        \brief Function to retrieve extension from pathname. Extension for path "/Users/Folder/image.png" is ".png".
        \returns extension value
        */
    String GetExtension() const;

    /**
        \brief Function to retrieve directory from pathname. Directory for path "/Users/Folder/image.png" is "/Users/Folder/".
        \returns directory value
        */
    FilePath GetDirectory() const;

    /**
        \brief Function to retrieve relative pathname for current working directory
        \returns relative path value
        */
    String GetRelativePathname() const;

    /**
        \brief Function to retrieve relative pathname for exact directory
        \param[in] forDirectory is exact directory for relative path calculation
        \returns relative path value
        */
    String GetRelativePathname(const FilePath& forDirectory) const;
    String GetRelativePathname(const String& forDirectory) const;
    String GetRelativePathname(const char8* forDirectory) const;

    /**
        \brief Function to retrieve string path value, passed in constructor
        \returns relative string path value
        */
    const String& GetStringValue() const;

    /**
        \brief Function to retrieve string path value as URL for Web Browser
        \returns path as URL
        */
    String AsURL() const;

    /**
        \brief Function for replacement of original filename
        \param[in] filename is new filename
        */
    void ReplaceFilename(const String& filename);

    /**
        \brief Function for replacement of original basename
        \param[in] basename is new basename
        */
    void ReplaceBasename(const String& basename);

    /**
        \brief Function for replacement of original extension
        \param[in] extension is new extension
        */
    void ReplaceExtension(const String& extension);

    /**
        \brief Function for replacement of original directory
        \param[in] directory is new directory
        */
    void ReplaceDirectory(const String& directory);
    void ReplaceDirectory(const FilePath& directory);

    /**
        \brief Function to modify absolute to be path to folder. For example "Users/Document" after function call will be "Users/Document/"
        */
    FilePath& MakeDirectoryPathname();

    /**
        \brief Function to truncate extension from path
        */
    void TruncateExtension();

    /**
        \brief Function to retrieve last directory name from FilePath that represents directory pathname
        \returns last directory name
        */
    String GetLastDirectoryName() const;

    /**
        \brief Function for comparison with extension of filepath object
        \param[in] extension is extension for comparison
        */
    bool IsEqualToExtension(const String& extension) const;

    /**
        \brief Function to retrieve path in framework style: with ~res: or ~doc:
        \returns pathname value for requested type
        */
    String GetFrameworkPath() const;

    /**
        \brief Function to set system path bundle path to project path for resolving pathnames such as "~res:/Gfx/image.png"
        */
    static void InitializeBundleName();

/**
        \brief Temporary function to set system path for NPAPI plugins for resolving pathnames such as "~res:/Gfx/image.png"
        */
#if defined(__DAVAENGINE_NPAPI__)
    static void InitializeBundleNameNPAPI(const String& pathToNPAPIPlugin);
#endif // #if defined (__DAVAENGINE_NPAPI__)

    /**
        \brief Function to set project path for resolving pathnames such as "~res:/Gfx/image.png"
        \param[in] newBundlePath - new project path
        */
    static void SetBundleName(const FilePath& newBundlePath);

    /**
        \brief Function to retrieve project path for resolving pathnames such as "~res:/Gfx/image.png"
        \returns project path
        */
    static const FilePath& GetBundleName();

    /**
        \brief Function to retrieve full path relative current documents folder
        \returns path relative corrent documents folder
        */
    static FilePath FilepathInDocuments(const char8* relativePathname);

    /**
        \brief Function to retrieve full path relative current documents folder
        \returns path relative corrent documents folder
        */
    static FilePath FilepathInDocuments(const String& relativePathname);

    /**
        \brief Function to retrieve type of path
        \returns type of path
        */
    inline ePathType GetType() const;

    bool StartsWith(const FilePath& basePath) const;

    static bool ContainPath(const FilePath& basePath, const FilePath& partPath);
    static bool ContainPath(const FilePath& basePath, const String& partPath);
    static bool ContainPath(const FilePath& basePath, const char8* partPath);

    static void AddResourcesFolder(const FilePath& folder);
    static void AddTopResourcesFolder(const FilePath& folder);
    static void RemoveResourcesFolder(const FilePath& folder);
    static const Vector<FilePath>& GetResFolders();
    DAVA_DEPRECATED(static const List<FilePath>& GetResourcesFolders());

    DAVA_DEPRECATED(bool Exists() const);

    int32 Compare(const FilePath& right) const;

    static bool IsAbsolutePathname(const String& pathname);

    /** Return normalized concat of specified 'path' and 'addition', i.e. AddPath("abc", "def") => "abcdef" */
    static String AddPath(const FilePath& path, const String& addition);

private:
    void Initialize(const String& pathname);
    void Initialize(const WideString& pathname);
    void InitializeWithDirectoryAndName(const String& directory, const String& filename);
    void InitializeWithDirectoryAndName(const WideString& directory, const WideString& filename);

    String ResolveResourcesPath() const;

    static String NormalizePathname(const String& pathname);

    static String MakeDirectory(const String& pathname);

    static String AbsoluteToRelative(const FilePath& directoryPathname, const FilePath& absolutePathname);

    static String GetFilename(const String& pathname);

    static String GetSystemPathname(const String& pathname, const ePathType pType);
    String GetFrameworkPathForPrefix(const String& typePrefix, const ePathType pType) const;

    static ePathType GetPathType(const String& pathname);

    static bool IsGlobbing(const String& pathname);

    String absolutePathname;
    ePathType pathType;
};

bool operator<(const FilePath& left, const FilePath& right);

inline bool FilePath::IsEmpty() const
{
    return (pathType == PATH_EMPTY);
}

inline FilePath::ePathType FilePath::GetType() const
{
    return pathType;
}

template <>
bool AnyCompare<FilePath>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);
extern template struct AnyCompare<FilePath>;

} // end namespace DAVA
