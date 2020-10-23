#include "Classes/Qt/Main/StatusBar.h"

#include <REPlatform/Scene/SceneEditor2.h>

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/Scene/Systems/CameraSystem.h>

#include <Time/SystemTimer.h>
#include <TArc/Core/Deprecated.h>

#include <QLabel>
#include <QLayout>
#include <QPalette>

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent)
{
    sceneGeometry = new QLabel(this);
    sceneGeometry->setToolTip("Resolution");
    sceneGeometry->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    addPermanentWidget(sceneGeometry);

    fpsCounter = new QLabel(this);
    fpsCounter->setToolTip("Current FPS for active scene");
    fpsCounter->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    addPermanentWidget(fpsCounter);

    distanceToCamera = new QLabel(this);
    distanceToCamera->setToolTip("Distance from camera to center of the selection");
    distanceToCamera->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    addPermanentWidget(distanceToCamera);

    selectionBoxSize = new QLabel(this);
    selectionBoxSize->setToolTip("Selection box size");
    selectionBoxSize->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    addPermanentWidget(selectionBoxSize);

    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setMargin(0);
    layout()->setSpacing(1);
    setStyleSheet("QStatusBar::item {border: none;}");

    selectionWrapper = DAVA::Deprecated::CreateDataWrapper(DAVA::ReflectedTypeDB::Get<DAVA::SelectionData>());
    selectionWrapper.SetListener(this);
}

void StatusBar::SetDistanceToCamera(DAVA::float32 distance)
{
    distanceToCamera->setText(QString::fromStdString(DAVA::Format("%0.6f", distance)));
}

void StatusBar::ResetDistanceToCamera()
{
    distanceToCamera->setText(QString::fromStdString("No selection"));
}

void StatusBar::UpdateDistanceToCamera()
{
    if (!activeScene)
    {
        ResetDistanceToCamera();
        return;
    }

    DAVA::SelectionData* data = DAVA::Deprecated::GetActiveDataNode<DAVA::SelectionData>();

    if (data != nullptr && data->GetSelection().IsEmpty() == false)
    {
        DAVA::float32 distanceToCamera = activeScene->GetSystem<DAVA::SceneCameraSystem>()->GetDistanceToCamera();
        SetDistanceToCamera(distanceToCamera);
    }
    else
    {
        ResetDistanceToCamera();
    }
}

void StatusBar::SceneActivated(DAVA::SceneEditor2* scene)
{
    DVASSERT(scene != nullptr);
    activeScene = scene;

    UpdateDistanceToCamera();
}

void StatusBar::SceneDeactivated(DAVA::SceneEditor2* scene)
{
    DVASSERT(scene != nullptr);
    activeScene = nullptr;
}

void StatusBar::OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    DVASSERT(selectionWrapper == wrapper);

    UpdateDistanceToCamera();
    UpdateSelectionBoxSize();
}

void StatusBar::UpdateByTimer()
{
    UpdateDistanceToCamera();
    UpdateFPS();
}

void StatusBar::OnSceneGeometryChaged(DAVA::uint32 width, DAVA::uint32 height)
{
    sceneGeometry->setText(QString::fromStdString(DAVA::Format("%u x %u", width, height)));
}

void StatusBar::UpdateSelectionBoxSize()
{
    DAVA::SelectionData* selectionData = DAVA::Deprecated::GetActiveDataNode<DAVA::SelectionData>();
    if (selectionData == nullptr)
    {
        selectionBoxSize->setText(QString());
        selectionBoxSize->setVisible(false);
        return;
    }

    const DAVA::SelectableGroup& selection = selectionData->GetSelection();
    if (selection.IsEmpty())
    {
        selectionBoxSize->setText(QString());
        selectionBoxSize->setVisible(false);
    }
    else
    {
        selectionBoxSize->setVisible(true);

        const DAVA::AABBox3& selectionBox = selectionData->GetSelectionBox();
        if (selectionBox.IsEmpty())
        {
            selectionBoxSize->setText("Empty box");
        }
        else
        {
            DAVA::Vector3 size = selectionBox.GetSize();
            selectionBoxSize->setText(QString::fromStdString(DAVA::Format("x:%0.2f, y: %0.2f, z: %0.2f", size.x, size.y, size.z)));
        }
    }
}

void StatusBar::UpdateFPS()
{
    DAVA::uint32 frames = 0;
    if (activeScene != nullptr)
    {
        frames = activeScene->GetFramesCount();
        activeScene->ResetFramesCount();
    }

    DAVA::uint64 currentTimeMS = DAVA::SystemTimer::GetMs();

    if (frames > 0 && lastTimeMS != 0 && lastTimeMS != currentTimeMS)
    {
        DAVA::uint64 deltaTime = currentTimeMS - lastTimeMS;
        fpsCounter->setText(QString::fromStdString(DAVA::Format("FPS: %lld", frames * 1000 / deltaTime)));
    }
    else
    {
        fpsCounter->setText(QString::fromStdString("FPS: unknown"));
    }

    lastTimeMS = currentTimeMS;
}
