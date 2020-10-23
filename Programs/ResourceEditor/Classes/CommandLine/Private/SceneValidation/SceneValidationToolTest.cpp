#include "Classes/CommandLine/SceneValidationTool.h"

#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>
#include <REPlatform/Scene/SceneHelper.h>

#include <TArc/Testing/ConsoleModuleTestExecution.h>
#include <TArc/Testing/TArcUnitTests.h>

#include <TextureCompression/TextureConverter.h>

#include <Base/BaseTypes.h>
#include <Engine/Engine.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/FileSystem.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Render/TextureDescriptor.h>
#include <Render/Material/NMaterial.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/CustomPropertiesComponent.h>
#include <Scene3D/Components/TransformComponent.h>

DAVA_TARC_TESTCLASS(SceneValidationToolTest)
{
    DAVA::String projectStr = "~doc:/Test/SceneValidationTool/";
    DAVA::String scenePathnameStr = projectStr + "DataSource/3d/Scene/testScene.sc2";
    std::unique_ptr<DAVA::CommandLineModuleTestUtils::TextureLoadingGuard> guard;

    void SetUp(const DAVA::String& testName) override
    {
        guard = DAVA::CommandLineModuleTestUtils::CreateTextureGuard({ DAVA::eGPUFamily::GPU_ORIGIN });
        DAVA::CommandLineModuleTestUtils::CreateProjectInfrastructure(projectStr);
    }

    void TearDown(const DAVA::String& testName) override
    {
        guard.reset();
        DAVA::CommandLineModuleTestUtils::ClearTestFolder(projectStr);
    }

    DAVA_TEST (CorrectMatricesTest)
    {
        using namespace DAVA;

        {
            DAVA::CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            builder.AddBox(DAVA::CommandLineModuleTestUtils::SceneBuilder::WITH_REF_TO_OWNER);
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "matrices" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() == 0);
    }

    DAVA_TEST (WrongMatricesTest)
    {
        using namespace DAVA;

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            Entity* box = builder.AddBox(CommandLineModuleTestUtils::SceneBuilder::WITHOUT_REF_TO_OWNER);
            Matrix4 notIdentityMatrix;
            notIdentityMatrix.Zero();
            TransformComponent* tc = box->GetComponent<TransformComponent>();
            tc->SetLocalMatrix(notIdentityMatrix); // specifying non-identity matrix: verification should fail
            builder.AddR2O(box);
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "matrices" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() != 0);
    }

    DAVA_TEST (CorrectSameNamesTest)
    {
        using namespace DAVA;

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            builder.AddBox();
            builder.AddBox();
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "sameNames" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() == 0);
    }

    DAVA_TEST (WrongSameNamesTest)
    {
        using namespace DAVA;

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            Entity* box1 = builder.AddBox();
            Entity* box2 = builder.AddBox();

            box1->AddComponent(new CustomPropertiesComponent);
            box2->AddComponent(new CustomPropertiesComponent);
            KeyedArchive* properties1 = GetCustomPropertiesArchieve(box1);
            KeyedArchive* properties2 = GetCustomPropertiesArchieve(box2);
            properties1->SetInt32("CollisionType", 1);
            properties2->SetInt32("CollisionType", 2); // collision type values are different: verification should be failed
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "sameNames" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() != 0);
    }

    DAVA_TEST (CorrectCollisionsTest)
    {
        using namespace DAVA;

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            Entity* box1 = builder.AddBox();

            box1->AddComponent(new CustomPropertiesComponent);
            KeyedArchive* properties1 = GetCustomPropertiesArchieve(box1);
            properties1->SetInt32("CollisionType", 1);
            properties1->SetInt32("MaterialKind", 1);
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "collisionTypes" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() == 0);
    }

    DAVA_TEST (WrongCollisionsTest)
    {
        using namespace DAVA;

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            Entity* box1 = builder.AddBox();

            box1->AddComponent(new CustomPropertiesComponent);
            KeyedArchive* properties1 = GetCustomPropertiesArchieve(box1);
            properties1->SetInt32("CollisionType", 1);
            // no MaterialKind or FallType was specified: verification should be failed
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "collisionTypes" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() != 0);
    }

    DAVA_TEST (CorrectRelevanceTest)
    {
        using namespace DAVA;

        auto ConvertTextures = [](Scene* scene)
        {
            SceneHelper::TextureCollector collector;
            SceneHelper::EnumerateSceneTextures(scene, collector);
            TexturesMap& texturesMap = collector.GetTextures();

            for (const std::pair<FilePath, Texture*>& entry : texturesMap)
            {
                DAVA::TextureDescriptor* descriptor = entry.second->texDescriptor;
                if (nullptr != descriptor && DAVA::FileSystem::Instance()->Exists(descriptor->pathname))
                {
                    for (uint32 i = 0; i < eGPUFamily::GPU_DEVICE_COUNT; ++i)
                    {
                        eGPUFamily gpu = static_cast<eGPUFamily>(i);
                        if (descriptor->HasCompressionFor(gpu))
                        {
                            DAVA::TextureConverter::ConvertTexture(*descriptor, gpu, true, DAVA::TextureConverter::eConvertQuality::ECQ_FASTEST);
                        }
                    }
                }
            }
        };

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            builder.AddBox();
            ConvertTextures(builder.scene);
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "texturesRelevance" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() == 0);
    }

    DAVA_TEST (WrongRelevanceTest)
    {
        using namespace DAVA;

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            builder.AddBox();
            // textures were not converted: verification should be failed
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "texturesRelevance" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() != 0);
    }

    DAVA_TEST (CorrectMaterialsTest)
    {
        using namespace DAVA;

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            builder.AddBox();

            Set<NMaterial*> materials;
            SceneHelper::BuildMaterialList(builder.scene, materials);

            NMaterial* material = *(materials.begin());
            material->SetFXName(FastName("~res:/Materials/TextureLightmapAllQualities.Opaque.material"));

            material->SetQualityGroup(FastName("Particle"));
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "materialGroups" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() == 0);
    }

    DAVA_TEST (WrongMaterialsTest)
    {
        using namespace DAVA;

        {
            CommandLineModuleTestUtils::SceneBuilder builder(scenePathnameStr, projectStr);
            builder.AddBox();

            Set<NMaterial*> materials;
            SceneHelper::BuildMaterialList(builder.scene, materials);

            NMaterial* material = *(materials.begin());
            material->SetFXName(FastName("~res:/Materials/TextureLightmapAllQualities.Opaque.material"));
            material->SetQualityGroup(FastName()); // no quality group was specified for FX material above: verification should be failed
        }

        Vector<String> cmdLine = { "ResourceEditor", "-scenevalidation", "-scene", scenePathnameStr, "-validate", "materialGroups" };

        SceneValidationTool tool(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(&tool, GetAccessor());

        TEST_VERIFY(tool.GetExitCode() != 0);
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("SceneValidationTool.cpp")
    END_FILES_COVERED_BY_TESTS();
};
