#include "Classes/Application/REGlobal.h"
#include "Classes/Application/ReflectionExtensions.h"

#include "Classes/Project/ProjectManagerModule.h"
#include "Classes/Project/ProjectResources.h"
#include "Classes/Project/ProjectManagerData.h"

#include "Classes/SceneManager/SceneManagerModule.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Qt/Scene/SceneEditor2.h"

#include "Classes/CommandLine/Private/CommandLineModuleTestUtils.h"
#include "Classes/MockModules/MockProjectManagerModule.h"

#include <TArc/Testing/TArcUnitTests.h>
#include <TArc/Testing/MockDefine.h>
#include <TArc/Testing/MockListener.h>
#include <TArc/DataProcessing/DataListener.h>
#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/Utils/QtDelayedExecutor.h>
#include <TArc/Testing/GMockInclude.h>

#include <Physics/CharacterControllerComponent.h>

#include <Base/Any.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/FileSystem.h>

#include <QMainWindow>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTest>

namespace SMTest
{
const DAVA::FastName allEntitiesName = DAVA::FastName("AllComponentsEntity");
}

DAVA_TARC_TESTCLASS(SceneManagerModuleTests)
{
    DAVA::Vector<const DAVA::ReflectedType*> componentTypes;
    DAVA::Set<const DAVA::ReflectedType*> ignoreComponentTypes = {
        // TODO remove this ignores after fix assert in PhysicsSystem
        DAVA::ReflectedTypeDB::Get<DAVA::CharacterControllerComponent>()
    };
    void InitComponentDerivedTypes(const DAVA::Type* type)
    {
        using namespace DAVA;

        const TypeInheritance* inheritance = type->GetInheritance();
        Vector<TypeInheritance::Info> derivedTypes = inheritance->GetDerivedTypes();
        for (const TypeInheritance::Info& derived : derivedTypes)
        {
            const ReflectedType* refType = ReflectedTypeDB::GetByType(derived.type);
            if (refType == nullptr)
            {
                continue;
            }

            const std::unique_ptr<ReflectedMeta>& meta = refType->GetStructure()->meta;
            if (meta != nullptr && (nullptr != meta->GetMeta<M::CantBeCreatedManualyComponent>()))
            {
                continue;
            }

            if (ignoreComponentTypes.count(refType) != 0)
            {
                continue;
            }

            if (refType->GetCtor(derived.type->Pointer()))
            {
                componentTypes.emplace_back(refType);
            }

            InitComponentDerivedTypes(derived.type);
        }
    }

    class BaseTestListener : public DAVA::TArc::DataListener
    {
    public:
        bool testSucceed = false;
    };

public:
    SceneManagerModuleTests()
    {
        InitComponentDerivedTypes(DAVA::Type::Instance<DAVA::Component>());
    }

private:
    class NewSceneListener : public BaseTestListener
    {
    public:
        DAVA::TArc::ContextAccessor* accessor = nullptr;
        void OnDataChanged(const DAVA::TArc::DataWrapper& w, const DAVA::Vector<DAVA::Any>& fields) override
        {
            TEST_VERIFY(fields.empty());
            if (w.HasData())
            {
                using namespace DAVA;

                SceneData* data = accessor->GetActiveContext()->GetData<SceneData>();
                TEST_VERIFY(data != nullptr);

                FilePath scenePath = data->GetScenePath();
                TEST_VERIFY(scenePath.GetFilename() == "newscene1.sc2");
                FileSystem* fs = GetEngineContext()->fileSystem;
                TEST_VERIFY(fs->Exists(scenePath) == false);

                SceneData::TSceneType scene = data->GetScene();
                TEST_VERIFY(scene);
                scene->Update(0.16f);

                TEST_VERIFY(scene->GetChildrenCount() == 2);
                TEST_VERIFY(scene->FindByName("editor.camera-light") != nullptr);
                TEST_VERIFY(scene->FindByName("editor.debug-camera") != nullptr);

                testSucceed = true;
            }
        }
    };

    DAVA_TEST (CreateNewEmptyScene)
    {
        NewSceneListener listener;
        listener.accessor = GetAccessor();

        DAVA::TArc::DataWrapper wrapper = GetAccessor()->CreateWrapper(DAVA::ReflectedTypeDB::Get<SceneData>());
        wrapper.SetListener(&listener);

        TEST_VERIFY(wrapper.HasData() == false);
        if (wrapper.HasData() == false)
        {
            InvokeOperation(REGlobal::CreateFirstSceneOperation.ID);
        }

        TEST_VERIFY(listener.testSucceed);
    }

    DAVA_TEST (GenerateSceneDataAndSaveScene)
    {
        SceneData* data = GetAccessor()->GetActiveContext()->GetData<SceneData>();
        TEST_VERIFY(data != nullptr);
        if (data != nullptr)
        {
            using namespace DAVA;

            SceneData::TSceneType scene = data->GetScene();
            TEST_VERIFY(scene);

            ScopedPtr<Entity> entity(new Entity());
            entity->SetName(SMTest::allEntitiesName);
            scene->AddNode(entity);

            { //Add generated components
                for (const ReflectedType* cType : componentTypes)
                {
                    Any newComponent = cType->CreateObject(ReflectedType::CreatePolicy::ByPointer);
                    Component* component = newComponent.Cast<Component*>();
                    entity->AddComponent(component);
                }
            }

            scene->Update(0.16f);
            CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(Mock::ProjectManagerModule::testScenePath, Mock::ProjectManagerModule::testProjectPath, scene.Get());
        }

        CloseActiveScene();
    }

    class OpenSavedSceneListener : public BaseTestListener
    {
    public:
        DAVA::TArc::ContextAccessor* accessor = nullptr;
        DAVA::Vector<const DAVA::ReflectedType*> componentTypes;
        void OnDataChanged(const DAVA::TArc::DataWrapper& w, const DAVA::Vector<DAVA::Any>& fields) override
        {
            TEST_VERIFY(fields.empty());
            if (w.HasData())
            {
                using namespace DAVA;

                SceneData* data = accessor->GetActiveContext()->GetData<SceneData>();
                TEST_VERIFY(data != nullptr);

                FilePath scenePath = data->GetScenePath();
                TEST_VERIFY(scenePath == Mock::ProjectManagerModule::testScenePath);

                SceneData::TSceneType scene = data->GetScene();
                TEST_VERIFY(scene);
                scene->Update(0.16f);

                Entity* entity = scene->FindByName(SMTest::allEntitiesName);
                TestEntityWithComponents(entity);

                testSucceed = true;
            }
        }

        void TestEntityWithComponents(DAVA::Entity* entity)
        {
            TEST_VERIFY(entity != nullptr);
            if (entity != nullptr)
            {
                using namespace DAVA;

                for (const ReflectedType* cType : componentTypes)
                {
                    Any newComponent = cType->CreateObject(ReflectedType::CreatePolicy::ByPointer);
                    Component* component = newComponent.Cast<Component*>();

                    TEST_VERIFY(entity->GetComponentCount(component->GetType()) > 0);

                    delete component;
                }
            }
        }
    };

    DAVA_TEST (OpenSavedScene)
    {
        using namespace DAVA;
        using namespace DAVA::TArc;

        OpenSavedSceneListener listener;
        listener.accessor = GetAccessor();
        listener.componentTypes = componentTypes;

        DataWrapper wrapper = GetAccessor()->CreateWrapper(ReflectedTypeDB::Get<SceneData>());
        wrapper.SetListener(&listener);

        TEST_VERIFY(wrapper.HasData() == false);
        if (wrapper.HasData() == false)
        {
            InvokeOperation(REGlobal::OpenSceneOperation.ID, FilePath(Mock::ProjectManagerModule::testScenePath));

            TEST_VERIFY(wrapper.HasData() == true);

            { //add scene
                InvokeOperation(REGlobal::AddSceneOperation.ID, FilePath(Mock::ProjectManagerModule::testScenePath));

                SceneData* data = GetAccessor()->GetActiveContext()->GetData<SceneData>();
                TEST_VERIFY(data != nullptr);
                SceneData::TSceneType scene = data->GetScene();
                TEST_VERIFY(scene);
                scene->Update(0.16f);

                scene->SetName(FastName("OriginalScene"));
                FastName loadedEntityName = FastName(FilePath(Mock::ProjectManagerModule::testScenePath).GetFilename());
                Entity* loadedEntity = scene->FindByName(loadedEntityName);
                TEST_VERIFY(loadedEntity != nullptr);
                if (loadedEntity != nullptr)
                {
                    Entity* testedEntity = loadedEntity->FindByName(SMTest::allEntitiesName);
                    listener.TestEntityWithComponents(testedEntity);
                }
            }

            {
                InvokeOperation(REGlobal::SaveCurrentScene.ID);
            }
        }
        TEST_VERIFY(listener.testSucceed);

        CloseActiveScene();
    }

    DAVA_TEST (OpenSavedModifiedScene)
    {
        using namespace DAVA;
        using namespace DAVA::TArc;

        OpenSavedSceneListener listener;
        listener.accessor = GetAccessor();
        listener.componentTypes = componentTypes;

        DataWrapper wrapper = GetAccessor()->CreateWrapper(ReflectedTypeDB::Get<SceneData>());
        wrapper.SetListener(&listener);

        TEST_VERIFY(wrapper.HasData() == false);
        if (wrapper.HasData() == false)
        {
            InvokeOperation(REGlobal::OpenSceneOperation.ID, FilePath(Mock::ProjectManagerModule::testScenePath));

            TEST_VERIFY(wrapper.HasData() == true);

            { //Test scene
                SceneData* data = GetAccessor()->GetActiveContext()->GetData<SceneData>();
                TEST_VERIFY(data != nullptr);
                SceneData::TSceneType scene = data->GetScene();
                TEST_VERIFY(scene);
                scene->Update(0.16f);

                Vector<Entity*> foundEntities;
                scene->GetChildEntitiesWithCondition(foundEntities, [](Entity* entity)
                                                     {
                                                         return (entity->GetName() == SMTest::allEntitiesName);
                                                     });

                TEST_VERIFY(foundEntities.size() == 2);

                for (Entity* entity : foundEntities)
                {
                    listener.TestEntityWithComponents(entity);
                }
            }
        }
        TEST_VERIFY(listener.testSucceed);

        CloseActiveScene();
    }

    DAVA_TEST (OpenResentScene)
    {
        using namespace DAVA;
        using namespace DAVA::TArc;
        using namespace ::testing;

        OpenSavedSceneListener listener;
        listener.accessor = GetAccessor();
        listener.componentTypes = componentTypes;

        DataWrapper openSceneWrapper = GetAccessor()->CreateWrapper(ReflectedTypeDB::Get<SceneData>());
        openSceneWrapper.SetListener(&listener);

        TEST_VERIFY(openSceneWrapper.HasData() == false);
        if (openSceneWrapper.HasData() == false)
        {
            GetAccessor()->GetGlobalContext()->GetData<GlobalSceneSettings>()->openLastScene = true;
            InvokeOperation(REGlobal::CreateFirstSceneOperation.ID);
        }

        TEST_VERIFY(listener.testSucceed);
        CloseActiveScene();
    }

    DAVA::TArc::QtDelayedExecutor recentSceneDelayedExecutor;
    DAVA::TArc::DataWrapper openResentSceneWrapper;
    std::unique_ptr<OpenSavedSceneListener> recentSceneListener;

    void OpenResentSceneFromMenuDelayed()
    {
        using namespace ::testing;

        TEST_VERIFY(recentSceneListener);

        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(Return())
        .WillOnce(Invoke([this]() {

            testCompleted = true;

            TEST_VERIFY(recentSceneListener->testSucceed);
            recentSceneListener.reset();
            CloseActiveScene();
        }))
        .WillRepeatedly(Return());
    }

    DAVA_TEST (OpenResentSceneFromMenu)
    {
        using namespace DAVA;
        using namespace DAVA::TArc;

        recentSceneListener.reset(new OpenSavedSceneListener());
        recentSceneListener->accessor = GetAccessor();
        recentSceneListener->componentTypes = componentTypes;

        openResentSceneWrapper = GetAccessor()->CreateWrapper(ReflectedTypeDB::Get<SceneData>());
        openResentSceneWrapper.SetListener(recentSceneListener.get());

        TEST_VERIFY(openResentSceneWrapper.HasData() == false);
        if (openResentSceneWrapper.HasData() == false)
        {
            QWidget* wnd = GetWindow(DAVA::TArc::mainWindowKey);
            QMainWindow* mainWnd = qobject_cast<QMainWindow*>(wnd);
            TEST_VERIFY(wnd != nullptr);

            QMenuBar* menu = mainWnd->menuBar();
            QMenu* fileMenu = menu->findChild<QMenu*>(MenuItems::menuFile);

            QList<QAction*> actions = fileMenu->actions();
            TEST_VERIFY(actions.size() > 0);

            QAction* resentSceneAction = *actions.rbegin();
            TEST_VERIFY(resentSceneAction->text() == QString::fromStdString(FilePath(Mock::ProjectManagerModule::testScenePath).GetAbsolutePathname()));

            testCompleted = false;
            resentSceneAction->triggered(false);

            recentSceneDelayedExecutor.DelayedExecute(DAVA::MakeFunction(this, &SceneManagerModuleTests::OpenResentSceneFromMenuDelayed));
        }
    }

    MOCK_METHOD0_VIRTUAL(AfterWrappersSync, void());

    bool testCompleted = true;
    bool TestComplete(const DAVA::String& testName) const override
    {
        return testCompleted;
    }

    class CloseSceneListener : public BaseTestListener
    {
    public:
        void OnDataChanged(const DAVA::TArc::DataWrapper& w, const DAVA::Vector<DAVA::Any>& fields) override
        {
            TEST_VERIFY(fields.empty());
            TEST_VERIFY(w.HasData() == false);

            testSucceed = true;
        }
    };

    void CloseActiveScene()
    {
        CloseSceneListener listener;
        DAVA::TArc::DataWrapper wrapper = GetAccessor()->CreateWrapper(DAVA::ReflectedTypeDB::Get<SceneData>());
        wrapper.SetListener(&listener);

        TEST_VERIFY(wrapper.HasData());
        if (wrapper.HasData())
        {
            InvokeOperation(REGlobal::CloseAllScenesOperation.ID, false);
        }

        TEST_VERIFY(listener.testSucceed);
    }

    BEGIN_TESTED_MODULES()
    DECLARE_TESTED_MODULE(ReflectionExtensionsModule)
    DECLARE_TESTED_MODULE(Mock::ProjectManagerModule)
    DECLARE_TESTED_MODULE(SceneManagerModule)
    END_TESTED_MODULES()
};
