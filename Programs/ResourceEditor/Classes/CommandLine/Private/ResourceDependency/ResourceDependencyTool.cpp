#include "Classes/CommandLine/ResourceDependencyTool.h"
#include "Classes/CommandLine/Private/ResourceDependency/ResourceDependencyConstants.h"
#include "Classes/CommandLine/Private/ResourceDependency/ResourceDependency.h"

#include <REPlatform/CommandLine/SceneConsoleHelper.h>

#include <TArc/Utils/ModuleCollection.h>

#include <Version/Version.h>

#include <Engine/Engine.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/FileSystem.h>
#include <Logger/Logger.h>
#include <Render/GPUFamilyDescriptor.h>
#include <Utils/Utils.h>

#include <iostream>

namespace ResourceDependencyToolDetails
{
void RemoveNonPrintablePathnames(DAVA::Set<DAVA::FilePath>& pathnames)
{
    using namespace DAVA;

    for (auto it = pathnames.begin(); it != pathnames.end();)
    {
        const FilePath& path = *it;
        if ((path.IsEmpty() == true) || (path.GetType() == FilePath::PATH_IN_MEMORY))
        {
            it = pathnames.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
}

ResourceDependencyTool::ResourceDependencyTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "resourceDependency")
{
    using namespace DAVA;

    options.AddOption("--file", VariantType(String()), "Path to resource (*.tex, *.sc2, etc) file");
    options.AddOption("--list", VariantType(String()), "Path to file with list of resources (*.tex, *.sc2, etc) files");
    options.AddOption("--type", VariantType(String()), "Could be \'d\' or \'c\'. \'d\' means dependency for downloading, \'c\' - dependency for convertion");
    options.AddOption("--outfile", VariantType(String()), "Path to result file");
    options.AddOption("--version", VariantType(false), "Print tool version");
}

bool ResourceDependencyTool::PostInitInternal()
{
    using namespace DAVA;

    if (options.GetOption("--version").AsBool() == true)
    {
        requiredAction = Action::PrintVersion;
        return true;
    }

    String filePathname = options.GetOption("--file").AsString();
    String fileListPathname = options.GetOption("--list").AsString();
    if (filePathname.empty() == false && fileListPathname.empty() == true)
    {
        resources.push_back(filePathname);
    }
    else if (filePathname.empty() == true && fileListPathname.empty() == false)
    {
        ScopedPtr<File> linksFile(File::Create(fileListPathname, File::READ | File::OPEN));
        if (linksFile)
        {
            do
            {
                String link = linksFile->ReadLine();
                if (!link.empty())
                {
                    resources.push_back(link);
                }
            } while (!linksFile->IsEof());
        }
    }

    if (resources.empty() == true)
    {
        Logger::Error("Cannot read any resource path");
        return false;
    }

    String typeStr = options.GetOption("--type").AsString();
    if (typeStr == "c")
    {
        mode = eDependencyType::CONVERT;
    }
    else if (typeStr == "d")
    {
        mode = eDependencyType::DOWNLOAD;
    }
    else
    {
        Logger::Error("value of --type is not correct");
        return false;
    }

    outFile = options.GetOption("--outfile").AsString();
    if (outFile.IsEmpty())
    {
        Logger::Error("Out file was not selected");
        return false;
    }

    return true;
}

DAVA::ConsoleModule::eFrameResult ResourceDependencyTool::OnFrameInternal()
{
    using namespace DAVA;

    if (requiredAction == Action::PrintVersion)
    {
        String version = Version::CreateAppVersion("Resource Editor");
        std::cout << version << std::endl;
    }
    else // dependency
    {
        Map<FilePath, Set<FilePath>> dependencyMap;
        if (ResourceDependency::GetDependencies(resources, dependencyMap, mode) == true)
        {
            GetEngineContext()->fileSystem->CreateDirectory(outFile.GetDirectory(), true);
            ScopedPtr<File> file(File::Create(outFile, File::WRITE | File::CREATE));
            if (file)
            {
                file->WriteLine("[");
                size_t resourcesCount = dependencyMap.size();
                size_t resourceIndex = 0;
                for (std::pair<const FilePath, Set<FilePath>>& pair : dependencyMap)
                {
                    file->WriteLine("  {");
                    file->WriteLine("    \"path\":\"" + pair.first.GetAbsolutePathname() + "\",");
                    file->WriteLine("    \"deps\": [");

                    ResourceDependencyToolDetails::RemoveNonPrintablePathnames(pair.second);

                    FilePath rootDirectory = pair.first.GetDirectory();
                    size_t pathCount = pair.second.size();
                    size_t pathIndex = 0;
                    for (const FilePath& path : pair.second)
                    {
                        String pathDelimeter = (pathIndex++ != pathCount - 1) ? "," : "";
                        String relativePath = path.GetRelativePathname(rootDirectory);
                        file->WriteLine("      \"" + relativePath + "\"" + pathDelimeter);
                    }
                    file->WriteLine("    ]");

                    String resourceDelimeter = (resourceIndex++ != resourcesCount - 1) ? "," : "";
                    file->WriteLine("  }" + resourceDelimeter);
                }
                file->WriteLine("]");
            }
            else
            {
                result = Result::RESULT_ERROR;
            }
        }
        else
        {
            result = Result::RESULT_ERROR;
        }
    }

    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

void ResourceDependencyTool::BeforeDestroyedInternal()
{
    DAVA::SceneConsoleHelper::FlushRHI();
}

void ResourceDependencyTool::ShowHelpInternal()
{
    using namespace DAVA;

    CommandLineModule::ShowHelpInternal();

    Logger::Info("Examples:");
    Logger::Info("\tresourceDependency --file absolute_path_to_resource --outfile absolute_path_to_outfile --type \'c\'|\'d' --> to get dependency of resource(*.tex, *.sc2, etc)");
    Logger::Info("\tresourceDependency --list absolute_path_to_file_with_list_of_resources --outfile absolute_path_to_outfile --type \'c\'|\'d' --> to get dependency of all resources in list(*.tex, *.sc2, etc)");
}

DECL_TARC_MODULE(ResourceDependencyTool);
