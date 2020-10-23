#include "ResaveUtility.h"

#include "Base/ScopedPtr.h"
#include "FileSystem/File.h"
#include "FileSystem/FileList.h"
#include "Logger/Logger.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "CommandLine/CommandLineParser.h"

namespace ResaveUtilityDetail
{
void EnumerateFolder(const DAVA::FilePath& folder, const DAVA::String& ext, DAVA::List<DAVA::FilePath>& files)
{
    using namespace DAVA;

    ScopedPtr<FileList> fileList(new FileList(folder));
    for (int32 i = 0, count = fileList->GetCount(); i < count; ++i)
    {
        const FilePath& pathname = fileList->GetPathname(i);
        if (fileList->IsDirectory(i) && !fileList->IsNavigationDirectory(i))
        {
            EnumerateFolder(pathname, ext, files);
        }
        else if (pathname.IsEqualToExtension(ext))
        {
            files.push_back(pathname);
        }
    }
}

void ReadFiles(const DAVA::FilePath& filelist, DAVA::List<DAVA::FilePath>& files)
{
    using namespace DAVA;

    ScopedPtr<File> fileWithLinks(File::Create(filelist, File::OPEN | File::READ));
    if (!fileWithLinks)
    {
        Logger::Error("Cannot open filelist %s", filelist.GetStringValue().c_str());
        return;
    }

    do
    {
        FilePath pathname = fileWithLinks->ReadLine();
        if (pathname.IsEmpty() == false && pathname.IsDirectoryPathname() == false)
        {
            files.push_back(pathname);
        }
    } while (!fileWithLinks->IsEof());
}

DAVA::String GetExtension(DAVA::String sourceString)
{
    using namespace DAVA;

    String::size_type pos = sourceString.rfind(".");
    if (pos == String::npos)
    {
        return "";
    }

    return sourceString.substr(pos);
}

} // ResaveUtilityDetail

ResaveUtility::ResaveUtility()
{
    using namespace DAVA;

    FilePath folder = CommandLineParser::GetCommandParam("-folder");
    FilePath file = CommandLineParser::GetCommandParam("-file");
    FilePath filelist = CommandLineParser::GetCommandParam("-filelist");

    if (file.IsEmpty() == false)
    {
        filesToResave.push_back(file);
    }
    else if (folder.IsEmpty() == false)
    {
        String ext = CommandLineParser::GetCommandParam("-ext");
        ext = ResaveUtilityDetail::GetExtension(ext);
        if (ext.empty())
        {
            Logger::Error("Wrong commandline: please provide extension for images in Engine format (e.g.  \".png\")");
            return;
        }

        folder.MakeDirectoryPathname();
        ResaveUtilityDetail::EnumerateFolder(folder, ext, filesToResave);
    }
    else if (filelist.IsEmpty() == false)
    {
        ResaveUtilityDetail::ReadFiles(filelist, filesToResave);
    }
    else
    {
        Logger::Error("Wrong commandline: please provide file, folder or filelist option");
    }
}

void ResaveUtility::Resave()
{
    using namespace DAVA;

    for (const FilePath& path : filesToResave)
    {
        ScopedPtr<Image> image(ImageSystem::LoadSingleMip(path));
        if (image)
        {
            ImageSystem::Save(path, image, image->format);
        }
        else
        {
            Logger::Error("Cannot open %s", path.GetStringValue().c_str());
        }
    }

    Logger::Info("%u files processed", static_cast<uint32>(filesToResave.size()));
}