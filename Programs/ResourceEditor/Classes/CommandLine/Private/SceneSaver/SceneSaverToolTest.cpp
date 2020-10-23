#include "Classes/CommandLine/SceneSaverTool.h"

#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>

#include <TArc/Testing/ConsoleModuleTestExecution.h>
#include <TArc/Testing/TArcUnitTests.h>

#include <Base/BaseTypes.h>
#include <Engine/Engine.h>
#include <FileSystem/FileList.h>
#include <Render/TextureDescriptor.h>

namespace SSTestDetail
{
const DAVA::String projectStr = "~doc:/Test/SceneSaverTool/";
const DAVA::String scenePathnameStr = projectStr + "DataSource/3d/Scene/testScene.sc2";
const DAVA::String newProjectStr = "~doc:/Test/SceneSaverTool_new/";
const DAVA::String newScenePathnameStr = newProjectStr + "DataSource/3d/Scene/testScene.sc2";
}

DAVA_TARC_TESTCLASS(SceneSaverToolTest)
{
    void EnumerateDescriptors(const DAVA::FilePath& folder, DAVA::List<DAVA::FilePath>& pathes)
    {
        using namespace DAVA;

        ScopedPtr<FileList> fileList(new FileList(folder));
        for (uint32 i = 0; i < fileList->GetCount(); ++i)
        {
            const FilePath& pathname = fileList->GetPathname(i);
            if (fileList->IsDirectory(i) && !fileList->IsNavigationDirectory(i))
            {
                EnumerateDescriptors(pathname, pathes);
            }
            else if (pathname.IsEqualToExtension(".tex"))
            {
                pathes.push_back(pathname);
            }
        }
    }

    bool CreateDummyCompressedFiles(const DAVA::FilePath& folder)
    {
        using namespace DAVA;

        List<FilePath> pathes;
        EnumerateDescriptors(folder, pathes);

        for (const FilePath& pathname : pathes)
        {
            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(pathname));
            if (descriptor)
            {
                for (int32 gpu = 0; gpu < eGPUFamily::GPU_DEVICE_COUNT; ++gpu)
                {
                    FilePath compressedPath = descriptor->CreateMultiMipPathnameForGPU(static_cast<eGPUFamily>(gpu));
                    ScopedPtr<File> dummyFile(File::Create(compressedPath, File::CREATE | File::WRITE));
                }
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    DAVA_TEST (SaveSceneTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SSTestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SSTestDetail::scenePathnameStr, SSTestDetail::projectStr);

        FileSystem* fs = GetEngineContext()->fileSystem;

        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(SSTestDetail::newProjectStr);
            FilePath dataSourcePath = SSTestDetail::projectStr + "DataSource/3d/";
            FilePath newDataSourcePath = SSTestDetail::newProjectStr + "DataSource/3d/";

            TEST_VERIFY(CreateDummyCompressedFiles(dataSourcePath));

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-scenesaver",
              "-save",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-outdir",
              newDataSourcePath.GetAbsolutePathname(),
              "-processfile",
              FilePath(SSTestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath),
              "-copyconverted"
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneSaverTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(fs->Exists(SSTestDetail::newScenePathnameStr));

            CommandLineModuleTestUtils::ClearTestFolder(SSTestDetail::newProjectStr);
        }

        CommandLineModuleTestUtils::ClearTestFolder(SSTestDetail::projectStr);
    }

    DAVA_TEST (ResaveSceneTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SSTestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SSTestDetail::scenePathnameStr, SSTestDetail::projectStr);

        FileSystem* fs = GetEngineContext()->fileSystem;

        FilePath dataSourcePath = SSTestDetail::projectStr + "DataSource/3d/";

        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-scenesaver",
          "-resave",
          "-indir",
          dataSourcePath.GetAbsolutePathname(),
          "-processfile",
          FilePath(SSTestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath),
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneSaverTool>(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        TEST_VERIFY(fs->Exists(SSTestDetail::scenePathnameStr));

        CommandLineModuleTestUtils::ClearTestFolder(SSTestDetail::projectStr);
    }

    DAVA_TEST (ResaveYamlTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SSTestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SSTestDetail::scenePathnameStr, SSTestDetail::projectStr);

        FileSystem* fs = GetEngineContext()->fileSystem;

        FilePath dataSourcePath = SSTestDetail::projectStr + "DataSource/3d/";

        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-scenesaver",
          "-resave",
          "-yaml",
          "-indir",
          FilePath(SSTestDetail::projectStr).GetAbsolutePathname() + "DataSource/",
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneSaverTool>(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        TEST_VERIFY(fs->Exists(SSTestDetail::projectStr + "DataSource/quality.yaml"));

        CommandLineModuleTestUtils::ClearTestFolder(SSTestDetail::projectStr);
    }

    DAVA_TEST (SaveSceneTagsTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SSTestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SSTestDetail::scenePathnameStr, SSTestDetail::projectStr);

        FilePath dataSourcePath = SSTestDetail::projectStr + "DataSource/3d/";
        FilePath newDataSourcePath = SSTestDetail::newProjectStr + "DataSource/3d/";

        auto getRelativePath = Bind(CommandLineModuleTestUtils::SceneBuilder::GetSceneRelativePathname,
                                    SSTestDetail::scenePathnameStr,
                                    dataSourcePath,
                                    std::placeholders::_1);

        FileSystem* fs = GetEngineContext()->fileSystem;

        String defaultSlotRelativePathname = getRelativePath(CommandLineModuleTestUtils::SceneBuilder::defaultSlotDir);
        String chinaSlotRelativePathname = getRelativePath(CommandLineModuleTestUtils::SceneBuilder::chinaSlotDir);
        String boxTexture = getRelativePath("box.png");
        String chineseBoxTexture = getRelativePath("box.china.png");
        String japaneseBoxTexture = getRelativePath("box.japan.png");

        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(SSTestDetail::newProjectStr);
            TEST_VERIFY(CreateDummyCompressedFiles(dataSourcePath));

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-scenesaver",
              "-save",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-outdir",
              newDataSourcePath.GetAbsolutePathname(),
              "-processfile",
              FilePath(SSTestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath),
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneSaverTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(fs->Exists(SSTestDetail::newScenePathnameStr));

            TEST_VERIFY(fs->Exists(newDataSourcePath + chinaSlotRelativePathname) == false);
            TEST_VERIFY(fs->Exists(newDataSourcePath + defaultSlotRelativePathname) == true);

            TEST_VERIFY(fs->Exists(newDataSourcePath + boxTexture) == true);
            TEST_VERIFY(fs->Exists(newDataSourcePath + chineseBoxTexture) == false);
            TEST_VERIFY(fs->Exists(newDataSourcePath + japaneseBoxTexture) == false);

            TEST_VERIFY(fs->CompareBinaryFiles(newDataSourcePath + boxTexture, dataSourcePath + boxTexture) == true);

            CommandLineModuleTestUtils::ClearTestFolder(SSTestDetail::newProjectStr);
        }

        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(SSTestDetail::newProjectStr);
            TEST_VERIFY(CreateDummyCompressedFiles(dataSourcePath));

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-scenesaver",
              "-save",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-outdir",
              newDataSourcePath.GetAbsolutePathname(),
              "-processfile",
              FilePath(SSTestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath),
              "-taglist",
              CommandLineModuleTestUtils::SceneBuilder::tagChina
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneSaverTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(fs->Exists(SSTestDetail::newScenePathnameStr));

            TEST_VERIFY(fs->Exists(newDataSourcePath + chinaSlotRelativePathname) == true);
            TEST_VERIFY(fs->Exists(newDataSourcePath + defaultSlotRelativePathname) == true);

            TEST_VERIFY(fs->Exists(newDataSourcePath + boxTexture) == true);
            TEST_VERIFY(fs->Exists(newDataSourcePath + chineseBoxTexture) == true);
            TEST_VERIFY(fs->Exists(newDataSourcePath + japaneseBoxTexture) == false);

            TEST_VERIFY(fs->CompareBinaryFiles(newDataSourcePath + boxTexture, dataSourcePath + boxTexture) == true);
            TEST_VERIFY(fs->CompareBinaryFiles(newDataSourcePath + chineseBoxTexture, dataSourcePath + chineseBoxTexture) == true);

            CommandLineModuleTestUtils::ClearTestFolder(SSTestDetail::newProjectStr);
        }

        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(SSTestDetail::newProjectStr);
            TEST_VERIFY(CreateDummyCompressedFiles(dataSourcePath));

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-scenesaver",
              "-save",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-outdir",
              newDataSourcePath.GetAbsolutePathname(),
              "-processfile",
              FilePath(SSTestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath),
              "-taglist",
              CommandLineModuleTestUtils::SceneBuilder::tagChina + ","
              + CommandLineModuleTestUtils::SceneBuilder::tagJapan
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneSaverTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(fs->Exists(SSTestDetail::newScenePathnameStr));

            TEST_VERIFY(fs->Exists(newDataSourcePath + chinaSlotRelativePathname) == true);
            TEST_VERIFY(fs->Exists(newDataSourcePath + defaultSlotRelativePathname) == true);

            TEST_VERIFY(fs->Exists(newDataSourcePath + boxTexture) == true);
            TEST_VERIFY(fs->Exists(newDataSourcePath + chineseBoxTexture) == true);
            TEST_VERIFY(fs->Exists(newDataSourcePath + japaneseBoxTexture) == true);

            TEST_VERIFY(fs->CompareBinaryFiles(newDataSourcePath + boxTexture, dataSourcePath + boxTexture) == true);
            TEST_VERIFY(fs->CompareBinaryFiles(newDataSourcePath + chineseBoxTexture, dataSourcePath + chineseBoxTexture) == true);
            TEST_VERIFY(fs->CompareBinaryFiles(newDataSourcePath + japaneseBoxTexture, dataSourcePath + japaneseBoxTexture) == true);

            CommandLineModuleTestUtils::ClearTestFolder(SSTestDetail::newProjectStr);
        }

        CommandLineModuleTestUtils::ClearTestFolder(SSTestDetail::projectStr);
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("SceneSaverTool.cpp")
    END_FILES_COVERED_BY_TESTS();
};
