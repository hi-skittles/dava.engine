#pragma once

#include <TArc/DataProcessing/DataListener.h>
#include <TArc/DataProcessing/DataWrapper.h>

#include <Base/BaseTypes.h>
#include <Math/AABBox3.h>

#include <QStatusBar>

class QLabel;
namespace DAVA
{
class SceneEditor2;
class SelectableGroup;
} // namespace DAVA

class StatusBar final : public QStatusBar, private DAVA::DataListener, public DAVA::TrackedObject
{
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = 0);

public slots:
    void SceneActivated(DAVA::SceneEditor2* scene);
    void SceneDeactivated(DAVA::SceneEditor2* scene);

    void UpdateByTimer();
    void OnSceneGeometryChaged(DAVA::uint32 width, DAVA::uint32 height);

private:
    void OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields) override;

    void UpdateSelectionBoxUI(const DAVA::AABBox3& newBox);

    void UpdateDistanceToCamera();
    void UpdateFPS();
    void SetDistanceToCamera(DAVA::float32 distance);
    void ResetDistanceToCamera();
    void UpdateSelectionBoxSize();

    QLabel* distanceToCamera = nullptr;
    QLabel* fpsCounter = nullptr;
    QLabel* sceneGeometry = nullptr;
    QLabel* selectionBoxSize = nullptr;
    DAVA::SceneEditor2* activeScene = nullptr;

    DAVA::DataWrapper selectionWrapper;

    DAVA::uint64 lastTimeMS = 0;
};
