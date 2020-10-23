#include "Classes/CommandLine/SceneExporterTool.h"
#include "Classes/CommandLine/Private/CommandLineModuleTestUtils.h"

#include <TArc/Testing/ConsoleModuleTestExecution.h>
#include <TArc/Testing/TArcUnitTests.h>

#include <Base/BaseTypes.h>
#include <Engine/Engine.h>
#include <FileSystem/YamlNode.h>
#include <FileSystem/YamlParser.h>
#include <FileSystem/YamlEmitter.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/FileList.h>
#include <Render/GPUFamilyDescriptor.h>
#include <Render/TextureDescriptor.h>
#include <Entity/ComponentManager.h>
#include <Engine/Engine.h>

#include <functional>

namespace SETestDetail
{
const DAVA::String projectStr = "~doc:/Test/SceneExporterTool/";
const DAVA::String scenePathnameStr = projectStr + "DataSource/3d/Scene/testScene.sc2";
const DAVA::String dataiOSStr = projectStr + "iOS/Data/3d/";
const DAVA::String dataAndroidStr = projectStr + "android/Data/3d/";
}

DAVA_TARC_TESTCLASS(SceneExporterToolTest)
{
    void TestExportedScene(const DAVA::FilePath& scenePathname)
    {
        using namespace DAVA;

        ScopedPtr<Scene> scene(new Scene());
        TEST_VERIFY(scene->LoadScene(scenePathname) == DAVA::SceneFileV2::eError::ERROR_NO_ERROR);

        ComponentManager* cm = GetEngineContext()->componentManager;
        const Vector<const Type*> componentTypes = cm->GetRegisteredSceneComponents();

        for (const Type* type : componentTypes)
        { //test that RE specific components were removed
            const ReflectedType* refType = ReflectedTypeDB::GetByType(type);

            DVASSERT(refType != nullptr);

            ReflectedMeta* meta = refType->GetStructure()->meta.get();

            if (meta == nullptr || meta->GetMeta<M::NonExportableComponent>() == nullptr)
            {
                continue;
            }

            Vector<Entity*> entities;
            scene->GetChildEntitiesWithComponent(entities, type);
            TEST_VERIFY(entities.empty() == true);
        }
    }

    void TestExportedTextures(const DAVA::FilePath& folder, const DAVA::Vector<DAVA::eGPUFamily>& gpuForTest, bool useHD)
    {
        using namespace DAVA;

        FileSystem* fs = GetEngineContext()->fileSystem;

        ScopedPtr<FileList> fileList(new FileList(folder));
        for (int32 i = 0, count = fileList->GetCount(); i < count; ++i)
        {
            const FilePath& pathname = fileList->GetPathname(i);
            if (fileList->IsDirectory(i) && !fileList->IsNavigationDirectory(i))
            {
                TestExportedTextures(pathname, gpuForTest, useHD);
            }
            else if (pathname.IsEqualToExtension(".tex"))
            {
                std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(pathname));
                if (descriptor)
                {
                    for (eGPUFamily gpu : gpuForTest)
                    {
                        Vector<FilePath> pathes;
                        descriptor->CreateLoadPathnamesForGPU(gpu, pathes);

                        TEST_VERIFY(pathes.size() == ((useHD) ? 2 : 1));
                        for (const FilePath& path : pathes)
                        {
                            TEST_VERIFY(fs->Exists(path));
                        }
                    }
                }
                else
                {
                    TEST_VERIFY(false);
                }
            }
        }
    }

    DAVA::FilePath FindTexturePathname(const DAVA::FilePath& folder)
    {
        using namespace DAVA;

        ScopedPtr<FileList> fileList(new FileList(folder));
        for (int32 i = 0, count = fileList->GetCount(); i < count; ++i)
        {
            FilePath pathname = fileList->GetPathname(i);
            if (fileList->IsDirectory(i) && !fileList->IsNavigationDirectory(i))
            {
                pathname = FindTexturePathname(pathname);
            }

            if (pathname.IsEqualToExtension(".tex"))
            {
                return pathname;
            }
        }

        return FilePath();
    }

    bool CreateOutputConfig(const DAVA::FilePath& yamlConfig)
    {
        using namespace DAVA;

        ScopedPtr<YamlNode> rootNode(YamlNode::CreateArrayNode(YamlNode::AR_BLOCK_REPRESENTATION));

        {
            YamlNode* iosNode = YamlNode::CreateMapNode(false);
            iosNode->Set(String("outdir"), SETestDetail::dataiOSStr);

            YamlNode* gpuNode = YamlNode::CreateArrayNode();
            gpuNode->Add(GPUFamilyDescriptor::GetGPUName(eGPUFamily::GPU_POWERVR_IOS));

            iosNode->Add(String("gpu"), gpuNode);
            rootNode->Add(iosNode);
        }

        {
            YamlNode* androidNode = YamlNode::CreateMapNode(false);
            androidNode->Set(String("outdir"), SETestDetail::dataAndroidStr);

            YamlNode* gpuNode = YamlNode::CreateArrayNode();
            gpuNode->Add(GPUFamilyDescriptor::GetGPUName(eGPUFamily::GPU_MALI));
            gpuNode->Add(GPUFamilyDescriptor::GetGPUName(eGPUFamily::GPU_ADRENO));

            androidNode->Add(String("gpu"), gpuNode);
            androidNode->Set(String("useHD"), true);

            rootNode->Add(androidNode);
        }

        return YamlEmitter::SaveToYamlFile(yamlConfig, rootNode);
    }

    DAVA_TEST (ExportSceneTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SETestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SETestDetail::scenePathnameStr, SETestDetail::projectStr);

        FilePath dataPath = SETestDetail::projectStr + "Data/3d/";
        FilePath dataSourcePath = SETestDetail::projectStr + "DataSource/3d/";

        String sceneRelativePathname = FilePath(SETestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath);

        {
            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-sceneexporter",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-outdir",
              dataPath.GetAbsolutePathname(),
              "-processfile",
              sceneRelativePathname,
              "-gpu",
              "mali,adreno",
              "-hd"
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneExporterTool>(cmdLine);
            DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TestExportedScene(dataPath + sceneRelativePathname);
            TestExportedTextures(dataPath, { eGPUFamily::GPU_MALI, eGPUFamily::GPU_ADRENO }, true);

            CommandLineModuleTestUtils::ClearTestFolder(dataPath);
        }

        {
            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-sceneexporter",
              "-scene",
              "-indir",
              FilePath(dataSourcePath).GetAbsolutePathname(),
              "-outdir",
              FilePath(dataPath).GetAbsolutePathname(),
              "-processdir",
              "./",
              "-gpu",
              "mali,adreno",
              "-hd"
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneExporterTool>(cmdLine);
            DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TestExportedScene(dataPath + sceneRelativePathname);
            TestExportedTextures(dataPath, { eGPUFamily::GPU_MALI, eGPUFamily::GPU_ADRENO }, true);

            CommandLineModuleTestUtils::ClearTestFolder(dataPath);
        }

        CommandLineModuleTestUtils::ClearTestFolder(SETestDetail::projectStr);
    }

    DAVA_TEST (ExportTextureTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SETestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SETestDetail::scenePathnameStr, SETestDetail::projectStr);

        FileSystem* fs = GetEngineContext()->fileSystem;

        FilePath dataPath = SETestDetail::projectStr + "Data/3d/";
        FilePath dataSourcePath = SETestDetail::projectStr + "DataSource/3d/";

        {
            FilePath texturePathname = FindTexturePathname(dataSourcePath);
            String textureRelativePathname = texturePathname.GetRelativePathname(dataSourcePath);

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-sceneexporter",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-outdir",
              dataPath.GetAbsolutePathname(),
              "-processfile",
              textureRelativePathname,
              "-gpu",
              "mali,adreno",
              "-hd"
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneExporterTool>(cmdLine);
            DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(fs->Exists(dataPath + textureRelativePathname));
            TestExportedTextures(dataPath, { eGPUFamily::GPU_MALI, eGPUFamily::GPU_ADRENO }, true);

            CommandLineModuleTestUtils::ClearTestFolder(dataPath);
        }

        {
            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-sceneexporter",
              "-texture",
              "-indir",
              FilePath(dataSourcePath).GetAbsolutePathname(),
              "-outdir",
              FilePath(dataPath).GetAbsolutePathname(),
              "-processdir",
              "./",
              "-gpu",
              "mali,adreno",
              "-hd"
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneExporterTool>(cmdLine);
            DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TestExportedTextures(dataPath, { eGPUFamily::GPU_MALI, eGPUFamily::GPU_ADRENO }, true);

            CommandLineModuleTestUtils::ClearTestFolder(dataPath);
        }

        CommandLineModuleTestUtils::ClearTestFolder(SETestDetail::projectStr);
    }

    DAVA_TEST (ExportFileListTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SETestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SETestDetail::scenePathnameStr, SETestDetail::projectStr);

        FileSystem* fs = GetEngineContext()->fileSystem;

        FilePath dataPath = SETestDetail::projectStr + "Data/3d/";
        FilePath dataSourcePath = SETestDetail::projectStr + "DataSource/3d/";
        FilePath texturePathname = FindTexturePathname(dataSourcePath);

        String sceneRelativePathname = FilePath(SETestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath);
        String textureRelativePathname = texturePathname.GetRelativePathname(dataSourcePath);

        FilePath linksFilePathname = SETestDetail::projectStr + "links.txt";
        {
            ScopedPtr<File> linksFile(File::Create(linksFilePathname, File::WRITE | File::CREATE));
            if (linksFile)
            {
                linksFile->WriteLine(sceneRelativePathname);
                linksFile->WriteLine(textureRelativePathname);
            }
            else
            {
                TEST_VERIFY(false);
            }
        }

        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-sceneexporter",
          "-indir",
          dataSourcePath.GetAbsolutePathname(),
          "-outdir",
          dataPath.GetAbsolutePathname(),
          "-processfilelist",
          linksFilePathname.GetAbsolutePathname(),
          "-gpu",
          "mali,adreno",
          "-hd"
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneExporterTool>(cmdLine);
        DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        TEST_VERIFY(fs->Exists(dataPath + sceneRelativePathname));
        TEST_VERIFY(fs->Exists(dataPath + textureRelativePathname));
        TestExportedTextures(dataPath, { eGPUFamily::GPU_MALI, eGPUFamily::GPU_ADRENO }, true);

        CommandLineModuleTestUtils::ClearTestFolder(SETestDetail::projectStr);
    }

    DAVA_TEST (ExportSceneTestOutput)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SETestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SETestDetail::scenePathnameStr, SETestDetail::projectStr);

        FileSystem* fs = GetEngineContext()->fileSystem;

        FilePath dataSourcePath = SETestDetail::projectStr + "DataSource/3d/";
        FilePath configPath = SETestDetail::projectStr + "config.yaml";
        TEST_VERIFY(CreateOutputConfig(configPath));

        String sceneRelativePathname = FilePath(SETestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath);

        {
            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-sceneexporter",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-output",
              configPath.GetAbsolutePathname(),
              "-processfile",
              sceneRelativePathname,
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneExporterTool>(cmdLine);
            DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(fs->Exists(SETestDetail::dataiOSStr + sceneRelativePathname));
            TestExportedTextures(SETestDetail::dataiOSStr, { eGPUFamily::GPU_POWERVR_IOS }, false);

            TEST_VERIFY(fs->Exists(SETestDetail::dataAndroidStr + sceneRelativePathname));
            TestExportedTextures(SETestDetail::dataAndroidStr, { eGPUFamily::GPU_MALI, eGPUFamily::GPU_ADRENO }, true);
        }

        CommandLineModuleTestUtils::ClearTestFolder(SETestDetail::projectStr);
    }

    DAVA_TEST (ExportSceneTestTags)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });

        FilePath dataPath = SETestDetail::projectStr + "Data/3d/";
        FilePath dataSourcePath = SETestDetail::projectStr + "DataSource/3d/";

        FilePath slotYamlPath = SETestDetail::projectStr + "Data/Slot.yaml";
        FilePath sourceDefaultSlotYamlPath = SETestDetail::projectStr + "DataSource/Slot.yaml";
        FilePath sourceChineseSlotYamlPath = SETestDetail::projectStr + "DataSource/Slot.china.yaml";

        auto getRelativePath = Bind(CommandLineModuleTestUtils::SceneBuilder::GetSceneRelativePathname,
                                    SETestDetail::scenePathnameStr,
                                    dataSourcePath,
                                    std::placeholders::_1);

        FileSystem* fs = GetEngineContext()->fileSystem;

        String defaultSlotDirPath = getRelativePath(CommandLineModuleTestUtils::SceneBuilder::defaultSlotDir);
        String chinaSlotDirPath = getRelativePath(CommandLineModuleTestUtils::SceneBuilder::chinaSlotDir);
        String boxTexture = getRelativePath("box.png");
        String chineseBoxTexture = getRelativePath("box.china.png");

        String sceneRelativePathname = FilePath(SETestDetail::scenePathnameStr).GetRelativePathname(dataSourcePath);

        auto testSlotDirContents = [&fs, &dataPath](const String& sourceSlotPath)
        {
            String boxSlotName = "box_slot";
            Vector<String> extensions{ ".sc2", ".png", ".tex" };
            for (const String& ext : extensions)
            {
                String filePath = sourceSlotPath + boxSlotName + ext;
                TEST_VERIFY(fs->Exists(dataPath + filePath) == true);
            }
        };

        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(SETestDetail::projectStr);
            CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SETestDetail::scenePathnameStr, SETestDetail::projectStr);

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-sceneexporter",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-outdir",
              dataPath.GetAbsolutePathname(),
              "-processfile",
              sceneRelativePathname,
              "-gpu",
              "origin",
              "-tag",
              CommandLineModuleTestUtils::SceneBuilder::tagChina
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneExporterTool>(cmdLine);
            DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(fs->Exists(dataPath + chinaSlotDirPath) == true);
            TEST_VERIFY(fs->Exists(dataPath + defaultSlotDirPath) == false);

            TEST_VERIFY(fs->CompareTextFiles(slotYamlPath, sourceChineseSlotYamlPath) == true);
            TEST_VERIFY(fs->CompareTextFiles(slotYamlPath, sourceDefaultSlotYamlPath) == false);
            testSlotDirContents(chinaSlotDirPath);

            TEST_VERIFY(fs->CompareBinaryFiles(dataPath + boxTexture, dataSourcePath + chineseBoxTexture) == true);
            TEST_VERIFY(fs->CompareBinaryFiles(dataPath + boxTexture, dataSourcePath + boxTexture) == false);

            CommandLineModuleTestUtils::ClearTestFolder(SETestDetail::projectStr);
        }

        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(SETestDetail::projectStr);
            CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SETestDetail::scenePathnameStr, SETestDetail::projectStr);

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-sceneexporter",
              "-indir",
              dataSourcePath.GetAbsolutePathname(),
              "-outdir",
              dataPath.GetAbsolutePathname(),
              "-processfile",
              sceneRelativePathname,
              "-gpu",
              "origin"
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneExporterTool>(cmdLine);
            DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TEST_VERIFY(fs->Exists(dataPath + chinaSlotDirPath) == false);
            TEST_VERIFY(fs->Exists(dataPath + defaultSlotDirPath) == true);

            TEST_VERIFY(fs->CompareTextFiles(slotYamlPath, sourceChineseSlotYamlPath) == false);
            TEST_VERIFY(fs->CompareTextFiles(slotYamlPath, sourceDefaultSlotYamlPath) == true);
            testSlotDirContents(defaultSlotDirPath);

            TEST_VERIFY(fs->CompareBinaryFiles(dataPath + boxTexture, dataSourcePath + chineseBoxTexture) == false);
            TEST_VERIFY(fs->CompareBinaryFiles(dataPath + boxTexture, dataSourcePath + boxTexture) == true);

            CommandLineModuleTestUtils::ClearTestFolder(SETestDetail::projectStr);
        }

        CommandLineModuleTestUtils::ClearTestFolder(SETestDetail::projectStr);
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("SceneExporterTool.cpp")
    END_FILES_COVERED_BY_TESTS();
}
;
