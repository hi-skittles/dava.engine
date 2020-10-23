#include "UnitTests/UnitTests.h"

#include "Scene3D/Scene.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/SlotComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Systems/SlotSystem.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Job/JobManager.h"
#include "Render/Texture.h"
#include "FileSystem/FilePath.h"
#include "Base/RefPtr.h"
#include "Base/FastName.h"
#include "Time/SystemTimer.h"

DAVA_TESTCLASS (SlotSystemTest)
{
    void ValidateParseResult(const DAVA::Vector<DAVA::SlotSystem::ItemsCache::Item>& items)
    {
        bool testItem1Found = false;
        bool testItem2Found = false;
        for (const DAVA::SlotSystem::ItemsCache::Item& item : items)
        {
            if (item.itemName == DAVA::FastName("TestItem1"))
            {
                testItem1Found = true;
                TEST_VERIFY(item.scenePath == DAVA::FilePath("~res:/3d/Maps/SlotItem/tree_k2_test3.sc2"));
                TEST_VERIFY(item.type == DAVA::FastName("SimpleItem"));
            }
            else if (item.itemName == DAVA::FastName("TestItem2"))
            {
                testItem2Found = true;
                TEST_VERIFY(item.scenePath == DAVA::FilePath("~res:/3d/Maps/SlotItem/big01.sc2"));
                TEST_VERIFY(item.type == DAVA::FastName("Test"));
            }
        }

        TEST_VERIFY(testItem1Found == true);
        TEST_VERIFY(testItem2Found == true);
    }

    DAVA_TEST (ConfigParseTest)
    {
        DAVA::RefPtr<DAVA::Scene> scene;
        scene.ConstructInplace();

        {
            DAVA::Vector<DAVA::SlotSystem::ItemsCache::Item> items = scene->slotSystem->GetItems(DAVA::FilePath("~res:/TestData/SlotSystemTest/slotConfig.yaml"));
            ValidateParseResult(items);
        }

        {
            DAVA::Vector<DAVA::SlotSystem::ItemsCache::Item> items = scene->slotSystem->GetItems(DAVA::FilePath("~res:/TestData/SlotSystemTest/slotConfig.xml"));
            ValidateParseResult(items);
        }
    }

    DAVA_TEST (LoadItemsTest)
    {
        using namespace DAVA;
        TEST_VERIFY(testScene.Get() == nullptr);
        testScene.ConstructInplace();

        Vector<SlotComponent*> components;

        {
            RefPtr<Entity> aEntity;
            aEntity.ConstructInplace();

            SlotComponent* alfaComponent = new SlotComponent();
            alfaComponent->SetSlotName(FastName("TestSlot"));
            alfaComponent->SetConfigFilePath(FilePath("~res:/TestData/SlotSystemTest/slotConfig.yaml"));
            aEntity->AddComponent(alfaComponent);
            components.push_back(alfaComponent);
            testScene->AddNode(aEntity.Get());
            testScene->slotSystem->SetAttachmentTransform(alfaComponent, Matrix4::MakeRotation(Vector3(1.0f, 0.0f, 0.5f), -0.3f) * Matrix4::MakeTranslation(Vector3(30.0f, 25.0f, 0.0f)));
        }

        {
            RefPtr<Entity> bEntity;
            bEntity.ConstructInplace();

            SlotComponent* betaComponent = new SlotComponent();
            betaComponent->SetSlotName(FastName("TestSlot"));
            betaComponent->SetConfigFilePath(FilePath("~res:/TestData/SlotSystemTest/slotConfig.xml"));
            bEntity->AddComponent(betaComponent);
            components.push_back(betaComponent);

            SlotComponent* gammaComponent = new SlotComponent();
            gammaComponent->SetSlotName(FastName("SecondTestSlot"));
            gammaComponent->SetConfigFilePath(FilePath("~res:/TestData/SlotSystemTest/slotConfig.xml"));
            bEntity->AddComponent(gammaComponent);
            components.push_back(gammaComponent);
            testScene->AddNode(bEntity.Get());
            testScene->slotSystem->SetAttachmentTransform(betaComponent, Matrix4::MakeRotation(Vector3(1.0f, 0.0f, 0.5f), -0.3f) * Matrix4::MakeTranslation(Vector3(35.0f, 25.0f, 0.0f)));
            testScene->slotSystem->SetAttachmentTransform(gammaComponent, Matrix4::MakeRotation(Vector3(2.0f, 0.0f, 0.5f), -0.3f) * Matrix4::MakeTranslation(Vector3(35.0f, 25.0f, 0.0f)));
        }

        for (SlotComponent* component : components)
        {
            TEST_VERIFY(testScene->slotSystem->LookUpLoadedEntity(component) == nullptr);
            TEST_VERIFY(testScene->slotSystem->GetSlotState(component) == SlotSystem::eSlotState::NOT_LOADED);
        }

        testScene->slotSystem->AttachItemToSlot(testScene.Get(), FastName("TestSlot"), FastName("TestItem1"));
        testScene->slotSystem->AttachItemToSlot(testScene.Get(), FastName("SecondTestSlot"), FastName("TestItem2"));

        for (SlotComponent* component : components)
        {
            TEST_VERIFY(testScene->slotSystem->LookUpLoadedEntity(component) != nullptr);
            TEST_VERIFY(testScene->slotSystem->GetSlotState(component) == SlotSystem::eSlotState::LOADING);
        }
    }

    DAVA_TEST (RemoveLoadedEntityTest)
    {
        using namespace DAVA;
        TEST_VERIFY(testScene.Get() == nullptr);
        testScene.ConstructInplace();

        RefPtr<Entity> aEntity;
        aEntity.ConstructInplace();

        SlotComponent* alfaComponent = new SlotComponent();
        alfaComponent->SetSlotName(FastName("TestSlot"));
        alfaComponent->SetConfigFilePath(FilePath("~res:/TestData/SlotSystemTest/slotConfig.yaml"));
        aEntity->AddComponent(alfaComponent);
        testScene->AddNode(aEntity.Get());
        testScene->slotSystem->SetAttachmentTransform(alfaComponent, Matrix4::MakeRotation(Vector3(1.0f, 0.0f, 0.5f), -0.3f) * Matrix4::MakeTranslation(Vector3(30.0f, 25.0f, 0.0f)));

        testScene->slotSystem->AttachItemToSlot(testScene.Get(), FastName("TestSlot"), FastName("TestItem1"));

        TEST_VERIFY(aEntity->GetChildrenCount() == 1);
        aEntity->RemoveNode(aEntity->GetChild(0));
        TEST_VERIFY(testScene->slotSystem->GetSlotState(alfaComponent) == SlotSystem::eSlotState::NOT_LOADED);
    }

    DAVA_TEST (RemoveSlotEntityTest)
    {
        using namespace DAVA;
        TEST_VERIFY(testScene.Get() == nullptr);
        testScene.ConstructInplace();

        RefPtr<Entity> aEntity;
        aEntity.ConstructInplace();

        SlotComponent* alfaComponent = new SlotComponent();
        alfaComponent->SetSlotName(FastName("TestSlot"));
        alfaComponent->SetConfigFilePath(FilePath("~res:/TestData/SlotSystemTest/slotConfig.yaml"));
        aEntity->AddComponent(alfaComponent);
        testScene->AddNode(aEntity.Get());
        testScene->slotSystem->SetAttachmentTransform(alfaComponent, Matrix4::MakeRotation(Vector3(1.0f, 0.0f, 0.5f), -0.3f) * Matrix4::MakeTranslation(Vector3(30.0f, 25.0f, 0.0f)));

        testScene->slotSystem->AttachItemToSlot(testScene.Get(), FastName("TestSlot"), FastName("TestItem1"));

        testScene->RemoveNode(aEntity.Get());
    }

    DAVA_TEST (RemoveSlotWhileLoadingTest)
    {
        using namespace DAVA;
        TEST_VERIFY(testScene.Get() == nullptr);
        testScene.ConstructInplace();

        RefPtr<Entity> aEntity;
        aEntity.ConstructInplace();

        SlotComponent* alfaComponent = new SlotComponent();
        alfaComponent->SetSlotName(FastName("TestSlot"));
        alfaComponent->SetConfigFilePath(FilePath("~res:/TestData/SlotSystemTest/slotConfig.yaml"));
        aEntity->AddComponent(alfaComponent);
        testScene->AddNode(aEntity.Get());
        testScene->slotSystem->SetAttachmentTransform(alfaComponent, Matrix4::MakeRotation(Vector3(1.0f, 0.0f, 0.5f), -0.3f) * Matrix4::MakeTranslation(Vector3(30.0f, 25.0f, 0.0f)));

        testScene->slotSystem->AttachItemToSlot(testScene.Get(), FastName("TestSlot"), FastName("TestItem1"));

        TEST_VERIFY(aEntity->GetChildrenCount() == 1);
        aEntity->RemoveComponent(alfaComponent);
    }

    void Update(DAVA::float32 timeElapsed, const DAVA::String& testName) override
    {
        using namespace DAVA;
        if (GetEngineContext()->jobManager->HasWorkerJobs() == false)
        {
            updateAfterJobFinished = true;
        }

        if (testScene.Get() != nullptr)
        {
            testScene->Update(timeElapsed);
        }

        if (testName == "RemoveLoadedEntityTest")
        {
            TEST_VERIFY(testScene->GetChildrenCount() == 1);
            Entity* aEntity = testScene->GetChild(0);
            TEST_VERIFY(aEntity->GetComponentCount<SlotComponent>() == 1);
            SlotComponent* alfaComponent = static_cast<SlotComponent*>(aEntity->GetComponent<SlotComponent>());
            SlotSystem::eSlotState state = testScene->slotSystem->GetSlotState(alfaComponent);
            TEST_VERIFY(state == SlotSystem::eSlotState::NOT_LOADED || state == SlotSystem::eSlotState::LOADED);
        }
        else if (testName == "RemoveSlotWhileLoadingTest")
        {
            TEST_VERIFY(testScene->GetChildrenCount() == 1);
            TEST_VERIFY(testScene->GetChild(0)->GetChildrenCount() == 0);
        }
    }

    bool ValidateLoadedItem(DAVA::Entity * rootEntity, DAVA::SlotComponent * component, const DAVA::FastName& subItemName) const
    {
        using namespace DAVA;
        bool entityFound = false;
        for (int32 i = 0; i < rootEntity->GetChildrenCount(); ++i)
        {
            Entity* itemEntity = rootEntity->GetChild(i);
            if (itemEntity->GetName() == component->GetSlotName())
            {
                entityFound = true;
                TEST_VERIFY(testScene->slotSystem->LookUpLoadedEntity(component) == itemEntity);
                TEST_VERIFY(testScene->slotSystem->LookUpSlot(itemEntity) == component);

                if (GetTransformComponent(itemEntity)->GetLocalTransform() == component->GetAttachmentTransform())
                {
                    if (itemEntity->GetChildrenCount() == 1)
                    {
                        TEST_VERIFY(testScene->slotSystem->GetSlotState(component) == SlotSystem::eSlotState::LOADED);
                        DAVA::Entity* loadedSceneEntity = itemEntity->GetChild(0);
                        TEST_VERIFY(loadedSceneEntity->GetName() == subItemName);
                        return true;
                    }
                }
            }
        }

        TEST_VERIFY(testScene->slotSystem->GetSlotState(component) == SlotSystem::eSlotState::LOADING);

        return false;
    }

    bool TestComplete(const DAVA::String& testName) const override
    {
        if (testName == "LoadItemsTest")
        {
            bool result = false;
            if (DAVA::GetEngineContext()->jobManager->HasWorkerJobs() == false)
            {
                TEST_VERIFY(testScene->GetChildrenCount() == 2);
                DAVA::Entity* aEntity = testScene->GetChild(0);

                TEST_VERIFY(aEntity->GetComponentCount<DAVA::SlotComponent>() == 1);
                DAVA::SlotComponent* alfaComponent = static_cast<DAVA::SlotComponent*>(aEntity->GetComponent<DAVA::SlotComponent>());
                result = ValidateLoadedItem(aEntity, alfaComponent, DAVA::FastName("tree_k2_test3"));

                DAVA::Entity* bEntity = testScene->GetChild(1);
                TEST_VERIFY(bEntity->GetComponentCount<DAVA::SlotComponent>() == 2);
                DAVA::SlotComponent* betaComponent = static_cast<DAVA::SlotComponent*>(bEntity->GetComponent<DAVA::SlotComponent>(0));
                DAVA::SlotComponent* gamaComponent = static_cast<DAVA::SlotComponent*>(bEntity->GetComponent<DAVA::SlotComponent>(1));

                result &= ValidateLoadedItem(bEntity, betaComponent, DAVA::FastName("tree_k2_test3"));
                result &= ValidateLoadedItem(bEntity, gamaComponent, DAVA::FastName("BigPlant01"));
            }

            return result;
        }
        else if (testName == "RemoveLoadedEntityTest" ||
                 testName == "RemoveSlotWhileLoadingTest" ||
                 testName == "RemoveSlotEntityTest")
        {
            return updateAfterJobFinished;
        }

        return true;
    }

    void SetUp(const DAVA::String& testName) override
    {
        gpuOrder = DAVA::Texture::GetGPULoadingOrder();
        DAVA::Vector<DAVA::eGPUFamily> newOrder;
        newOrder.push_back(DAVA::GPU_ORIGIN);
        DAVA::Texture::SetGPULoadingOrder(newOrder);
    }

    void TearDown(const DAVA::String& testName) override
    {
        DAVA::Texture::SetGPULoadingOrder(gpuOrder);
        testScene = DAVA::RefPtr<DAVA::Scene>();
        updateAfterJobFinished = false;
    }

    DAVA::Vector<DAVA::eGPUFamily> gpuOrder;
    DAVA::RefPtr<DAVA::Scene> testScene;
    bool updateAfterJobFinished = false;
};
