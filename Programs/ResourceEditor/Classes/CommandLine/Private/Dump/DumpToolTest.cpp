#include "Classes/CommandLine/DumpTool.h"

#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>
#include <REPlatform/CommandLine/SceneConsoleHelper.h>
#include <REPlatform/Scene/Utils/SceneDumper.h>

#include <TArc/Testing/ConsoleModuleTestExecution.h>
#include <TArc/Testing/TArcUnitTests.h>

#include <Base/ScopedPtr.h>
#include <FileSystem/FileSystem.h>
#include <Render/Highlevel/Landscape.h>
#include <Render/Highlevel/RenderObject.h>
#include <Render/Highlevel/Vegetation/VegetationRenderObject.h>
#include <Render/TextureDescriptor.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/SlotComponent.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>

#include <memory>

namespace DTestDetail
{
const DAVA::String projectStr = "~doc:/Test/DumpTool/";
const DAVA::String scenePathnameStr = projectStr + "DataSource/3d/Scene/testScene.sc2";
const DAVA::String anotherScenePathnameStr = projectStr + "DataSource/3d/AnotherScene/testScene.sc2";
const DAVA::String linksStr = "~doc:/Test/DumpTool/links.txt";

DAVA::Set<DAVA::String> ReadLinks(const DAVA::String& linksStr)
{
    using namespace DAVA;

    Set<String> dumpedLinks;
    ScopedPtr<File> linksFile(File::Create(linksStr, File::READ | File::OPEN));
    if (linksFile)
    {
        do
        {
            String link = linksFile->ReadLine();
            if (!link.empty())
            {
                dumpedLinks.insert(link);
            }

        } while (!linksFile->IsEof());
    }
    return dumpedLinks;
}
}

DAVA_TARC_TESTCLASS(DumpToolTest)
{
    void TestTags(const DAVA::Vector<DAVA::String>& tagList)
    {
        using namespace DAVA;

        Set<String> dumpedLinks = DTestDetail::ReadLinks(DTestDetail::linksStr);
        TEST_VERIFY(dumpedLinks.empty() == false);

        auto it = std::find_if(std::begin(dumpedLinks), std::end(dumpedLinks), [](const String& link) {
            return link.find(CommandLineModuleTestUtils::SceneBuilder::tagJapan) != String::npos;
        });

        if (std::find(std::begin(tagList), std::end(tagList), CommandLineModuleTestUtils::SceneBuilder::tagJapan)
            == std::end(tagList))
        {
            TEST_VERIFY(it == std::end(dumpedLinks));
        }
        else
        {
            TEST_VERIFY(it != std::end(dumpedLinks));
        }

        it = std::find_if(std::begin(dumpedLinks), std::end(dumpedLinks), [](const String& link) {
            return link.find("China.slot/box_slot.sc2") != String::npos;
        });
        TEST_VERIFY(it != std::end(dumpedLinks));

        ScopedPtr<Scene> scene(new Scene());
        TEST_VERIFY(scene->LoadScene(DTestDetail::scenePathnameStr) == DAVA::SceneFileV2::eError::ERROR_NO_ERROR);

        auto pathInLinks = [&dumpedLinks](const FilePath& path, const String& tag) -> bool
        {
            FilePath taggedPath = path;
            taggedPath.ReplaceBasename(taggedPath.GetBasename() + tag);
            auto it = std::find(std::begin(dumpedLinks), std::end(dumpedLinks), taggedPath.GetAbsolutePathname());
            return it != std::end(dumpedLinks);
        };

        uint32 entityCount = scene->GetChildrenCount();
        for (uint32 e = 0; e < entityCount; ++e)
        {
            Entity* child = scene->GetChild(e);

            RenderObject* ro = GetRenderObject(child);
            if (ro == nullptr)
                continue;

            if (ro->GetType() != RenderObject::TYPE_MESH)
                continue;

            SlotComponent* comp = child->GetComponent<SlotComponent>();
            if (comp == nullptr)
                continue;

            for (const String& tag : tagList)
            {
                if (tag == CommandLineModuleTestUtils::SceneBuilder::tagJapan)
                    continue;
                TEST_VERIFY(pathInLinks(comp->GetConfigFilePath(), tag) == true);
            }

            uint32 rbCount = ro->GetRenderBatchCount();
            for (uint32 r = 0; r < rbCount; ++r)
            {
                RenderBatch* rb = ro->GetRenderBatch(r);
                TEST_VERIFY(rb != nullptr);

                NMaterial* mat = rb->GetMaterial();
                if (mat == nullptr)
                    continue;

                const UnorderedMap<FastName, MaterialTextureInfo*>& textures = mat->GetLocalTextures();
                for (auto& tx : textures)
                {
                    if (tx.first == FastName("heightmap"))
                        continue;

                    TEST_VERIFY(dumpedLinks.count(tx.second->path.GetAbsolutePathname()) == 1);

                    std::unique_ptr<TextureDescriptor> texDescriptor(TextureDescriptor::CreateFromFile(tx.second->path));
                    TEST_VERIFY(texDescriptor != nullptr);

                    Vector<FilePath> pathes;
                    texDescriptor->CreateLoadPathnamesForGPU(eGPUFamily::GPU_ORIGIN, pathes);
                    for (const FilePath& p : pathes)
                    {
                        TEST_VERIFY(dumpedLinks.count(p.GetAbsolutePathname()) == 1);
                        for (const String& tag : tagList)
                        {
                            TEST_VERIFY(pathInLinks(p, tag) == true);
                        }
                    }
                }
            }
        }
    }

    void TestLinks(DAVA::SceneDumper::eMode mode,
                   const DAVA::Vector<DAVA::eGPUFamily>& compressedGPUs,
                   const DAVA::String& scenePathnameStr,
                   const DAVA::FilePath& outDir = "")
    {
        using namespace DAVA;
        Set<String> dumpedLinks;
        if (outDir.IsEmpty() == true)
        {
            dumpedLinks = DTestDetail::ReadLinks(DTestDetail::linksStr);
        }
        else
        {
            DAVA::FilePath dumpPath = scenePathnameStr;
            dumpPath.ReplaceExtension(".dump");
            DAVA::FilePath linksPath =
            outDir + dumpPath.GetRelativePathname(DTestDetail::projectStr + "/DataSource/");
            dumpedLinks = DTestDetail::ReadLinks(linksPath.GetAbsolutePathname());
        }
        TEST_VERIFY(dumpedLinks.empty() == false);

        ScopedPtr<Scene> scene(new Scene());
        TEST_VERIFY(scene->LoadScene(scenePathnameStr) == DAVA::SceneFileV2::eError::ERROR_NO_ERROR);

        auto testMaterial = [&dumpedLinks, &compressedGPUs](NMaterial* mat)
        {
            if (mat == nullptr)
                return;

            const UnorderedMap<FastName, MaterialTextureInfo*>& textures = mat->GetLocalTextures();
            for (auto& tx : textures)
            {
                if (tx.first == FastName("heightmap"))
                    continue;

                TEST_VERIFY(dumpedLinks.count(tx.second->path.GetAbsolutePathname()) == 1);

                std::unique_ptr<TextureDescriptor> texDescriptor(TextureDescriptor::CreateFromFile(tx.second->path));
                TEST_VERIFY(texDescriptor != nullptr);

                for (eGPUFamily gpu : compressedGPUs)
                {
                    Vector<FilePath> pathes;
                    texDescriptor->CreateLoadPathnamesForGPU(gpu, pathes);
                    for (const FilePath& p : pathes)
                    {
                        TEST_VERIFY(dumpedLinks.count(p.GetAbsolutePathname()) == 1);
                    }
                }
            }
        };

        uint32 entityCount = scene->GetChildrenCount();
        for (uint32 e = 0; e < entityCount; ++e)
        {
            Entity* child = scene->GetChild(e);

            //test reference to owner
            String ownerName = child->GetName().c_str() + String(".sc2");

            auto it = std::find_if(dumpedLinks.begin(), dumpedLinks.end(), [&ownerName](const String& str) {
                return str.find(ownerName) != String::npos;
            });

            TEST_VERIFY((it == dumpedLinks.end()) == (mode == SceneDumper::eMode::REQUIRED));

            RenderObject* ro = GetRenderObject(child);
            if (ro == nullptr)
                continue;

            if (ro->GetType() == RenderObject::TYPE_LANDSCAPE)
            {
                Landscape* landscape = static_cast<Landscape*>(ro);
                testMaterial(landscape->GetMaterial());

                TEST_VERIFY(dumpedLinks.count(landscape->GetHeightmapPathname().GetAbsolutePathname()) == 1);
            }
            else if (ro->GetType() == RenderObject::TYPE_VEGETATION)
            {
                VegetationRenderObject* vegetation = static_cast<VegetationRenderObject*>(ro);

                const size_t expectedCount = (mode == SceneDumper::eMode::REQUIRED) ? 0 : 1;
                TEST_VERIFY(dumpedLinks.count(vegetation->GetCustomGeometryPath().GetAbsolutePathname()) == expectedCount);
                TEST_VERIFY(dumpedLinks.count(vegetation->GetLightmapPath().GetAbsolutePathname()) == 1);
            }
            else
            {
                uint32 rbCount = ro->GetRenderBatchCount();
                for (uint32 r = 0; r < rbCount; ++r)
                {
                    RenderBatch* rb = ro->GetRenderBatch(r);
                    TEST_VERIFY(rb != nullptr);
                    testMaterial(rb->GetMaterial());
                }
            }
        }
    }

    DAVA_TEST (DumpFileExtended)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(DTestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(DTestDetail::scenePathnameStr, DTestDetail::projectStr);

        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-dump",
          "-links",
          "-indir",
          FilePath(DTestDetail::scenePathnameStr).GetDirectory().GetAbsolutePathname(),
          "-processfile",
          FilePath(DTestDetail::scenePathnameStr).GetFilename(),
          "-outfile",
          FilePath(DTestDetail::linksStr).GetAbsolutePathname(),
          "-mode",
          "e",
          "-gpu",
          "all"
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<DumpTool>(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        TestLinks(SceneDumper::eMode::EXTENDED, { GPU_POWERVR_IOS, GPU_POWERVR_ANDROID, GPU_TEGRA, GPU_MALI, GPU_ADRENO, GPU_DX11 },
                  DTestDetail::scenePathnameStr);

        CommandLineModuleTestUtils::ClearTestFolder(DTestDetail::projectStr);
    }

    DAVA_TEST (DumpFileRequired)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(DTestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(DTestDetail::scenePathnameStr, DTestDetail::projectStr);

        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-dump",
          "-links",
          "-indir",
          FilePath(DTestDetail::scenePathnameStr).GetDirectory().GetAbsolutePathname(),
          "-processfile",
          FilePath(DTestDetail::scenePathnameStr).GetFilename(),
          "-outfile",
          FilePath(DTestDetail::linksStr).GetAbsolutePathname(),
          "-mode",
          "r",
          "-gpu",
          "mali,tegra"
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<DumpTool>(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        TestLinks(SceneDumper::eMode::REQUIRED, { eGPUFamily::GPU_MALI, eGPUFamily::GPU_TEGRA },
                  DTestDetail::scenePathnameStr);

        CommandLineModuleTestUtils::ClearTestFolder(DTestDetail::projectStr);
    }

    DAVA_TEST (FileSystemTagsTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(DTestDetail::projectStr);
            CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(DTestDetail::scenePathnameStr, DTestDetail::projectStr);

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-dump",
              "-links",
              "-indir",
              FilePath(DTestDetail::scenePathnameStr).GetDirectory().GetAbsolutePathname(),
              "-processfile",
              FilePath(DTestDetail::scenePathnameStr).GetFilename(),
              "-outfile",
              FilePath(DTestDetail::linksStr).GetAbsolutePathname(),
              "-gpu",
              "origin",
              "-taglist",
              CommandLineModuleTestUtils::SceneBuilder::tagChina
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<DumpTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TestTags({ CommandLineModuleTestUtils::SceneBuilder::tagChina });

            CommandLineModuleTestUtils::ClearTestFolder(DTestDetail::projectStr);
        }

        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(DTestDetail::projectStr);
            CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(DTestDetail::scenePathnameStr, DTestDetail::projectStr);

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-dump",
              "-links",
              "-indir",
              FilePath(DTestDetail::scenePathnameStr).GetDirectory().GetAbsolutePathname(),
              "-processfile",
              FilePath(DTestDetail::scenePathnameStr).GetFilename(),
              "-outfile",
              FilePath(DTestDetail::linksStr).GetAbsolutePathname(),
              "-gpu",
              "origin",
              "-taglist",
              CommandLineModuleTestUtils::SceneBuilder::tagChina + ","
              + CommandLineModuleTestUtils::SceneBuilder::tagJapan
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<DumpTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            TestTags({ CommandLineModuleTestUtils::SceneBuilder::tagChina,
                       CommandLineModuleTestUtils::SceneBuilder::tagJapan });

            CommandLineModuleTestUtils::ClearTestFolder(DTestDetail::projectStr);
        }
    }

    DAVA_TEST (FileListTest)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });

        CommandLineModuleTestUtils::CreateProjectInfrastructure(DTestDetail::projectStr);

        DAVA::FilePath qualityPath = DTestDetail::projectStr + "/quality.yaml";
        TEST_VERIFY(qualityPath == SceneConsoleHelper::CreateQualityPathname(qualityPath));

        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(DTestDetail::scenePathnameStr, DTestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(DTestDetail::anotherScenePathnameStr, DTestDetail::projectStr);

        FilePath fileListFilePath = DTestDetail::projectStr + "scenePaths.txt";
        {
            ScopedPtr<File> fileListFile(File::Create(fileListFilePath, File::WRITE | File::CREATE));
            TEST_VERIFY(fileListFile.get() != nullptr);

            DAVA::FilePath dataSource = DTestDetail::projectStr + "/DataSource/";
            fileListFile->WriteLine(FilePath(DTestDetail::scenePathnameStr).GetRelativePathname(dataSource));
            fileListFile->WriteLine(FilePath(DTestDetail::anotherScenePathnameStr).GetRelativePathname(dataSource));
        }

        DAVA::FilePath outDir = FilePath(DTestDetail::projectStr) + "/Data/";
        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-dump",
          "-links",
          "-processfilelist",
          fileListFilePath.GetAbsolutePathname(),
          "-indir",
          DTestDetail::projectStr + "/DataSource/",
          "-outdir",
          outDir.GetAbsolutePathname(),
          "-gpu",
          "origin",
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<DumpTool>(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        TestLinks(SceneDumper::eMode::EXTENDED, { eGPUFamily::GPU_ORIGIN },
                  DTestDetail::scenePathnameStr, outDir);
        TestLinks(SceneDumper::eMode::EXTENDED, { eGPUFamily::GPU_ORIGIN },
                  DTestDetail::anotherScenePathnameStr, outDir);

        CommandLineModuleTestUtils::ClearTestFolder(DTestDetail::projectStr);
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("DumpTool.cpp")
    END_FILES_COVERED_BY_TESTS();
};
