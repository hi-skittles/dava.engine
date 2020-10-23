#include "Classes/Project/ProjectManagerModule.h"
#include "Classes/Application/LaunchModule.h"

#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Global/GlobalOperations.h>

#include <TArc/Testing/TArcUnitTests.h>
#include <TArc/Testing/MockListener.h>
#include <TArc/Testing/GMockInclude.h>
#include <TArc/DataProcessing/DataWrapper.h>

#include <Base/Any.h>

#include <QMainWindow>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QTest>

namespace PMT
{
const DAVA::String projectModulePropertiesKey = "ProjectManagerProperties";
const DAVA::String testFolder = DAVA::String("~doc:/Test/");
const DAVA::String firstFakeProjectPath = DAVA::String("~doc:/Test/ProjectManagerTest1/");

class SceneManagerMockModule : public DAVA::ClientModule
{
protected:
    void PostInit() override
    {
        using namespace DAVA;

        RegisterOperation(DAVA::CreateFirstSceneOperation.ID, this, &SceneManagerMockModule::CreateNewSceneMock);
        RegisterOperation(DAVA::CloseAllScenesOperation.ID, this, &SceneManagerMockModule::CloseAllScenesMock);

        ContextAccessor* accessor = GetAccessor();
        // prepare test environment
        {
            CommandLineModuleTestUtils::CreateTestFolder(testFolder);
        }

        {
            CommandLineModuleTestUtils::CreateProjectInfrastructure(firstFakeProjectPath);
            DAVA::PropertiesItem propsItem = GetAccessor()->CreatePropertiesNode(PMT::projectModulePropertiesKey);
            propsItem.Set(lastOpenProject, firstFakeProjectPath);
        }

        accessor->GetGlobalContext()->GetData<GeneralSettings>()->reloadParticlesOnProjectOpening = false;
    }

    ~SceneManagerMockModule() override
    {
        using namespace DAVA;

        ContextAccessor* accessor = GetAccessor();

        DAVA::PropertiesItem propsItem = accessor->CreatePropertiesNode(projectModulePropertiesKey);
        propsItem.Set(lastOpenProject, DAVA::Any(lastOpenedProject));

        PropertiesItem item = accessor->CreatePropertiesNode(projectsHistoryKey);
        item.Set(recentItemsKey, projectsHistory);

        CommandLineModuleTestUtils::ClearTestFolder(testFolder);
    }

    void CreateNewSceneMock()
    {
    }

    void CloseAllScenesMock(bool)
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SceneManagerMockModule, DAVA::ClientModule)
    {
        DAVA::ReflectionRegistrator<SceneManagerMockModule>::Begin()
        .ConstructorByPointer()
        .End();
    }

    //recent items properties names
    const DAVA::String projectsHistoryKey = "Recent projects";
    const DAVA::String recentItemsKey = "recent items";
    const DAVA::String lastOpenProject = "Internal/LastProjectPath";

    DAVA::Vector<DAVA::String> projectsHistory;
    DAVA::FilePath lastOpenedProject;
    DAVA::FilePath laspOpenedPathInSettings;
    bool reloadParticles = false;
};
}

DAVA_TARC_TESTCLASS(ProjectManagerTests)
{
    DAVA::DataWrapper wrapper;
    DAVA::MockListener listener;

    DAVA_TEST (LaunchAppTest)
    {
        using namespace ::testing;
        wrapper = GetAccessor()->CreateWrapper(DAVA::ReflectedTypeDB::Get<DAVA::ProjectManagerData>());
        wrapper.SetListener(&listener);
        DVASSERT(wrapper.HasData() == true);

        DAVA::ProjectManagerData* data = GetAccessor()->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
        TEST_VERIFY(data != nullptr);
        if (data->IsOpened() == true)
        {
            DAVA::FilePath path = data->GetProjectPath();
            TEST_VERIFY(path == PMT::firstFakeProjectPath);
        }
        else
        {
            EXPECT_CALL(listener, OnDataChanged(_, _))
            .WillOnce(Invoke([this](const DAVA::DataWrapper& w, const DAVA::Vector<DAVA::Any>& fields)
                             {
                                 TEST_VERIFY(w.HasData());
                                 TEST_VERIFY(wrapper == w);
                                 TEST_VERIFY(fields.empty());

                                 DAVA::ProjectManagerData* data = GetAccessor()->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
                                 TEST_VERIFY(data != nullptr);
                                 TEST_VERIFY(data->IsOpened() == true);

                                 DAVA::FilePath path = data->GetProjectPath();

                                 TEST_VERIFY(path == PMT::firstFakeProjectPath);
                             }));
        }
    }

    DAVA_TEST (CloseProject)
    {
        using namespace ::testing;
        using namespace DAVA;

        EXPECT_CALL(*GetMockInvoker(), Invoke(DAVA::CloseAllScenesOperation.ID, _))
        .WillOnce(Invoke([this](int id, const DAVA::Any& arg1)
                         {
                             TEST_VERIFY(arg1.CanCast<bool>());
                             TEST_VERIFY(arg1.Cast<bool>() == true);

                             DAVA::Vector<DataContext::ContextID> contexts;
                             GetAccessor()->ForEachContext([&contexts](DataContext& ctx)
                                                           {
                                                               contexts.push_back(ctx.GetID());
                                                           });

                             ContextManager* mng = GetContextManager();
                             for (DataContext::ContextID id : contexts)
                             {
                                 mng->DeleteContext(id);
                             }
                         }));

        EXPECT_CALL(listener, OnDataChanged(wrapper, _))
        .WillOnce(Invoke([this](const DataWrapper& w, const DAVA::Vector<DAVA::Any>& fields)
                         {
                             TEST_VERIFY(wrapper.HasData());
                             TEST_VERIFY(fields.size() == 1);
                             TEST_VERIFY(fields[0].CanCast<DAVA::String>());
                             TEST_VERIFY(fields[0].Cast<DAVA::String>() == ProjectManagerData::ProjectPathProperty);

                             TEST_VERIFY(GetAccessor()->GetGlobalContext()->GetData<ProjectManagerData>()->GetProjectPath().IsEmpty());
                         }));

        QWidget* wnd = GetWindow(DAVA::mainWindowKey);
        QMainWindow* mainWnd = qobject_cast<QMainWindow*>(wnd);
        TEST_VERIFY(wnd != nullptr);

        QMenuBar* menu = mainWnd->menuBar();
        QMenu* fileMenu = menu->findChild<QMenu*>(MenuItems::menuFile);

        QAction* closeProjectAction = nullptr;
        QList<QAction*> actions = fileMenu->actions();
        foreach (QAction* action, actions)
        {
            if (action->objectName() == "Close Project")
            {
                closeProjectAction = action;
                break;
            }
        }

        TEST_VERIFY(closeProjectAction != nullptr);
        closeProjectAction->triggered(false);
    }

    BEGIN_TESTED_MODULES()
    DECLARE_TESTED_MODULE(PMT::SceneManagerMockModule)
    DECLARE_TESTED_MODULE(ProjectManagerModule)
    DECLARE_TESTED_MODULE(LaunchModule)
    END_TESTED_MODULES()
};
