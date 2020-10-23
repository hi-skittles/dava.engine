#include "Classes/Qt/Tools/AddSwitchEntityDialog/SwitchEntityCreator.h"

#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>
#include <REPlatform/Global/StringConstants.h>

#include <TArc/Testing/TArcUnitTests.h>

#include <Base/BaseTypes.h>
#include <Base/ScopedPtr.h>
#include <Engine/Engine.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/KeyedArchive.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/CustomPropertiesComponent.h>
#include <Scene3D/Components/SwitchComponent.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>

namespace AddSwitchTestDetails
{
const DAVA::String testFolder = DAVA::String("~doc:/Test/Switch/");
const DAVA::String testScenePath = testFolder + DAVA::String("test.sc2");
}

// clang-format off
DAVA_TARC_TESTCLASS(AddSwitchTest)
{
    void Setup()
    {
        DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
        fs->CreateDirectory(AddSwitchTestDetails::testFolder, true);
    }

    void Cleanup()
    {
        DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
        fs->DeleteDirectoryFiles(AddSwitchTestDetails::testFolder);
        fs->DeleteDirectory(AddSwitchTestDetails::testFolder);
    }

    DAVA_TEST (CreateSwitch)
    {
        using namespace DAVA;

        Setup();

        { // create scene and save it
            using namespace CommandLineModuleTestUtils;
            CreateProjectInfrastructure(AddSwitchTestDetails::testFolder);
            SceneBuilder builder(AddSwitchTestDetails::testScenePath, AddSwitchTestDetails::testFolder);
            SceneBuilder::BoxBuilder boxBuilder;

            { // without chidren
                Vector<Entity*> entities;
                Entity* simpleEntity = builder.AddWater(SceneBuilder::R2OMode::WITH_REF_TO_OWNER);
                KeyedArchive *archive = GetCustomPropertiesArchieve(simpleEntity);
                archive->SetInt32("CollisionType", 1);
                entities.push_back(simpleEntity);

                Entity* simpleEntityCrashed = builder.AddWater(SceneBuilder::WITH_REF_TO_OWNER);
                KeyedArchive *archiveCrashed = GetCustomPropertiesArchieve(simpleEntityCrashed);
                archiveCrashed->SetInt32("CollisionType", 2);
                entities.push_back(simpleEntityCrashed);

                SwitchEntityCreator creator;
                for(Entity* e: entities)
                {
                    TEST_VERIFY(creator.HasMeshRenderObjectsRecursive(e) == true);
                    TEST_VERIFY(creator.HasSwitchComponentsRecursive(e) == false);
                }

                RefPtr<Entity> switchEntity(creator.CreateSwitchEntity(entities));
                TEST_VERIFY(creator.HasSwitchComponentsRecursive(switchEntity.Get()) == true);
                builder.scene->AddNode(switchEntity.Get());

                KeyedArchive *archiveSwitch = GetCustomPropertiesArchieve(switchEntity.Get());
                TEST_VERIFY(archiveSwitch != nullptr)

                TEST_VERIFY(archiveSwitch->GetInt32("CollisionType", 0) == 1);
                TEST_VERIFY(archiveSwitch->GetInt32("CollisionTypeCrashed", 0) == 2);

                SwitchComponent* sw = GetSwitchComponent(switchEntity.Get());
                sw->SetSwitchIndex(1);
            }

            { //with children
                Vector<Entity* >entities;

                Entity* simpleEntity = builder.AddWater(SceneBuilder::R2OMode::WITH_REF_TO_OWNER);
                Entity* simpleEntityChild = builder.AddWater(SceneBuilder::R2OMode::WITH_REF_TO_OWNER);
                simpleEntity->AddNode(simpleEntityChild);
                KeyedArchive *archive = GetCustomPropertiesArchieve(simpleEntity);
                archive->SetInt32("CollisionType", 1);
                entities.push_back(simpleEntity);

                Entity* simpleEntityCrashed = builder.AddWater(SceneBuilder::WITH_REF_TO_OWNER);
                KeyedArchive *archiveCrashed = GetCustomPropertiesArchieve(simpleEntityCrashed);
                archiveCrashed->SetInt32("CollisionType", 2);
                entities.push_back(simpleEntityCrashed);

                SwitchEntityCreator creator;
                for(Entity* e: entities)
                {
                    TEST_VERIFY(creator.HasMeshRenderObjectsRecursive(e) == true);
                    TEST_VERIFY(creator.HasSwitchComponentsRecursive(e) == false);
                }

                RefPtr<Entity> switchEntity(creator.CreateSwitchEntity(entities));
                TEST_VERIFY(creator.HasSwitchComponentsRecursive(switchEntity.Get()) == true);
                builder.scene->AddNode(switchEntity.Get());
                switchEntity->SetName("switchWithoutChildren");

                KeyedArchive *archiveSwitch = GetCustomPropertiesArchieve(switchEntity.Get());
                TEST_VERIFY(archiveSwitch != nullptr)

                TEST_VERIFY(archiveSwitch->GetInt32("CollisionType", 0) == 1);

                SwitchComponent* sw = GetSwitchComponent(switchEntity.Get());
                sw->SetSwitchIndex(0);
            }
        }

        { // load scene and test it
            ScopedPtr<Scene> scene(new Scene());
            SceneFileV2::eError sceneLoadResult = scene->LoadScene(AddSwitchTestDetails::testScenePath);
            TEST_VERIFY(sceneLoadResult == SceneFileV2::eError::ERROR_NO_ERROR);

            auto testSwitch = [scene](FastName name, int32 index)
            {
                Entity* switchEntity = scene->FindByName(name);
                SwitchComponent* sw = GetSwitchComponent(switchEntity);
                TEST_VERIFY(sw != nullptr);
                if(sw != nullptr)
                {
                    TEST_VERIFY(sw->GetSwitchIndex() == index);
                }
            };
            testSwitch(DAVA::FastName(ResourceEditor::SWITCH_NODE_NAME), 1);
            testSwitch(FastName("switchWithoutChildren"), 0);
        }

        Cleanup();
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("SwitchEntityCreator.cpp")
    END_FILES_COVERED_BY_TESTS();
};
// clang-format on
