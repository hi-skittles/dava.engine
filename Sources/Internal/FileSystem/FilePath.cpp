#include "FileSystem/FilePath.h"
#include "FileSystem/FileSystem.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Utils/UTF8Utils.h"
#include "Utils/Utils.h"
#include "Utils/StringFormat.h"
#include "Utils/UTF8Utils.h"
#include "Logger/Logger.h"
#include "Engine/Engine.h"

#if defined(__DAVAENGINE_MACOS__)
#include <pwd.h>
#include <unistd.h>
#endif

// #define USE_LOCAL_RESOURCES

namespace DAVA
{
void FilePath::SetBundleName(const FilePath& newBundlePath)
{
    FilePath virtualBundlePath = newBundlePath;

    if (!virtualBundlePath.IsEmpty())
    {
        virtualBundlePath.MakeDirectoryPathname();
    }

    virtualBundlePath.pathType = PATH_IN_RESOURCES;

    const EngineContext* ctx = GetEngineContext();
    if (!ctx->fileSystem->resourceFolders.empty())
    {
        ctx->fileSystem->resourceFolders.erase(begin(ctx->fileSystem->resourceFolders));
    }

    ctx->fileSystem->resourceFolders.insert(begin(ctx->fileSystem->resourceFolders), virtualBundlePath);
}

const FilePath& FilePath::GetBundleName()
{
    const EngineContext* ctx = GetEngineContext();
    DVASSERT(ctx->fileSystem->resourceFolders.size());
    return ctx->fileSystem->resourceFolders.front();
}

void FilePath::AddResourcesFolder(const FilePath& folder)
{
    DVASSERT(!folder.IsEmpty());

    RemoveResourcesFolder(folder); // we need to remove folder from list to organize correct order of resource folders

    FilePath resPath = folder;
    resPath.pathType = PATH_IN_RESOURCES;

    const EngineContext* ctx = GetEngineContext();
    ctx->fileSystem->resourceFolders.push_back(resPath);
}

void FilePath::AddTopResourcesFolder(const FilePath& folder)
{
    DVASSERT(!folder.IsEmpty());

    RemoveResourcesFolder(folder); // we need to remove folder from list to organize correct order of resource folders

    FilePath resPath = folder;
    resPath.pathType = PATH_IN_RESOURCES;

    const EngineContext* ctx = GetEngineContext();
    ctx->fileSystem->resourceFolders.insert(begin(ctx->fileSystem->resourceFolders), resPath);
}

void FilePath::RemoveResourcesFolder(const FilePath& folder)
{
    const EngineContext* ctx = GetEngineContext();
    auto it = std::remove(begin(ctx->fileSystem->resourceFolders), end(ctx->fileSystem->resourceFolders), folder);
    if (it != end(ctx->fileSystem->resourceFolders))
    {
        ctx->fileSystem->resourceFolders.erase(it);
    }
}

const Vector<FilePath>& FilePath::GetResFolders()
{
    const EngineContext* ctx = GetEngineContext();
    return ctx->fileSystem->resourceFolders;
}

const List<FilePath>& FilePath::GetResourcesFolders()
{
    // for backward compatibility use list values
    static List<FilePath> list;

    const EngineContext* ctx = GetEngineContext();
    if (list.size() != ctx->fileSystem->resourceFolders.size() ||
        !std::equal(begin(ctx->fileSystem->resourceFolders), end(ctx->fileSystem->resourceFolders), begin(list)))
    {
        list.clear();

        std::copy(begin(ctx->fileSystem->resourceFolders), end(ctx->fileSystem->resourceFolders), std::back_inserter(list));
    }

    return list;
}

#if defined(__DAVAENGINE_WIN_UAP__)
String GetResourceDirName(const String& arch, const String& dirName, const String& resPrefix)
{
    String result;

    size_t idx = dirName.find(arch);
    if (idx != String::npos)
    {
        result = dirName;
        result.replace(idx, arch.size(), resPrefix);
    }

    return result;
}
#endif

#if defined(__DAVAENGINE_WINDOWS__)
void FilePath::InitializeBundleName()
{
    FilePath execDirectory = FileSystem::Instance()->GetCurrentExecutableDirectory();
    FilePath workingDirectory = FileSystem::Instance()->GetCurrentWorkingDirectory();
    SetBundleName(execDirectory + "Data/");

    if (workingDirectory != execDirectory)
    {
        FilePath dataDirPath(workingDirectory + "Data/");
        if (FileSystem::Instance()->Exists(dataDirPath))
        {
            AddResourcesFolder(dataDirPath);
        }
    }

#if defined(__DAVAENGINE_WIN_UAP__) && defined(DAVA_WIN_UAP_RESOURCES_DEPLOYMENT_LOCATION)
    String additionalResourcePath;

    //get the directory basename
    String dirBaseName = execDirectory.GetLastDirectoryName();
    std::transform(dirBaseName.begin(), dirBaseName.end(), dirBaseName.begin(), ::tolower);

//find resource dir name
#if defined(_M_IX86)
    String arch = "_x86_";
#elif defined(_M_X64)
    String arch = "_x64_";
#elif defined(_M_ARM)
    String arch = "_arm_";
#endif
    String resourceDir = GetResourceDirName(arch, dirBaseName, DAVA_WIN_UAP_RESOURCES_PREFIX);

    //resource dir found, use it
    if (!resourceDir.empty())
    {
        additionalResourcePath = "../" + resourceDir + '/';
    }

    //additional resource path for resources
    additionalResourcePath += DAVA_WIN_UAP_RESOURCES_DEPLOYMENT_LOCATION;
    additionalResourcePath += '/';

    AddResourcesFolder(execDirectory + additionalResourcePath);
    if (workingDirectory != execDirectory)
    {
        AddResourcesFolder(workingDirectory + additionalResourcePath);
    }
#endif
}
#elif defined(__DAVAENGINE_ANDROID__)
void FilePath::InitializeBundleName()
{
#ifdef USE_LOCAL_RESOURCES
    SetBundleName(FilePath("/mnt/sdcard/DavaProject/Data/"));
#endif
}
#elif defined(__DAVAENGINE_LINUX__)
void FilePath::InitializeBundleName()
{
    // TODO: linux
    FilePath execDirectory = FileSystem::Instance()->GetCurrentExecutableDirectory();
    FilePath workingDirectory = FileSystem::Instance()->GetCurrentWorkingDirectory();

    SetBundleName(execDirectory + "Data/");

    if (workingDirectory != execDirectory)
    {
        FilePath dataDirPath(workingDirectory + "Data/");
        if (FileSystem::Instance()->Exists(dataDirPath))
        {
            AddResourcesFolder(dataDirPath);
        }
    }
}
#endif

FilePath FilePath::FilepathInDocuments(const char8* relativePathname)
{
    FilePath path(FileSystem::Instance()->GetCurrentDocumentsDirectory() + relativePathname);
    path.pathType = PATH_IN_DOCUMENTS;
    return path;
}

FilePath FilePath::FilepathInDocuments(const String& relativePathname)
{
    return FilepathInDocuments(relativePathname.c_str());
}

bool FilePath::StartsWith(const FilePath& basePath) const
{
    DVASSERT(!basePath.IsEmpty());
    // if both path starts with ~res:/ we can just compare text without conversion to absolute path
    if (absolutePathname.compare(0, 6, "~res:/") == 0 && basePath.absolutePathname.compare(0, 6, "~res:/") == 0)
    {
        return absolutePathname.compare(0, basePath.absolutePathname.size(), basePath.absolutePathname) == 0;
    }

    const String baseStr = basePath.GetAbsolutePathname();
    const String thisStr = GetAbsolutePathname();
    return thisStr.compare(0, baseStr.size(), baseStr) == 0;
}

bool FilePath::ContainPath(const FilePath& basePath, const FilePath& partPath)
{
    return basePath.GetAbsolutePathname().find(partPath.GetAbsolutePathname()) != std::string::npos;
}

bool FilePath::ContainPath(const FilePath& basePath, const String& partPath)
{
    return basePath.GetAbsolutePathname().find(partPath) != std::string::npos;
}

bool FilePath::ContainPath(const FilePath& basePath, const char8* partPath)
{
    return ContainPath(basePath, String(partPath));
}

bool operator<(const FilePath& left, const FilePath& right)
{
    return left.Compare(right) < 0;
}

FilePath::FilePath()
{
    pathType = PATH_EMPTY;
}

FilePath::FilePath(const FilePath& path)
{
    pathType = path.pathType;
    absolutePathname = path.absolutePathname;
}

FilePath::FilePath(FilePath&& path)
    : absolutePathname(std::move(path.absolutePathname))
    , pathType(path.pathType)
{
    path.pathType = PATH_EMPTY;
}

FilePath::FilePath(const char8* sourcePath)
{
    Initialize(String(sourcePath));
}

FilePath::FilePath(const char16* sourcePath)
{
    Initialize(WideString(sourcePath));
}

FilePath::FilePath(const String& pathname)
{
    Initialize(pathname);
}

FilePath::FilePath(const WideString& pathname)
{
    Initialize(pathname);
}

FilePath::FilePath(const char8* directory, const String& filename)
{
    InitializeWithDirectoryAndName(String(directory), filename);
}

FilePath::FilePath(const String& directory, const String& filename)
{
    InitializeWithDirectoryAndName(directory, filename);
}

FilePath::FilePath(const char16* directory, const WideString& filename)
{
    InitializeWithDirectoryAndName(WideString(directory), filename);
}

FilePath::FilePath(const WideString& directory, const WideString& filename)
{
    InitializeWithDirectoryAndName(directory, filename);
}

FilePath::FilePath(const FilePath& directory, const String& filename)
{
    DVASSERT(directory.IsDirectoryPathname());

    pathType = directory.pathType;
    absolutePathname = AddPath(directory, filename);
}

FilePath::FilePath(const FilePath& directory, const WideString& filename)
{
    DVASSERT(directory.IsDirectoryPathname());

    pathType = directory.pathType;
    absolutePathname = AddPath(directory, UTF8Utils::EncodeToUTF8(filename));
}

void FilePath::InitializeWithDirectoryAndName(const String& directory, const String& filename)
{
    FilePath directoryPath(directory);
    DVASSERT(!directoryPath.IsEmpty());
    directoryPath.MakeDirectoryPathname();

    pathType = directoryPath.pathType;
    absolutePathname = AddPath(directoryPath, filename);
}

void FilePath::InitializeWithDirectoryAndName(const WideString& directory, const WideString& filename)
{
    InitializeWithDirectoryAndName(UTF8Utils::EncodeToUTF8(directory), UTF8Utils::EncodeToUTF8(filename));
}

void FilePath::Initialize(const String& _pathname)
{
    pathType = GetPathType(_pathname);
    if (pathType == PATH_IN_MEMORY)
    {
        absolutePathname = _pathname;
        return;
    }

    String pathname = NormalizePathname(_pathname);

    if (pathType == PATH_EMPTY)
    {
        absolutePathname = String();
    }
    else if (pathType == PATH_IN_RESOURCES)
    {
        absolutePathname = pathname;
    }
    else if (pathType == PATH_IN_DOCUMENTS)
    {
        absolutePathname = GetSystemPathname(pathname, pathType);
    }
    else if (IsAbsolutePathname(pathname))
    {
#if defined(__DAVAENGINE_MACOS__)
        if (IsGlobbing(pathname))
        {
            char* value = getenv("HOME");
            if (!value)
            {
                // No $HOME variable, check the password database.
                passwd* pw = getpwuid(getuid());
                value = pw->pw_dir;
            }

            absolutePathname = value + pathname.substr(1, -1);
        }
        else
#endif
        {
            absolutePathname = pathname;
        }
    }
    else
    {
#if defined(__DAVAENGINE_ANDROID__)
        absolutePathname = pathname;
#else //#if defined(__DAVAENGINE_ANDROID__)
        const EngineContext* ctx = GetEngineContext();
        FilePath path = ctx->fileSystem->GetCurrentWorkingDirectory() + pathname;
        absolutePathname = path.GetAbsolutePathname();
#endif //#if defined(__DAVAENGINE_ANDROID__)
    }
}

void FilePath::Initialize(const WideString& _pathname)
{
    Initialize(UTF8Utils::EncodeToUTF8(_pathname));
}

FilePath::~FilePath()
{
}

String FilePath::GetAbsolutePathname() const
{
    if (pathType == PATH_IN_RESOURCES)
    {
        return ResolveResourcesPath();
    }

    return absolutePathname;
}

#if defined(__DAVAENGINE_WINDOWS__)
FilePath::NativeStringType FilePath::GetNativeAbsolutePathname() const
{
    String str = GetAbsolutePathname();
    return UTF8Utils::EncodeToWideString(str);
}

FilePath FilePath::FromNativeString(const NativeStringType& path)
{
    String str = UTF8Utils::EncodeToUTF8(path);
    return FilePath(str);
}
#else
FilePath::NativeStringType FilePath::GetNativeAbsolutePathname() const
{
    return GetAbsolutePathname();
}

FilePath FilePath::FromNativeString(const NativeStringType& path)
{
    return FilePath(path);
}
#endif

String FilePath::ResolveResourcesPath() const
{
    const char* absStr = absolutePathname.c_str();
    if (0 == strncmp(absStr, "~res:/", 6))
    {
        String relativePathname = absolutePathname.substr(6);
        FilePath path;

        const EngineContext* ctx = GetEngineContext();
        for (auto reverseIt = ctx->fileSystem->resourceFolders.rbegin(); reverseIt != ctx->fileSystem->resourceFolders.rend(); ++reverseIt)
        {
            path = reverseIt->absolutePathname + relativePathname;
            if (ctx->fileSystem->Exists(path))
            {
                return path.absolutePathname;
            }
        }
        return relativePathname;
    }

    return absolutePathname;
}

FilePath& FilePath::operator=(const FilePath& path)
{
    this->absolutePathname = path.absolutePathname;
    this->pathType = path.pathType;

    return *this;
}

FilePath& FilePath::operator=(FilePath&& path)
{
    absolutePathname = std::move(path.absolutePathname);
    pathType = path.pathType;
    path.pathType = PATH_EMPTY;

    return *this;
}

FilePath FilePath::operator+(const String& path) const
{
    FilePath pathname(AddPath(*this, path));

    pathname.pathType = this->pathType;
    if (this->pathType == PATH_EMPTY)
    {
        pathname.pathType = GetPathType(pathname.absolutePathname);
    }

    return pathname;
}

FilePath& FilePath::operator+=(const String& path)
{
    if (pathType == PATH_EMPTY)
    {
        Initialize(path);
    }
    else
    {
        absolutePathname = AddPath(*this, path);
    }

    return (*this);
}

bool FilePath::operator==(const FilePath& path) const
{
    return absolutePathname == path.absolutePathname;
}

bool FilePath::operator!=(const FilePath& path) const
{
    return !(*this == path);
}

bool FilePath::IsDirectoryPathname() const
{
    if (IsEmpty())
    {
        return false;
    }

    const auto lastPosition = absolutePathname.length() - 1;
    return (absolutePathname.at(lastPosition) == '/');
}

String FilePath::GetFilename() const
{
    return GetFilename(absolutePathname);
}

String FilePath::GetFilename(const String& pathname)
{
    String::size_type slashpos = pathname.rfind(String("/"));
    if (slashpos == String::npos)
        return pathname;

    return pathname.substr(slashpos + 1);
}

String FilePath::GetBasename() const
{
    if (IsDirectoryPathname())
    {
        return GetLastDirectoryName();
    }
    else
    {
        String filename = GetFilename();
        const String::size_type dotpos = filename.rfind(String("."));
        if (String::npos != dotpos)
        {
            filename = filename.substr(0, dotpos);
        }

        return filename;
    }
}

DAVA::String FilePath::GetName() const
{
    if (IsDirectoryPathname())
    {
        return GetLastDirectoryName();
    }
    else
    {
        return GetFilename();
    }
}

String FilePath::GetExtension() const
{
    const String filename = GetFilename();

    const String::size_type dotpos = filename.rfind(String("."));
    if (dotpos == String::npos)
        return String();

    return filename.substr(dotpos);
}

FilePath FilePath::GetDirectory() const
{
    FilePath directory;

    const String::size_type slashpos = absolutePathname.rfind(String("/"));
    if (slashpos != String::npos)
    {
        directory = absolutePathname.substr(0, slashpos + 1);
    }

    directory.pathType = pathType;
    return directory;
}

String FilePath::GetRelativePathname() const
{
    FileSystem* fs = GetEngineContext()->fileSystem;
    const FilePath cwd = fs->GetCurrentWorkingDirectory();
    return GetRelativePathname(cwd);
}

String FilePath::GetRelativePathname(const FilePath& forDirectory) const
{
    if (forDirectory.IsEmpty())
        return GetAbsolutePathname();

    DVASSERT(forDirectory.IsDirectoryPathname());

    return AbsoluteToRelative(forDirectory, *this);
}

String FilePath::GetRelativePathname(const String& forDirectory) const
{
    if (forDirectory.empty())
    {
        return String();
    }

    return GetRelativePathname(FilePath(forDirectory));
}

String FilePath::GetRelativePathname(const char8* forDirectory) const
{
    if (forDirectory == nullptr)
    {
        return String();
    }

    return GetRelativePathname(FilePath(forDirectory));
}

const String& FilePath::GetStringValue() const
{
    return absolutePathname;
}

void FilePath::ReplaceFilename(const String& filename)
{
    DVASSERT(!IsEmpty());

    absolutePathname = (GetDirectory() + filename).absolutePathname;
}

void FilePath::ReplaceBasename(const String& basename)
{
    if (!IsEmpty())
    {
        const String extension = GetExtension();
        absolutePathname = (GetDirectory() + (basename + extension)).absolutePathname;
    }
}

void FilePath::ReplaceExtension(const String& extension)
{
    if (!IsEmpty())
    {
        const String basename = GetBasename();
        absolutePathname = (GetDirectory() + (basename + extension)).absolutePathname;
    }
}

void FilePath::ReplaceDirectory(const String& directory)
{
    DVASSERT(!IsEmpty());

    const String filename = GetFilename();
    Initialize((MakeDirectory(directory) + filename));
}

void FilePath::ReplaceDirectory(const FilePath& directory)
{
    DVASSERT(!IsEmpty());

    DVASSERT(directory.IsDirectoryPathname());
    const String filename = GetFilename();

    absolutePathname = (directory + filename).absolutePathname;
    pathType = directory.pathType;
}

FilePath& FilePath::MakeDirectoryPathname()
{
    DVASSERT(!IsEmpty());

    absolutePathname = MakeDirectory(absolutePathname);

    return *this;
}

void FilePath::TruncateExtension()
{
    DVASSERT(!IsEmpty());

    ReplaceExtension(String(""));
}

String FilePath::GetLastDirectoryName() const
{
    DVASSERT(!IsEmpty() && IsDirectoryPathname());

    String path = absolutePathname;
    path.pop_back();

    return FilePath(path).GetFilename();
}

bool FilePath::IsEqualToExtension(const String& extension) const
{
    DVASSERT(!extension.empty() && extension.at(0) != '*');

    String selfExtension = GetExtension();
    return (CompareCaseInsensitive(extension, selfExtension) == 0);
}

FilePath FilePath::CreateWithNewExtension(const FilePath& pathname, const String& extension)
{
    FilePath path(pathname);
    path.ReplaceExtension(extension);
    return path;
}

String FilePath::GetSystemPathname(const String& pathname, const ePathType pType)
{
    if (pType == PATH_IN_FILESYSTEM || pType == PATH_IN_MEMORY)
        return pathname;

    String retPath = pathname;
    if (pType == PATH_IN_RESOURCES)
    {
        retPath = FilePath(retPath).GetAbsolutePathname();
    }
    else if (pType == PATH_IN_DOCUMENTS)
    {
        retPath = retPath.erase(0, 5);
        retPath = FilepathInDocuments(retPath).GetAbsolutePathname();
    }

    return NormalizePathname(retPath);
}

struct IsPathStartingWith
{
    const String& start;
    IsPathStartingWith(const String& p)
        : start(p)
    {
    }
    bool operator()(const FilePath& p) const
    {
        const String& path = p.GetStringValue();
        return start.find(path) == 0;
    }
};

String FilePath::GetFrameworkPath() const
{
    if (PATH_IN_RESOURCES == pathType)
        return absolutePathname;

    String pathInRes = GetFrameworkPathForPrefix("~res:/", PATH_IN_RESOURCES);
    if (!pathInRes.empty())
    {
        return pathInRes;
    }

    String pathInDoc = GetFrameworkPathForPrefix("~doc:/", PATH_IN_DOCUMENTS);
    if (!pathInDoc.empty())
    {
        return pathInDoc;
    }

    const EngineContext* ctx = GetEngineContext();
    // search starting from last added directories
    auto it = std::find_if(rbegin(ctx->fileSystem->resourceFolders), rend(ctx->fileSystem->resourceFolders),
                           IsPathStartingWith(absolutePathname));

    if (it != rend(ctx->fileSystem->resourceFolders))
    {
        const String& s = it->GetStringValue();
        String copy = absolutePathname;
        return copy.replace(0, s.size(), "~res:/");
    }

    DVASSERT(false);

    return String();
}

String FilePath::GetFrameworkPathForPrefix(const String& typePrefix, const ePathType pType) const
{
    DVASSERT(!typePrefix.empty());

    String prefixPathname = GetSystemPathname(typePrefix, pType);

    String::size_type pos = absolutePathname.find(prefixPathname);
    if (pos == 0)
    {
        String pathname = absolutePathname;
        pathname = pathname.replace(pos, prefixPathname.length(), typePrefix);
        return pathname;
    }

    return String();
}

String FilePath::NormalizePathname(const String& pathname)
{
    if (pathname.empty())
        return String();

    String path = pathname;
    std::replace(path.begin(), path.end(), '\\', '/');

    Vector<String> tokens;
    Split(path, "/", tokens);

    //TODO: correctly process situation ../../folders/filename
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (String(".") == tokens[i])
        {
            for (size_t k = i + 1; k < tokens.size(); ++k)
            {
                tokens[k - 1] = tokens[k];
            }
            --i;
            tokens.pop_back();
        }
        else if ((1 <= i) && (String("..") == tokens[i] && String("..") != tokens[i - 1]))
        {
            for (size_t k = i + 1; k < tokens.size(); ++k)
            {
                tokens[k - 2] = tokens[k];
            }
            i -= 2;
            tokens.pop_back();
            tokens.pop_back();
        }
    }

    String result = "";
    if ('/' == path[0])
        result = "/";

    for (size_t k = 0; k < tokens.size(); ++k)
    {
        result += tokens[k];
        if (k + 1 != tokens.size())
            result += String("/");
    }

    //process last /
    if (('/' == path.back()) && (!result.empty()))
    {
        result += String("/");
    }

    return result;
}

String FilePath::MakeDirectory(const String& pathname)
{
    if (pathname.empty())
    {
        return String();
    }

    const auto lastPosition = pathname.length() - 1;
    if (pathname.at(lastPosition) != '/')
    {
        return pathname + String("/");
    }

    return pathname;
}

String FilePath::AbsoluteToRelative(const FilePath& directoryPathname, const FilePath& absolutePathname)
{
    if (absolutePathname.IsEmpty())
        return String();

    DVASSERT(directoryPathname.IsDirectoryPathname());

    Vector<String> folders;
    Vector<String> fileFolders;

    if (directoryPathname.GetType() == PATH_IN_RESOURCES && absolutePathname.GetType() == PATH_IN_RESOURCES)
    {
        Split(directoryPathname.absolutePathname, "/", folders);
        Split(absolutePathname.GetDirectory().absolutePathname, "/", fileFolders);
    }
    else
    {
        Split(directoryPathname.GetAbsolutePathname(), "/", folders);
        Split(absolutePathname.GetDirectory().GetAbsolutePathname(), "/", fileFolders);
    }

    Vector<String>::size_type equalCount = 0;
    for (; equalCount < folders.size() && equalCount < fileFolders.size(); ++equalCount)
    {
        if (folders[equalCount] != fileFolders[equalCount])
        {
            break;
        }
    }

    String retPath = "";
    for (Vector<String>::size_type i = equalCount; i < folders.size(); ++i)
    {
        retPath += "../";
    }

    for (Vector<String>::size_type i = equalCount; i < fileFolders.size(); ++i)
    {
        retPath += fileFolders[i] + "/";
    }

    return (retPath + absolutePathname.GetFilename());
}

bool FilePath::IsGlobbing(const String& pathname)
{
    if (pathname.empty() || pathname.length() < 2)
    {
        return false;
    }

    if (pathname[0] == '~' && pathname[1] == '/')
    {
        return true;
    }
    return false;
}

bool FilePath::IsAbsolutePathname(const String& pathname)
{
    if (pathname.empty())
        return false;

    //Unix style
    if ((pathname[0] == '/') || IsGlobbing(pathname))
        return true;

    //Win or DAVA style (c:/, ~res:/, ~doc:/)
    String::size_type winFound = pathname.find(":");
    if (winFound != String::npos)
    {
        return true;
    }

    return false;
}

String FilePath::AddPath(const FilePath& path, const String& addition)
{
    if (path.IsEmpty())
        return NormalizePathname(addition);

    String absPathname = path.absolutePathname + addition;
    if (path.pathType == PATH_IN_RESOURCES && absPathname.find("~res:") == 0)
    {
        const String frameworkPath = GetSystemPathname("~res:/", PATH_IN_RESOURCES) + "Data";
        absPathname = NormalizePathname(frameworkPath + absPathname.substr(5));

        if (absPathname.find(frameworkPath) == 0)
        {
            absPathname.replace(0, frameworkPath.length(), "~res:");
        }

        return absPathname;
    }

    return NormalizePathname(absPathname);
}

FilePath::ePathType FilePath::GetPathType(const String& pathname)
{
    if (pathname.empty())
    {
        return PATH_EMPTY;
    }

    String::size_type find = pathname.find("~res:");
    if (find == 0)
    {
        return PATH_IN_RESOURCES;
    }

#if defined(__DAVAENGINE_ANDROID__) && defined(USE_LOCAL_RESOURCES)
    if (0 == pathname.find("~zip:"))
    {
        return PATH_IN_RESOURCES;
    }
#endif

    find = pathname.find("~doc:");
    if (find == 0)
    {
        return PATH_IN_DOCUMENTS;
    }

    if ((pathname.find("FBO ") == 0)
        || (pathname.find("memoryfile_") == 0)
        || (pathname.find("Text ") == 0))
    {
        return PATH_IN_MEMORY;
    }

    return PATH_IN_FILESYSTEM;
}

bool FilePath::Exists() const
{
    return FileSystem::Instance()->Exists(*this);
}

int32 FilePath::Compare(const FilePath& right) const
{
    if (absolutePathname < right.absolutePathname)
        return -1;
    if (absolutePathname > right.absolutePathname)
        return 1;

    return 0;
}

String FilePath::AsURL() const
{
    String path = GetAbsolutePathname();
// HACK this code incorrect but works
// how do we know where file exist on android FS or inside APK(Zip)
// here we always
#if defined(__DAVAENGINE_ANDROID__)
    if (path.empty())
    {
        return "file:///android_asset/Data/";
    }
    else if (path.at(0) != '/')
    {
        return ("file:///android_asset/Data/" + path);
    }
#endif //#if defined(__DAVAENGINE_ANDROID__)

    return ("file://" + path);
}

template <>
bool AnyCompare<FilePath>::IsEqual(const Any& v1, const Any& v2)
{
    return v1.Get<FilePath>() == v2.Get<FilePath>();
}
}
