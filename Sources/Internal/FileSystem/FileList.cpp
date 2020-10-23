#include "FileSystem/FileList.h"

#include "Engine/Engine.h"
#include "Logger/Logger.h"
#include "Utils/UTF8Utils.h"

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#elif defined(__DAVAENGINE_WINDOWS__)
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <direct.h>
#elif defined(__DAVAENGINE_ANDROID__)
#include "Engine/Private/Android/AssetsManagerAndroid.h"
#include <dirent.h>
#include <sys/stat.h>
#elif defined(__DAVAENGINE_LINUX__)
#include <dirent.h>
#include <sys/stat.h>
#endif //PLATFORMS

namespace DAVA
{
FileList::FileList(const FilePath& filepath, bool includeHidden)
{
    DVASSERT(filepath.IsDirectoryPathname());

    path = filepath;

// now check native FS for files
#if defined(__DAVAENGINE_WINDOWS__)

    struct _wfinddata_t c_file;
    intptr_t hFile;
    FileEntry entry;

    WideString searchPath = UTF8Utils::EncodeToWideString(path.GetAbsolutePathname());
    if (searchPath.back() == L'\\' || searchPath.back() == L'/')
        searchPath += L'*';
    else
        searchPath += L"/*";

    if ((hFile = _wfindfirst(searchPath.c_str(), &c_file)) != -1L)
    {
        do
        {
            //TODO: need to check for Win32
            entry.name = UTF8Utils::EncodeToUTF8(c_file.name);
            entry.path = filepath + entry.name;
            entry.size = c_file.size;
            entry.isHidden = (_A_HIDDEN & c_file.attrib) != 0;
            entry.isDirectory = (_A_SUBDIR & c_file.attrib) != 0;
            if (entry.isDirectory)
            {
                entry.path.MakeDirectoryPathname();
            }

            if (!entry.isHidden || includeHidden)
            {
                fileList.push_back(entry);
            }
            //Logger::FrameworkDebug("filelist: %s %s", filepath.c_str(), entry.name.c_str());
        } while (_wfindnext(hFile, &c_file) == 0);

        _findclose(hFile);
    }

//TODO add drives
//entry.Name = "E:\\";
//entry.isDirectory = true;
//Files.push_back(entry);
#elif defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
    struct dirent** namelist;
    FileEntry entry;

    int32 n = scandir(path.GetAbsolutePathname().c_str(), &namelist, 0, alphasort);

    if (n >= 0)
    {
        while (n--)
        {
            entry.path = path + namelist[n]->d_name;
            entry.name = namelist[n]->d_name;

            struct stat entry_stat;
            stat(entry.path.GetAbsolutePathname().c_str(), &entry_stat);

            if (DT_LNK == namelist[n]->d_type)
            {
                entry.isDirectory = (S_IFDIR == ((entry_stat.st_mode) & S_IFMT));
            }
            else
            {
                entry.isDirectory = (DT_DIR == namelist[n]->d_type);
            }

            entry.size = entry_stat.st_size;

            if (entry.name != "." && entry.name != "..")
            {
                entry.isHidden = (!entry.name.empty() && entry.name[0] == '.');
            }

            if (entry.isDirectory)
            {
                entry.path.MakeDirectoryPathname();
            }

            if (!entry.isHidden || includeHidden)
            {
                fileList.push_back(entry);
            }

            free(namelist[n]);
        }
        free(namelist);
    }
#elif defined(__DAVAENGINE_ANDROID__)

    const String& dirPath = path.GetAbsolutePathname();

    DIR* dir = opendir(dirPath.c_str());
    if (nullptr != dir)
    {
        // print all the files and directories within directory
        FileEntry entry;
        for (struct dirent* ent = readdir(dir); nullptr != ent; ent = readdir(dir))
        {
            String fileOrDirName = ent->d_name;
            if (fileOrDirName == "." || fileOrDirName == "..")
            {
                continue; // just skip. faster work less bugs
            }
            entry.name = fileOrDirName;
            entry.path = path + entry.name;

            if (ent->d_type != DT_DIR && ent->d_type != DT_REG)
            {
                Logger::Error("unsupported d_type in directory: %s", dirPath.c_str());
                continue;
            }

            entry.isDirectory = (DT_DIR == ent->d_type);
            entry.isHidden = (!entry.name.empty() && entry.name[0] == '.');
            entry.size = 0;

            if (!entry.isDirectory)
            {
                struct stat st;
                String fullPath = dirPath + fileOrDirName;
                if (stat(fullPath.c_str(), &st) == 0)
                {
                    entry.size = st.st_size;
                }
            }
            else
            {
                entry.path.MakeDirectoryPathname();
            }

            if (!entry.isHidden || includeHidden)
            {
                fileList.push_back(entry);
            }
        }
        closedir(dir);
    }
    else
    {
        // could not open directory try in APK assets
        AssetsManagerAndroid* assets = AssetsManagerAndroid::Instance();
        Vector<ResourceArchive::FileInfo> files;
        if (assets->ListDirectory(dirPath, files))
        {
            FileEntry entry;
            for (ResourceArchive::FileInfo& info : files)
            {
                entry.path = info.relativeFilePath;

                entry.size = info.originalSize;
                entry.isDirectory = (info.relativeFilePath.back() == '/');
                entry.isHidden = (!entry.name.empty() && entry.name[0] == '.');

                if (entry.isDirectory)
                {
                    entry.name = entry.path.GetLastDirectoryName();
                }
                else
                {
                    entry.name = entry.path.GetFilename();
                }

                if (!entry.isHidden || includeHidden)
                {
                    fileList.push_back(entry);
                }
            }
        }
    }
#elif defined(__DAVAENGINE_LINUX__)
    const String& dirPath = path.GetAbsolutePathname();

    DIR* dir = opendir(dirPath.c_str());
    if (nullptr != dir)
    {
        // print all the files and directories within directory
        FileEntry entry;
        for (struct dirent* ent = readdir(dir); nullptr != ent; ent = readdir(dir))
        {
            String fileOrDirName = ent->d_name;
            if (fileOrDirName == "." || fileOrDirName == "..")
            {
                continue; // just skip. faster work less bugs
            }
            entry.name = fileOrDirName;
            entry.path = path + entry.name;

            if (ent->d_type != DT_DIR && ent->d_type != DT_REG)
            {
                Logger::Error("unsupported d_type in directory: %s", dirPath.c_str());
                continue;
            }

            entry.isDirectory = (DT_DIR == ent->d_type);
            entry.isHidden = (!entry.name.empty() && entry.name[0] == '.');
            entry.size = 0;

            if (!entry.isDirectory)
            {
                struct stat st;
                String fullPath = dirPath + fileOrDirName;
                if (stat(fullPath.c_str(), &st) == 0)
                {
                    entry.size = st.st_size;
                }
            }
            else
            {
                entry.path.MakeDirectoryPathname();
            }

            if (!entry.isHidden || includeHidden)
            {
                fileList.push_back(entry);
            }
        }
        closedir(dir);
    }
#else
#error Unknown platform
#endif //PLATFORMS

    directoryCount = 0;
    fileCount = 0;
    for (uint32 fi = 0; fi < GetCount(); ++fi)
    {
        if (IsDirectory(fi))
        {
            if (!IsNavigationDirectory(fi))
                directoryCount++;
        }
        else
            fileCount++;
    }
}

FileList::~FileList()
{
}

uint32 FileList::GetCount() const
{
    return static_cast<uint32>(fileList.size());
}

uint32 FileList::GetFileCount() const
{
    return fileCount;
}

uint32 FileList::GetDirectoryCount() const
{
    return directoryCount;
}

const FilePath& FileList::GetPathname(uint32 index) const
{
    return fileList.at(index).path;
}

const String& FileList::GetFilename(uint32 index) const
{
    return fileList.at(index).name;
}

bool FileList::IsDirectory(uint32 index) const
{
    return fileList.at(index).isDirectory;
}

bool FileList::IsNavigationDirectory(uint32 index) const
{
    const String& filename = GetFilename(index);

    if ((filename == ".") || (filename == ".."))
    {
        return true;
    }

    return false;
}

bool FileList::IsHidden(uint32 index) const
{
    return fileList.at(index).isHidden;
}

uint32 FileList::GetFileSize(uint32 index) const
{
    return fileList[index].size;
}

void FileList::Sort()
{
    stable_sort(begin(fileList), end(fileList));
}

}; // end of namespace DAVA
