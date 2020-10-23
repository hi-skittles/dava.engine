#include "Classes/CommandLine/ResourceDependencyTool.h"

#include <REPlatform/CommandLine/SceneConsoleHelper.h>
#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>

#include <TArc/Testing/ConsoleModuleTestExecution.h>
#include <TArc/Testing/TArcUnitTests.h>

#include <Base/ScopedPtr.h>
#include <Engine/Engine.h>
#include <FileSystem/File.h>
#include <FileSystem/FileList.h>
#include <FileSystem/FileSystem.h>
#include <Render/RenderBase.h>
#include <Render/TextureDescriptor.h>
#include <Render/TextureDescriptorUtils.h>
#include <Render/Highlevel/Heightmap.h>
#include <Render/Image/Image.h>
#include <Render/Image/ImageSystem.h>

#include <memory>

namespace DependencyTestDetails
{
const DAVA::String projectStr = "~doc:/Test/DependencyTool/";
const DAVA::String scenePathnameStr = projectStr + "DataSource/3d/Scene/testScene.sc2";
const DAVA::String texture3DPathnameStr = projectStr + "DataSource/3d/Texture/3D/texture.tex";
const DAVA::String textureCubePathnameStr = projectStr + "DataSource/3d/Texture/Cube/texture.tex";

const DAVA::String outStr = projectStr + "out.txt";
const DAVA::String fileListStr = projectStr + "file.list";

bool FoundInFileImpl(const DAVA::FilePath& outFile, const DAVA::FilePath& rootDirectory, const DAVA::Vector<DAVA::FilePath>& pathes, bool comparisionValue)
{
    using namespace DAVA;

    ScopedPtr<File> file(File::Create(outFile, File::READ | File::OPEN));
    if (file)
    {
        String fileContent;
        file->ReadString(fileContent);

        if (fileContent.empty() == false)
        {
            for (const FilePath& p : pathes)
            {
                if ((fileContent.find(p.GetRelativePathname(rootDirectory)) == String::npos) == comparisionValue)
                {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
}

bool FoundInFile(const DAVA::FilePath& outFile, const DAVA::FilePath& rootDirectory, const DAVA::Vector<DAVA::FilePath>& pathes)
{
    return FoundInFileImpl(outFile, rootDirectory, pathes, true);
}

bool NotFoundInFile(const DAVA::FilePath& outFile, const DAVA::FilePath& rootDirectory, const DAVA::Vector<DAVA::FilePath>& pathes)
{
    return FoundInFileImpl(outFile, rootDirectory, pathes, false);
}
}

// clang-format off
DAVA_TARC_TESTCLASS(ResourceDependencyToolTest)
{
private:
    std::unique_ptr<DAVA::CommandLineModuleTestUtils::TextureLoadingGuard> textureLoadingGuard;

    DAVA::FilePath projectPathname;

    DAVA::FilePath scenePathname;
    DAVA::FilePath texture3DPathname;
    DAVA::FilePath textureCubePathname;

    DAVA::FilePath outfilePathname;
    DAVA::FilePath filelistPathname;

    DAVA::Vector<DAVA::FilePath> facePathnames;
    DAVA::FilePath pngPathname;

    DAVA::Vector<DAVA::FilePath> textures;
    DAVA::Vector<DAVA::FilePath> heightmaps;

    void EnumerateSceneFiles(const DAVA::FilePath& folderPathname)
    {
        using namespace DAVA;

        ScopedPtr<FileList> fileList(new FileList(folderPathname));
        for (int32 i = 0, count = fileList->GetCount(); i < count; ++i)
        {
            const FilePath& pathname = fileList->GetPathname(i);
            if (fileList->IsDirectory(i) == false)
            {
                String filename = pathname.GetFilename();
                
                if((filename.find(".china.") != String::npos) || (filename.find(".japan.") != String::npos ))
                {   // ignore tags
                    continue;
                }
                
                if (pathname.IsEqualToExtension(TextureDescriptor::GetDescriptorExtension()))
                {
                    textures.push_back(pathname);
                }
                else if (pathname.IsEqualToExtension(Heightmap::FileExtension()))
                {
                    heightmaps.push_back(pathname);
                }
            }
        }
    }

public:
    ResourceDependencyToolTest()
    {
        using namespace DAVA;

        {   // resolve ~doc:// path in same folder.
            projectPathname = FilePath(DependencyTestDetails::projectStr).GetAbsolutePathname();
            
            scenePathname = FilePath(DependencyTestDetails::scenePathnameStr).GetAbsolutePathname();;
            texture3DPathname = FilePath(DependencyTestDetails::texture3DPathnameStr).GetAbsolutePathname();;
            textureCubePathname = FilePath(DependencyTestDetails::textureCubePathnameStr).GetAbsolutePathname();;
            
            outfilePathname = FilePath(DependencyTestDetails::outStr).GetAbsolutePathname();
            filelistPathname = FilePath(DependencyTestDetails::fileListStr).GetAbsolutePathname();
        }

        {   // clear data from interrupted or crashed tests
            CommandLineModuleTestUtils::ClearTestFolder(projectPathname);
        }


        // create test data
        textureLoadingGuard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(projectPathname);

        { // create scene
            CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(scenePathname, projectPathname);
            EnumerateSceneFiles(scenePathname.GetDirectory());
        }

        { // create textures
            GetEngineContext()->fileSystem->CreateDirectory(texture3DPathname.GetDirectory(), true);
            CommandLineModuleTestUtils::CreateTextureFiles(texture3DPathname, 16, 16, PixelFormat::FORMAT_RGBA8888, 20);
            pngPathname = FilePath::CreateWithNewExtension(texture3DPathname, ".png");


            FilePath cubeDirectory = textureCubePathname.GetDirectory();
            GetEngineContext()->fileSystem->CreateDirectory(cubeDirectory, true);
            
            ScopedPtr<Image> image(Image::Create(16, 16, PixelFormat::FORMAT_RGBA8888));
            facePathnames = Vector<FilePath>
            {
                cubeDirectory + "texture_px.png",
                cubeDirectory + "texture_nx.png",
                cubeDirectory + "texture_py.tga",
                cubeDirectory + "texture_ny.tga",
                cubeDirectory + "texture_pz.png",
                cubeDirectory + "texture_nz.png"
            };
            
            for (const FilePath& path : facePathnames)
            {
                ImageSystem::Save(path, image.get());
            }
            
            TextureDescriptorUtils::CreateDescriptorCube(textureCubePathname, facePathnames);
        }

        { // create filelist
            ScopedPtr<File> file(File::Create(filelistPathname, File::WRITE | File::CREATE));
            if (file)
            {
                file->WriteLine(texture3DPathname.GetAbsolutePathname());
                file->WriteLine(textureCubePathname.GetAbsolutePathname());
                file->WriteLine(scenePathname.GetAbsolutePathname());
            }
        }
    }

    ~ResourceDependencyToolTest() override
    {
        DAVA::CommandLineModuleTestUtils::ClearTestFolder(projectPathname);
    }

    DAVA_TEST (SingleTextureTest)
    {
        using namespace DAVA;

        { // 3d texture
            { // download
                Vector<String> cmdLine
                {
                    "ResourceEditor",
                    "resourceDependency",
                    "--file",
                    texture3DPathname.GetAbsolutePathname(),
                    "--type",
                    "d",
                    "--outfile",
                    outfilePathname.GetAbsolutePathname()
                };

                std::unique_ptr<CommandLineModule> tool = std::make_unique<ResourceDependencyTool>(cmdLine);
                DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

                TEST_VERIFY(DependencyTestDetails::NotFoundInFile(outfilePathname, texture3DPathname.GetDirectory(), { pngPathname }) == true);
                
                GetEngineContext()->fileSystem->DeleteFile(outfilePathname);
            }

            { // convert
                Vector<String> cmdLine
                {
                    "ResourceEditor",
                    "resourceDependency",
                    "--file",
                    texture3DPathname.GetAbsolutePathname(),
                    "--type",
                    "c",
                    "--outfile",
                    outfilePathname.GetAbsolutePathname()
                };

                std::unique_ptr<CommandLineModule> tool = std::make_unique<ResourceDependencyTool>(cmdLine);
                DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

                TEST_VERIFY(DependencyTestDetails::FoundInFile(outfilePathname, texture3DPathname.GetDirectory(), { pngPathname }) == true);

                GetEngineContext()->fileSystem->DeleteFile(outfilePathname);
            }
        }

        { // Cube texture
            { // download
                Vector<String> cmdLine
                {
                    "ResourceEditor",
                    "resourceDependency",
                    "--file",
                    textureCubePathname.GetAbsolutePathname(),
                    "--type",
                    "d",
                    "--outfile",
                    outfilePathname.GetAbsolutePathname()
                };

                std::unique_ptr<CommandLineModule> tool = std::make_unique<ResourceDependencyTool>(cmdLine);
                DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

                TEST_VERIFY(DependencyTestDetails::NotFoundInFile(outfilePathname, textureCubePathname.GetDirectory(), facePathnames) == true);
            
                GetEngineContext()->fileSystem->DeleteFile(outfilePathname);
            }

            { // convert
                Vector<String> cmdLine
                {
                    "ResourceEditor",
                    "resourceDependency",
                    "--file",
                    textureCubePathname.GetAbsolutePathname(),
                    "--type",
                    "c",
                    "--outfile",
                    outfilePathname.GetAbsolutePathname()
                };

                std::unique_ptr<CommandLineModule> tool = std::make_unique<ResourceDependencyTool>(cmdLine);
                DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

                TEST_VERIFY(DependencyTestDetails::FoundInFile(outfilePathname, textureCubePathname.GetDirectory(), facePathnames) == true);

                GetEngineContext()->fileSystem->DeleteFile(outfilePathname);
            }
        }
    }

    DAVA_TEST (SingleSceneTest)
    {
        using namespace DAVA;

        { // download
            Vector<String> cmdLine
            {
                "ResourceEditor",
                "resourceDependency",
                "--file",
                scenePathname.GetAbsolutePathname(),
                "--type",
                "d",
                "--outfile",
                outfilePathname.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<ResourceDependencyTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(DependencyTestDetails::FoundInFile(outfilePathname, scenePathname.GetDirectory(), textures) == true);
            TEST_VERIFY(DependencyTestDetails::FoundInFile(outfilePathname, scenePathname.GetDirectory(), heightmaps) == true);

            GetEngineContext()->fileSystem->DeleteFile(outfilePathname);
        }

        { // convert
            Vector<String> cmdLine
            {
                "ResourceEditor",
                "resourceDependency",
                "--file",
                scenePathname.GetAbsolutePathname(),
                "--type",
                "c",
                "--outfile",
                outfilePathname.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<ResourceDependencyTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());
            
            TEST_VERIFY(DependencyTestDetails::NotFoundInFile(outfilePathname, scenePathname.GetDirectory(), textures) == true);
            TEST_VERIFY(DependencyTestDetails::NotFoundInFile(outfilePathname, scenePathname.GetDirectory(), heightmaps) == true);

            GetEngineContext()->fileSystem->DeleteFile(outfilePathname);
        }
    }

    DAVA_TEST (MultipleFilesTest)
    {
        using namespace DAVA;

        { // download
            Vector<String> cmdLine
            {
                "ResourceEditor",
                "resourceDependency",
                "--list",
                filelistPathname.GetAbsolutePathname(),
                "--type",
                "d",
                "--outfile",
                outfilePathname.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<ResourceDependencyTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(DependencyTestDetails::NotFoundInFile(outfilePathname, texture3DPathname.GetDirectory(), { pngPathname}) == true);
            TEST_VERIFY(DependencyTestDetails::NotFoundInFile(outfilePathname, textureCubePathname.GetDirectory(), facePathnames) == true);

            TEST_VERIFY(DependencyTestDetails::FoundInFile(outfilePathname, scenePathname.GetDirectory(), textures) == true);
            TEST_VERIFY(DependencyTestDetails::FoundInFile(outfilePathname, scenePathname.GetDirectory(), heightmaps) == true);
            
            GetEngineContext()->fileSystem->DeleteFile(outfilePathname);
        }

        { // convert
            Vector<String> cmdLine
            {
                "ResourceEditor",
                "resourceDependency",
                "--list",
                filelistPathname.GetAbsolutePathname(),
                "--type",
                "c",
                "--outfile",
                outfilePathname.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<ResourceDependencyTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(DependencyTestDetails::FoundInFile(outfilePathname, texture3DPathname.GetDirectory(), { pngPathname}) == true);
            TEST_VERIFY(DependencyTestDetails::FoundInFile(outfilePathname, textureCubePathname.GetDirectory(), facePathnames) == true);

            TEST_VERIFY(DependencyTestDetails::NotFoundInFile(outfilePathname, scenePathname.GetDirectory(), textures) == true);
            TEST_VERIFY(DependencyTestDetails::NotFoundInFile(outfilePathname, scenePathname.GetDirectory(), heightmaps) == true);

            GetEngineContext()->fileSystem->DeleteFile(outfilePathname);
        }
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("ResourceDependencyTool.cpp")
    END_FILES_COVERED_BY_TESTS();
    
};

// clang-format on
