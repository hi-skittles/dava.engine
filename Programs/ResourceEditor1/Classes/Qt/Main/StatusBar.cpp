#include "StatusBar.h"

#include "Time/SystemTimer.h"

#include "Main/mainwindow.h"
#include "Scene/SceneEditor2.h"
#include "Commands2/Base/RECommand.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Selection/Selection.h"
#include "Classes/Selection/SelectionData.h"

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

    selectionWrapper = REGlobal::CreateDataWrapper(DAVA::ReflectedTypeDB::Get<SelectionData>());
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

    const SelectableGroup& selection = Selection::GetSelection();
    if (selection.IsEmpty() == false)
    {
        DAVA::float32 distanceToCamera = activeScene->cameraSystem->GetDistanceToCamera();
        SetDistanceToCamera(distanceToCamera);
    }
    else
    {
        ResetDistanceToCamera();
    }
}

void StatusBar::SceneActivated(SceneEditor2* scene)
{
    DVASSERT(scene != nullptr);
    activeScene = scene;

    UpdateDistanceToCamera();
}

void StatusBar::SceneDeactivated(SceneEditor2* scene)
{
    DVASSERT(scene != nullptr);
    activeScene = nullptr;
}

void StatusBar::OnDataChanged(const DAVA::TArc::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
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
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData == nullptr)
    {
        selectionBoxSize->setText(QString());
        selectionBoxSize->setVisible(false);
        return;
    }

    const SelectableGroup& selection = selectionData->GetSelection();
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
