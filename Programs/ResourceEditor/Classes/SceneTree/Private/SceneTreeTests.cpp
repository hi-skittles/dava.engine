#include "Classes/Application/ReflectionExtensions.h"
#include "Classes/SceneManager/SceneManagerModule.h"
#include "Classes/SceneTree/Private/SceneTreeSystem.h"
#include "Classes/SceneTree/SceneTreeModule.h"
#include "Classes/Selection/SelectionModule.h"

#include "Classes/MockModules/MockProjectManagerModule.h"

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/Selectable.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <TArc/DataProcessing/AnyQMetaType.h>
#include <TArc/SharedModules/SettingsModule/SettingsModule.h>
#include <TArc/Testing/MockDefine.h>
#include <TArc/Testing/MockListener.h>
#include <TArc/Testing/TArcUnitTests.h>
#include <TArc/Testing/GMockInclude.h>

#include <Base/Any.h>
#include <Scene3D/Components/Controller/WASDControllerComponent.h>

#include <QLineEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QtTest>
#include <QVariant>

DAVA_TARC_TESTCLASS(SceneTreeTests)
{
    QTreeView* sceneTreeView = nullptr;

    template <typename T>
    T* ExtractObject(QModelIndex index)
    {
        QVariant sceneVariant = sceneTreeView->model()->data(index, Qt::UserRole);
        TEST_VERIFY(sceneVariant.canConvert<DAVA::Any>());
        DAVA::Selectable sceneObj(sceneVariant.value<DAVA::Any>());
        TEST_VERIFY(sceneObj.ContainsObject() == true);

        T* result = nullptr;
        if (sceneObj.CanBeCastedTo<T>() == true)
        {
            result = sceneObj.Cast<T>();
        }

        return result;
    }

    void ForceSyncSceneTree()
    {
        ForceSyncSceneTree(GetAccessor()->GetActiveContext()->GetData<DAVA::SceneData>()->GetScene().Get());
    }

    static void ForceSyncSceneTree(DAVA::Scene * scene)
    {
        scene->GetSystem<SceneTreeSystem>()->Process(0.01f);
    }

    void MatchEntitiesWithScene()
    {
        TEST_VERIFY(sceneTreeView != nullptr);
        TEST_VERIFY(sceneTreeView->model() != nullptr);
        QModelIndex rootIndex = sceneTreeView->rootIndex();
        TEST_VERIFY(rootIndex == QModelIndex());

        sceneTreeView->expandAll();

        DAVA::SceneEditor2* scene = ExtractObject<DAVA::SceneEditor2>(rootIndex);
        TEST_VERIFY(scene != nullptr);

        DAVA::SceneEditor2* activeScene = GetAccessor()->GetActiveContext()->GetData<DAVA::SceneData>()->GetScene().Get();
        TEST_VERIFY(scene == activeScene);

        DAVA::Function<QModelIndex(DAVA::Entity*, QModelIndex index)> checkEntityWithChildren = [&](DAVA::Entity* e, QModelIndex index) {
            DAVA::Entity* entity = ExtractObject<DAVA::Entity>(index);
            TEST_VERIFY(e == entity);

            QModelIndex currentIndex = index;
            for (DAVA::int32 childIndex = 0; childIndex < entity->GetChildrenCount(); ++childIndex)
            {
                DAVA::Entity* child = entity->GetChild(childIndex);
                currentIndex = checkEntityWithChildren(child, sceneTreeView->indexBelow(currentIndex));
            }

            return currentIndex;
        };

        QAbstractItemModel* model = sceneTreeView->model();
        for (DAVA::int32 childIndex = 0; childIndex < activeScene->GetChildrenCount(); ++childIndex)
        {
            DAVA::Entity* child = activeScene->GetChild(childIndex);

            checkEntityWithChildren(child, model->index(childIndex, 0, rootIndex));
        }
    }

    DAVA_TEST (SceneTreeCreationTest)
    {
        DAVA::Logger::Info("SceneTreeCreationTest start");
        sceneTreeView = LookupSingleWidget<QTreeView>(DAVA::mainWindowKey, QStringLiteral("SceneTreeView"));
        TEST_VERIFY(sceneTreeView != nullptr);
        TEST_VERIFY(sceneTreeView->model() == nullptr);
        TEST_VERIFY(sceneTreeView->rootIndex().isValid() == false);
        TEST_VERIFY(sceneTreeView->indexBelow(sceneTreeView->rootIndex()).isValid() == false);
        DAVA::Logger::Info("SceneTreeCreationTest end");
    }

    DAVA_TEST (LightAndCameraTest)
    {
        DAVA::Logger::Info("LightAndCameraTest start");
        TEST_VERIFY(sceneTreeView != nullptr);

        using namespace ::testing;

        DAVA::ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetActiveContext() == nullptr);
        InvokeOperation(DAVA::CreateFirstSceneOperation.ID);

        DAVA::DataContext* ctx = accessor->GetActiveContext();
        TEST_VERIFY(ctx != nullptr);
        ForceSyncSceneTree();

        auto finalStep = [this]() {
            DAVA::Logger::Info("LightAndCameraTest final step start");
            MatchEntitiesWithScene();
            InvokeOperation(DAVA::CloseAllScenesOperation.ID, false);
            DAVA::Logger::Info("LightAndCameraTest final step end");
        };

        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(Invoke(finalStep))
        .WillOnce(Return());

        DAVA::Logger::Info("LightAndCameraTest end");
    }

    DAVA_TEST (SwitchScenesTest)
    {
        DAVA::Logger::Info("SwitchScenesTest start");
        TEST_VERIFY(sceneTreeView != nullptr);

        using namespace ::testing;

        DAVA::ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetActiveContext() == nullptr);
        InvokeOperation(DAVA::CreateFirstSceneOperation.ID);

        DAVA::DataContext* ctx = accessor->GetActiveContext();
        TEST_VERIFY(ctx != nullptr);

        DAVA::SceneData* sceneData = ctx->GetData<DAVA::SceneData>();

        DAVA::ScopedPtr<DAVA::Entity> newNode(new DAVA::Entity());
        newNode->SetName("dummyEntity");
        newNode->AddComponent(new DAVA::WASDControllerComponent());
        sceneData->GetScene()->AddNode(newNode.get());
        ForceSyncSceneTree();

        auto step1 = [this]() {
            MatchEntitiesWithScene();
            InvokeOperation(DAVA::CreateFirstSceneOperation.ID);
            ForceSyncSceneTree();
        };

        auto step2 = [this, ctx]() {
            MatchEntitiesWithScene();
            GetContextManager()->ActivateContext(ctx->GetID());
        };

        auto finalStep = [this]() {
            MatchEntitiesWithScene();
            InvokeOperation(DAVA::CloseAllScenesOperation.ID, false);
        };

        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(Invoke(step1))
        .WillOnce(Return())
        .WillOnce(Invoke(step2))
        .WillOnce(Return())
        .WillOnce(Invoke(finalStep))
        .WillOnce(Return());
    }

    DAVA_TEST (ChangeSceneInplaceScenesTest)
    {
        TEST_VERIFY(sceneTreeView != nullptr);

        using namespace ::testing;

        DAVA::ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetActiveContext() == nullptr);
        InvokeOperation(DAVA::CreateFirstSceneOperation.ID);

        DAVA::DataContext* ctx = accessor->GetActiveContext();
        TEST_VERIFY(ctx != nullptr);

        DAVA::SceneData* sceneData = ctx->GetData<DAVA::SceneData>();
        DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

        DAVA::ScopedPtr<DAVA::Entity> newNode(new DAVA::Entity());
        newNode->SetName("dummyEntity");
        newNode->AddComponent(new DAVA::WASDControllerComponent());
        sceneData->GetScene()->AddNode(newNode.get());

        DAVA::Entity* entityForDelete = new DAVA::Entity();
        entityForDelete->SetName("nodeForDelete");

        auto step1 = [=]() {
            MatchEntitiesWithScene();

            scene->AddNode(entityForDelete);

            {
                DAVA::ScopedPtr<DAVA::Entity> subNode(new DAVA::Entity());
                subNode->SetName("subNode1");
                newNode->AddNode(subNode.get());
            }

            {
                DAVA::ScopedPtr<DAVA::Entity> subNode(new DAVA::Entity());
                subNode->SetName("subNode2");
                newNode->AddNode(subNode.get());
            }
        };

        auto step2 = [=]() {
            MatchEntitiesWithScene();

            scene->RemoveNode(entityForDelete);
            ForceSyncSceneTree(scene);
        };

        auto finalStep = [=]() {
            entityForDelete->Release();
            MatchEntitiesWithScene();
            InvokeOperation(DAVA::CloseAllScenesOperation.ID, false);
        };

        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(Invoke(step1))
        .WillOnce(Invoke(step2))
        .WillOnce(Invoke(finalStep))
        .WillOnce(Return());
    }

    DAVA_TEST (ExcludeFromSelectionOnCollapseTest)
    {
        DAVA::ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetActiveContext() == nullptr);
        InvokeOperation(DAVA::CreateFirstSceneOperation.ID);

        DAVA::DataContext* ctx = accessor->GetActiveContext();
        TEST_VERIFY(ctx != nullptr);

        DAVA::SceneData* sceneData = ctx->GetData<DAVA::SceneData>();
        DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

        DAVA::ScopedPtr<DAVA::Entity> dummyNode(new DAVA::Entity());
        dummyNode->SetName("dummyEntity");
        DAVA::Entity* rawDummyPointer = dummyNode;

        DAVA::ScopedPtr<DAVA::Entity> embeddedNode(new DAVA::Entity());
        DAVA::Entity* rawEmbeddedPointer = embeddedNode;
        embeddedNode->SetName("embeddedEntity");
        dummyNode->AddNode(embeddedNode);
        scene->AddNode(dummyNode);
        ForceSyncSceneTree(scene);

        auto step1 = [this, rawDummyPointer, rawEmbeddedPointer]() {
            TEST_VERIFY(sceneTreeView != nullptr);
            TEST_VERIFY(sceneTreeView->model() != nullptr);
            QModelIndex rootIndex = sceneTreeView->rootIndex();
            TEST_VERIFY(rootIndex == QModelIndex());

            QModelIndex index = sceneTreeView->model()->index(0, 0, rootIndex);
            while (index.isValid() && index.data(Qt::DisplayRole).toString() != QString("dummyEntity"))
            {
                index = sceneTreeView->indexBelow(index);
            }

            TEST_VERIFY(index.isValid() == true);
            DAVA::Entity* dummy = ExtractObject<DAVA::Entity>(index);
            TEST_VERIFY(sceneTreeView->isExpanded(index) == false);
            TEST_VERIFY(dummy == rawDummyPointer);
            TEST_VERIFY(sceneTreeView->indexBelow(index).isValid() == false);

            QRect itemRect = sceneTreeView->visualRect(index);
            QPoint clickToExpandPoint = QPoint(sceneTreeView->indentation() >> 1, itemRect.center().y());

            QTest::mouseClick(sceneTreeView->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), clickToExpandPoint);
            QModelIndex embeddedIndex = sceneTreeView->indexBelow(index);
            TEST_VERIFY(embeddedIndex.isValid() == true);
            DAVA::Entity* embedded = ExtractObject<DAVA::Entity>(embeddedIndex);
            TEST_VERIFY(embedded == rawEmbeddedPointer);

            QTest::mouseClick(sceneTreeView->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), itemRect.center());

            QRect embeddetItemRect = sceneTreeView->visualRect(embeddedIndex);
            QTest::mouseClick(sceneTreeView->viewport(), Qt::LeftButton, Qt::ControlModifier, embeddetItemRect.center());

            DAVA::SelectionData* selectionData = GetAccessor()->GetActiveContext()->GetData<DAVA::SelectionData>();
            {
                DAVA::SelectableGroup currentSelection = selectionData->GetSelection();
                TEST_VERIFY(currentSelection.GetSize() == 2);
                TEST_VERIFY(currentSelection.ContainsObject(DAVA::Any(rawDummyPointer)) == true);
                TEST_VERIFY(currentSelection.ContainsObject(DAVA::Any(rawEmbeddedPointer)) == true);
            }

            QTest::mouseClick(sceneTreeView->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), clickToExpandPoint);

            {
                DAVA::SelectableGroup currentSelection = selectionData->GetSelection();
                TEST_VERIFY(currentSelection.GetSize() == 1);
                TEST_VERIFY(currentSelection.ContainsObject(DAVA::Any(rawDummyPointer)) == true);
                TEST_VERIFY(currentSelection.ContainsObject(DAVA::Any(rawEmbeddedPointer)) == false);
            }

            InvokeOperation(DAVA::CloseAllScenesOperation.ID, false);
        };

        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step1))
        .WillOnce(::testing::Return());
    }

    DAVA_TEST (FiltrationTest)
    {
        DAVA::ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetActiveContext() == nullptr);
        InvokeOperation(DAVA::CreateFirstSceneOperation.ID);

        DAVA::DataContext* ctx = accessor->GetActiveContext();
        TEST_VERIFY(ctx != nullptr);

        DAVA::SceneData* sceneData = ctx->GetData<DAVA::SceneData>();
        DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

        DAVA::ScopedPtr<DAVA::Entity> dummyNode(new DAVA::Entity());
        dummyNode->SetName("dummyRoot");
        DAVA::Entity* rawDummyPointer = dummyNode;

        DAVA::ScopedPtr<DAVA::Entity> filterMatchedNode(new DAVA::Entity());
        DAVA::Entity* rawFilterMatchedPointer = filterMatchedNode;
        filterMatchedNode->SetName("filterMacthed_Entity");
        dummyNode->AddNode(filterMatchedNode);

        DAVA::ScopedPtr<DAVA::Entity> filterNotMatchedNode1(new DAVA::Entity());
        filterNotMatchedNode1->SetName("filterNotMacthed1");
        filterMatchedNode->AddNode(filterNotMatchedNode1);

        DAVA::ScopedPtr<DAVA::Entity> filterNotMatchedNode2(new DAVA::Entity());
        filterNotMatchedNode2->SetName("filterNotMacthed2");
        dummyNode->AddNode(filterNotMatchedNode2);

        scene->AddNode(dummyNode);

        ForceSyncSceneTree(scene);

        auto step1 = [this]() {
            TEST_VERIFY(sceneTreeView != nullptr);
            TEST_VERIFY(sceneTreeView->model() != nullptr);
            QModelIndex rootIndex = sceneTreeView->rootIndex();
            TEST_VERIFY(rootIndex == QModelIndex());

            MatchEntitiesWithScene();

            QLineEdit* lineEdit = LookupSingleWidget<QLineEdit>(DAVA::mainWindowKey, "SceneTreeFilterTextEdit");
            QTest::keyClicks(lineEdit, "Entity");
            QTest::keyClick(lineEdit, Qt::Key_Enter);
        };

        auto step2 = [this, rawDummyPointer, rawFilterMatchedPointer]() {
            TEST_VERIFY(sceneTreeView != nullptr);
            TEST_VERIFY(sceneTreeView->model() != nullptr);
            QModelIndex rootIndex = sceneTreeView->rootIndex();
            TEST_VERIFY(rootIndex == QModelIndex());

            QModelIndex firstVisible = sceneTreeView->model()->index(0, 0, rootIndex);
            TEST_VERIFY(firstVisible.isValid() == true);

            DAVA::Set<DAVA::Entity*> visibleEntities;
            while (firstVisible.isValid())
            {
                visibleEntities.insert(ExtractObject<DAVA::Entity>(firstVisible));
                firstVisible = sceneTreeView->indexBelow(firstVisible);
            }

            TEST_VERIFY(visibleEntities.size() == 2);
            TEST_VERIFY(visibleEntities.count(rawFilterMatchedPointer) == 1);
            TEST_VERIFY(visibleEntities.count(rawDummyPointer) == 1);

            QLineEdit* lineEdit = LookupSingleWidget<QLineEdit>(DAVA::mainWindowKey, "SceneTreeFilterTextEdit");
            lineEdit->clear();
        };

        auto step3 = [this]() {
            MatchEntitiesWithScene();
            InvokeOperation(DAVA::CloseAllScenesOperation.ID, false);
        };

        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step1))
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step2))
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step3))
        .WillOnce(::testing::Return());
    }

    QToolButton* LookupActionButton(const QString& buttonTooltip)
    {
        QToolBar* sceneTreeToolBar = LookupSingleWidget<QToolBar>(DAVA::mainWindowKey, "SceneTreeToolBar");
        QList<QToolButton*> buttons = sceneTreeToolBar->findChildren<QToolButton*>();

        QToolButton* resultButton = nullptr;

        foreach (QToolButton* button, buttons)
        {
            QString tooltip = button->toolTip();
            if (tooltip == buttonTooltip)
            {
                resultButton = button;
                break;
            }
        }

        TEST_VERIFY(resultButton != nullptr);

        return resultButton;
    }

    QModelIndex FindTopLevelItemByName(const QString& name)
    {
        QModelIndex index = sceneTreeView->model()->index(0, 0, QModelIndex());

        while (index.isValid() == true && index.data(Qt::DisplayRole).toString() != name)
        {
            index = sceneTreeView->indexBelow(index);
        }

        TEST_VERIFY(index.isValid() == true);

        return index;
    };

    DAVA_TEST (InverseExpandingOfSelectedTest)
    {
        DAVA::ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetActiveContext() == nullptr);
        InvokeOperation(DAVA::CreateFirstSceneOperation.ID);

        DAVA::DataContext* ctx = accessor->GetActiveContext();
        TEST_VERIFY(ctx != nullptr);

        DAVA::SceneData* sceneData = ctx->GetData<DAVA::SceneData>();
        DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

        auto creator = [scene](DAVA::int32 index) {
            DAVA::ScopedPtr<DAVA::Entity> child(new DAVA::Entity());
            child->SetName(DAVA::FastName(DAVA::Format("child%d", index)));

            DAVA::ScopedPtr<DAVA::Entity> subChild(new DAVA::Entity());
            subChild->SetName(DAVA::FastName(DAVA::Format("subChild%d", index)));
            child->AddNode(subChild);
            scene->AddNode(child);
        };

        creator(1);
        creator(2);
        creator(3);

        ForceSyncSceneTree(scene);

        auto step1 = [this]() {
            TEST_VERIFY(sceneTreeView != nullptr);
            TEST_VERIFY(sceneTreeView->model() != nullptr);

            QModelIndex child1Index = FindTopLevelItemByName("child1");
            QModelIndex child2Index = FindTopLevelItemByName("child2");
            QModelIndex child3Index = FindTopLevelItemByName("child3");

            TEST_VERIFY(sceneTreeView->isExpanded(child1Index) == false);
            TEST_VERIFY(sceneTreeView->isExpanded(child2Index) == false);
            TEST_VERIFY(sceneTreeView->isExpanded(child3Index) == false);

            QRect child2Rect = sceneTreeView->visualRect(child2Index);

            QPoint expandClickPoint = QPoint(sceneTreeView->indentation() >> 1, child2Rect.center().y());
            QTest::mouseClick(sceneTreeView->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), expandClickPoint);

            QRect child3Rect = sceneTreeView->visualRect(child3Index);

            QTest::mouseClick(sceneTreeView->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), child2Rect.center());
            QTest::mouseClick(sceneTreeView->viewport(), Qt::LeftButton, Qt::ControlModifier, child3Rect.center());

            TEST_VERIFY(sceneTreeView->isExpanded(child1Index) == false);
            TEST_VERIFY(sceneTreeView->isExpanded(child2Index) == true);
            TEST_VERIFY(sceneTreeView->isExpanded(child3Index) == false);

            QTest::keyClick(sceneTreeView, Qt::Key_X);
        };

        auto step2 = [this]() {
            QModelIndex child1Index = FindTopLevelItemByName("child1");
            QModelIndex child2Index = FindTopLevelItemByName("child2");
            QModelIndex child3Index = FindTopLevelItemByName("child3");

            TEST_VERIFY(sceneTreeView->isExpanded(child1Index) == false);
            TEST_VERIFY(sceneTreeView->isExpanded(child2Index) == false);
            TEST_VERIFY(sceneTreeView->isExpanded(child3Index) == true);

            QTest::mouseClick(LookupActionButton("Collapse All"), Qt::LeftButton);
        };

        auto step3 = [this]() {
            QModelIndex child1Index = FindTopLevelItemByName("child1");
            QModelIndex child2Index = FindTopLevelItemByName("child2");
            QModelIndex child3Index = FindTopLevelItemByName("child3");

            TEST_VERIFY(sceneTreeView->isExpanded(child1Index) == false);
            TEST_VERIFY(sceneTreeView->isExpanded(child2Index) == false);
            TEST_VERIFY(sceneTreeView->isExpanded(child3Index) == false);

            QTest::mouseClick(LookupActionButton("Expand All"), Qt::LeftButton);
        };

        auto step4 = [this]() {
            QModelIndex child1Index = FindTopLevelItemByName("child1");
            QModelIndex child2Index = FindTopLevelItemByName("child2");
            QModelIndex child3Index = FindTopLevelItemByName("child3");

            TEST_VERIFY(sceneTreeView->isExpanded(child1Index) == true);
            TEST_VERIFY(sceneTreeView->isExpanded(child2Index) == true);
            TEST_VERIFY(sceneTreeView->isExpanded(child3Index) == true);

            QTest::mouseClick(LookupActionButton("Collapse All"), Qt::LeftButton);
        };

        auto step5 = [this]() {
            InvokeOperation(DAVA::CloseAllScenesOperation.ID, false);
        };

        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step1))
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step2))
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step3))
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step4))
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step3))
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step5))
        .WillOnce(::testing::Return());
    }

    DAVA_TEST (RemoveSelectedEntityTest)
    {
        DAVA::ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetActiveContext() == nullptr);
        InvokeOperation(DAVA::CreateFirstSceneOperation.ID);

        DAVA::DataContext* ctx = accessor->GetActiveContext();
        TEST_VERIFY(ctx != nullptr);

        DAVA::SceneData* sceneData = ctx->GetData<DAVA::SceneData>();
        DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

        DAVA::ScopedPtr<DAVA::Entity> child(new DAVA::Entity());
        child->SetName("EntityForRemove");
        scene->AddNode(child);
        ForceSyncSceneTree(scene);

        auto step1 = [this, scene]() {
            TEST_VERIFY(sceneTreeView != nullptr);
            TEST_VERIFY(sceneTreeView->model() != nullptr);

            TEST_VERIFY(scene->GetChildrenCount() == 3);

            QModelIndex entityIndex = FindTopLevelItemByName("EntityForRemove");
            QRect itemRect = sceneTreeView->visualRect(entityIndex);
            QTest::mouseClick(sceneTreeView->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(), itemRect.center());
            QTest::mouseClick(LookupActionButton("Remove selection"), Qt::LeftButton);
        };

        auto step2 = [this, scene]() {
            TEST_VERIFY(sceneTreeView != nullptr);
            TEST_VERIFY(sceneTreeView->model() != nullptr);
            TEST_VERIFY(scene->GetChildrenCount() == 2);

            DAVA::FastName forRemoveName("EntityForRemove");
            for (DAVA::int32 i = 0; i < scene->GetChildrenCount(); ++i)
            {
                TEST_VERIFY(scene->GetChild(i)->GetName() != forRemoveName);
            }

            MatchEntitiesWithScene();
            InvokeOperation(DAVA::CloseAllScenesOperation.ID, false);
        };

        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step1))
        .WillOnce(::testing::Return())
        .WillOnce(::testing::Invoke(step2))
        .WillOnce(::testing::Return());
    }

    MOCK_METHOD0_VIRTUAL(AfterWrappersSync, void());

    BEGIN_TESTED_MODULES()
    DECLARE_TESTED_MODULE(ReflectionExtensionsModule)
    DECLARE_TESTED_MODULE(Mock::ProjectManagerModule)
    DECLARE_TESTED_MODULE(SceneManagerModule)
    DECLARE_TESTED_MODULE(SelectionModule)
    DECLARE_TESTED_MODULE(SceneTreeModule)
    END_TESTED_MODULES()
};
