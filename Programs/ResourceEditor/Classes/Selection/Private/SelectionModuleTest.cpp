#include "Classes/Application/LaunchModule.h"
#include "Classes/Application/ReflectionExtensions.h"
#include "Classes/MockModules/MockProjectManagerModule.h"
#include "Classes/SceneManager/SceneManagerModule.h"
#include "Classes/Selection/SelectionModule.h"

#include <REPlatform/Commands/EntityAddCommand.h>
#include <REPlatform/Commands/TransformCommand.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/DataNodes/Settings/GlobalSceneSettings.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Scene/BaseTransformProxies.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/CameraSystem.h>
#include <REPlatform/Scene/Systems/SelectionSystem.h>

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Testing/TArcTestClass.h>
#include <TArc/Testing/TArcUnitTests.h>
#include <TArc/Testing/MockDefine.h>

#include <Engine/Qt/RenderWidget.h>
#include <Math/Transform.h>
#include <Math/TransformUtils.h>
#include <Render/Highlevel/Camera.h>
#include <Scene3D/Components/RenderComponent.h>
#include <Scene3D/Components/TransformComponent.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>
#include <UI/UIControlSystem.h>

#include <QApplication> 
#include <QTest>

using namespace DAVA;

DAVA_TARC_TESTCLASS(SelectionModuleTest)
{
    void SetFocus(QWidget * widget)
    {
        widget->clearFocus();
        if (widget->isActiveWindow() == false)
        {
            QWidget* topLevel = widget;
            while (topLevel->parent() != nullptr)
                topLevel = topLevel->parentWidget();
            qApp->setActiveWindow(widget);
        }

        widget->setFocus(Qt::MouseFocusReason);
    }

    void SkipUIFrames()
    {
        for (int32 i = 0; i < FRAME_SKIP; ++i)
            EXPECT_CALL(*this, AfterWrappersSync());
    }

    void SetUpTest(bool autoSelectNewEntity)
    {
        GetAccessor()->GetGlobalContext()->GetData<GlobalSceneSettings>()->autoSelectNewEntity = autoSelectNewEntity;
        InvokeOperation(CreateFirstSceneOperation.ID);
    }

    void TearDownTest()
    {
        InvokeOperation(DAVA::CloseAllScenesOperation.ID, false);
    }

    DAVA_TEST (SelectEntityTest)
    {
        SetUpTest(false);
        SceneEditor2* scene = GetAccessor()->GetActiveContext()->GetData<SceneData>()->GetScene().Get();
        ScopedPtr<Entity> node(new Entity());
        scene->AddNode(node);
        node->SetLocked(false);
        Vector3 newPosition(25.0f, 25.0f, 0.0f);
        Transform newTransform(newPosition);
        TransformComponent* tc = node->GetComponent<TransformComponent>();
        scene->Exec(std::unique_ptr<Command>(new TransformCommand(Selectable(node.get()), tc->GetLocalTransform(), newTransform)));

        RenderWidget* renderWidget = GetContextManager()->GetRenderWidget();

        auto stepMouseClick = [this, scene, renderWidget, node]()
        {
            Rect r(renderWidget->childrenRect().x(), renderWidget->childrenRect().y(), renderWidget->childrenRect().width(), renderWidget->childrenRect().height());
            Camera* camera = scene->GetCurrentCamera();
            Vector2 nodePos = camera->GetOnScreenPosition(node->GetComponent<TransformComponent>()->GetWorldTransform().GetTranslation(), r);
            QTest::mouseClick(GetRenderWidgetTestTarget(), Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(nodePos.x, nodePos.y));
        };

        auto stepCheckSelection = [this, scene, node]()
        {
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.GetSize() == 1);
            TEST_VERIFY(selectionGroup.ContainsObject(node.get()));
        };

        ::testing::InSequence sequence;
        SkipUIFrames();
        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::SetFocus, this, renderWidget)))
        .WillOnce(::testing::Invoke(stepMouseClick))
        .WillOnce(::testing::Invoke(stepCheckSelection))
        .WillOnce(::testing::Invoke([node]() { node->SetLocked(true); }))
        .WillOnce(::testing::Invoke(stepMouseClick))
        .WillOnce(::testing::Invoke(stepCheckSelection))
        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::TearDownTest, this)))
        .WillOnce(::testing::Return());
    }

    DAVA_TEST (SelectionSomeEntityTest)
    {
        SetUpTest(false);
        SceneEditor2* scene = GetAccessor()->GetActiveContext()->GetData<SceneData>()->GetScene().Get();
        QWidget* widget = GetRenderWidgetTestTarget();
        ScopedPtr<Entity> node1(new Entity());
        ScopedPtr<Entity> node2(new Entity());
        scene->AddNode(node1);
        scene->AddNode(node2);

        Rect viewport = scene->GetSystem<SceneCameraSystem>()->GetViewportRect();
        Vector2 pos = scene->GetCurrentCamera()->GetOnScreenPosition(node1->GetComponent<TransformComponent>()->GetWorldTransform().GetTranslation(), viewport);
        TEST_VERIFY(viewport.PointInside(pos));
        pos = scene->GetCurrentCamera()->GetOnScreenPosition(node2->GetComponent<TransformComponent>()->GetWorldTransform().GetTranslation(), viewport);
        TEST_VERIFY(viewport.PointInside(pos));

        auto stepMouseMove = [this, widget, viewport]()
        {
            QPoint startPoint(widget->x() + viewport.x, widget->y() + viewport.y);
            QPoint endPoint(startPoint.x() + viewport.dx, startPoint.y() + viewport.dy);

            SelectByMouseRect(widget, startPoint, endPoint);
        };

        auto stepCheckSelection = [this, scene, node1, node2]()
        {
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.GetSize() == 2);
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()));
            TEST_VERIFY(selectionGroup.ContainsObject(node2.get()));
        };

        ::testing::InSequence sequence;
        SkipUIFrames();
        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::SetFocus, this, widget)))
        .WillOnce(::testing::Invoke(stepMouseMove))
        .WillOnce(::testing::Invoke(stepCheckSelection))
        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::TearDownTest, this)))
        .WillOnce(::testing::Return());
    }

    DAVA_TEST (SelectionAfterRemoveTest)
    {
        SetUpTest(true);
        SceneEditor2* scene = GetAccessor()->GetActiveContext()->GetData<SceneData>()->GetScene().Get();
        RenderWidget* renderWidget = GetContextManager()->GetRenderWidget();

        auto startTest = [this, scene]
        {
            ScopedPtr<Entity> node1(new Entity());
            scene->AddNode(node1);
            scene->Update(0.01f);
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.GetSize() == 1);
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()));

            ScopedPtr<Entity> node2(new Entity());
            scene->AddNode(node2);
            scene->Update(0.01f);
            selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.GetSize() == 1);
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()) == false);
            TEST_VERIFY(selectionGroup.ContainsObject(node2.get()));

            scene->RemoveNode(node2);
            scene->Update(0.01f);
            selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.IsEmpty());
        };

        ::testing::InSequence sequence;
        SkipUIFrames();
        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::SetFocus, this, renderWidget)))
        .WillOnce(::testing::Invoke(startTest))
        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::TearDownTest, this)))
        .WillOnce(::testing::Return());
    }

    DAVA_TEST (SelectionSolidEntity)
    {
        SetUpTest(false);
        SceneEditor2* scene = GetAccessor()->GetActiveContext()->GetData<SceneData>()->GetScene().Get();
        QWidget* widget = GetRenderWidgetTestTarget();
        ScopedPtr<Entity> node1(new Entity());
        ScopedPtr<Entity> node2(new Entity());
        ScopedPtr<Entity> node3(new Entity());

        node1.get()->AddNode(node2);
        node1.get()->AddNode(node3);
        scene->AddNode(node1);

        Rect viewport = scene->GetSystem<SceneCameraSystem>()->GetViewportRect();

        auto stepSelectViewportByMouse = [this, scene, widget, viewport]()
        {
            QPoint startPoint(widget->x() + viewport.x, widget->y() + viewport.y);
            QPoint endPoint(startPoint.x() + viewport.dx, startPoint.y() + viewport.dy);

            SelectByMouseRect(widget, startPoint, endPoint);
        };

        auto stepClick = [this, scene, node3]
        {
            RenderWidget* renderWidget = GetContextManager()->GetRenderWidget();
            Rect r(renderWidget->childrenRect().x(), renderWidget->childrenRect().y(), renderWidget->childrenRect().width(), renderWidget->childrenRect().height());
            Vector2 nodePos = scene->GetCurrentCamera()->GetOnScreenPosition(node3->GetComponent<TransformComponent>()->GetWorldTransform().GetTranslation(), r);
            QTest::mouseClick(GetRenderWidgetTestTarget(), Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(nodePos.x, nodePos.y));
        };

        auto stepMoveThirdEntity = [scene, node3]
        {
            Vector3 newPosition(15.0f, 15.0f, 0.0f);
            Matrix4 newTransform = Matrix4::MakeTranslation(newPosition);

            scene->Exec(std::unique_ptr<Command>(new TransformCommand(Selectable(node3.get()), node3->GetComponent<TransformComponent>()->GetLocalTransform(), newTransform)));
        };

        auto stepClickNotInEntity = [this, scene, node1]
        {
            float32 scale = 2.f * GetAccessor()->GetGlobalContext()->GetData<GlobalSceneSettings>()->debugBoxScale;
            Vector3 left = scene->GetCurrentCamera()->GetLeft();
            Matrix4 sourceMatrix = node1->GetComponent<TransformComponent>()->GetWorldMatrix();
            Vector3 offsetPoint = (scale * left) * sourceMatrix;

            RenderWidget* renderWidget = GetContextManager()->GetRenderWidget();
            Rect r(renderWidget->childrenRect().x(), renderWidget->childrenRect().y(), renderWidget->childrenRect().width(), renderWidget->childrenRect().height());
            Vector2 pos = scene->GetCurrentCamera()->GetOnScreenPosition(offsetPoint, r);

            QTest::mouseClick(GetRenderWidgetTestTarget(), Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(pos.x, pos.y));
        };

        auto stepCheckEmptySelection = [this, scene, node1, node2, node3]()
        {
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.IsEmpty());
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()) == false);
            TEST_VERIFY(selectionGroup.ContainsObject(node2.get()) == false);
            TEST_VERIFY(selectionGroup.ContainsObject(node3.get()) == false);
        };

        auto stepCheckRectSelectionWithSolidFalse = [this, scene, node1, node2, node3]()
        {
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.GetSize() == 3);
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()));
            TEST_VERIFY(selectionGroup.ContainsObject(node2.get()));
            TEST_VERIFY(selectionGroup.ContainsObject(node3.get()));
        };

        auto stepCheckSelectionWithSolidTrue = [this, scene, node1, node2, node3]()
        {
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.GetSize() == 1);
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()));
            TEST_VERIFY(selectionGroup.ContainsObject(node2.get()) == false);
            TEST_VERIFY(selectionGroup.ContainsObject(node3.get()) == false);
        };

        auto stepCheckClickSelectionWithSolidFalse = [this, scene, node1, node2, node3]()
        {
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.GetSize() == 1);
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()) == false);
            TEST_VERIFY(selectionGroup.ContainsObject(node2.get()) == false);
            TEST_VERIFY(selectionGroup.ContainsObject(node3.get()));
        };

        ::testing::InSequence sequence;
        SkipUIFrames();
        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::SetFocus, this, widget)))

        .WillOnce(::testing::Invoke([node1] { node1->SetSolid(false); }))
        .WillOnce(::testing::Invoke(stepClickNotInEntity))
        .WillOnce(::testing::Invoke(stepCheckEmptySelection))

        .WillOnce(::testing::Invoke(stepMoveThirdEntity))
        .WillOnce(::testing::Invoke(stepSelectViewportByMouse))
        .WillOnce(::testing::Invoke(stepCheckRectSelectionWithSolidFalse))

        .WillOnce(::testing::Invoke(stepClick))
        .WillOnce(::testing::Invoke(stepCheckClickSelectionWithSolidFalse))

        .WillOnce(::testing::Invoke([node1] { node1->SetSolid(true); }))
        .WillOnce(::testing::Invoke(stepSelectViewportByMouse))
        .WillOnce(::testing::Invoke(stepCheckSelectionWithSolidTrue))

        .WillOnce(::testing::Invoke(stepClick))
        .WillOnce(::testing::Invoke(stepCheckSelectionWithSolidTrue))

        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::TearDownTest, this)))
        .WillOnce(::testing::Return());
    }

    DAVA_TEST (SelectionByHotkey)
    {
        SetUpTest(true);
        SceneEditor2* scene = GetAccessor()->GetActiveContext()->GetData<SceneData>()->GetScene().Get();
        ScopedPtr<Entity> node1(new Entity());
        scene->Exec(std::make_unique<EntityAddCommand>(node1, scene));
        scene->Update(0.01f);

        auto stepCheckUndo = [scene, node1]()
        {
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.IsEmpty());
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()) == false);
        };

        auto stepCheckRedo = [scene, node1]()
        {
            SelectableGroup selectionGroup = scene->GetSystem<SelectionSystem>()->GetSelection();
            TEST_VERIFY(selectionGroup.GetSize() == 1);
            TEST_VERIFY(selectionGroup.ContainsObject(node1.get()));
        };

        ::testing::InSequence sequence;
        SkipUIFrames();
        EXPECT_CALL(*this, AfterWrappersSync())
        .WillOnce(::testing::Invoke(stepCheckRedo))
        .WillOnce(::testing::Invoke([scene]() { scene->Undo(); }))
        .WillOnce(::testing::Invoke(stepCheckUndo))
        .WillOnce(::testing::Invoke([scene]() { scene->Redo(); }))
        .WillOnce(::testing::Invoke(stepCheckRedo))
        .WillOnce(::testing::Invoke([this]() { QTest::keyClick(GetRenderWidgetTestTarget(), Qt::Key_Z, Qt::ControlModifier); }))
        .WillOnce(::testing::Invoke(stepCheckUndo))
        .WillOnce(::testing::Invoke([this]() { QTest::keyClick(GetRenderWidgetTestTarget(), Qt::Key_Z, Qt::ShiftModifier | Qt::ControlModifier); }))
        .WillOnce(::testing::Invoke(stepCheckRedo))

        .WillOnce(::testing::Invoke(DAVA::Bind(&SelectionModuleTest::TearDownTest, this)))
        .WillOnce(::testing::Return());
    }

    MOCK_METHOD0_VIRTUAL(AfterWrappersSync, void());

    BEGIN_TESTED_MODULES()
    DECLARE_TESTED_MODULE(ReflectionExtensionsModule)
    DECLARE_TESTED_MODULE(Mock::ProjectManagerModule)
    DECLARE_TESTED_MODULE(SceneManagerModule)
    DECLARE_TESTED_MODULE(SelectionModule)
    DECLARE_TESTED_MODULE(LaunchModule)
    END_TESTED_MODULES()
};