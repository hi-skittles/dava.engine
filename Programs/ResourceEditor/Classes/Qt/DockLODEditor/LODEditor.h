#pragma once

#include "Classes/Qt/Tools/QtPosSaver/QtPosSaver.h"

#include <REPlatform/Scene/Systems/EditorLODSystem.h>
#include <REPlatform/Scene/Systems/EditorStatisticsSystem.h>

#include <Base/BaseTypes.h>

#include <QWidget>

namespace Ui
{
class LODEditor;
}

namespace DAVA
{
class FieldBinder;
class SceneEditor2;
class SelectableGroup;
}

class QFrame;
class QPushButton;
class LODDistanceWidget;

class LazyUpdater;
class LODEditor : public QWidget, private DAVA::EditorLODSystemUIDelegate, DAVA::EditorStatisticsSystemUIDelegate
{
    Q_OBJECT

public:
    explicit LODEditor(QWidget* parent = nullptr);
    ~LODEditor() override;

private slots:

    //force signals
    void ForceDistanceStateChanged(bool checked);
    void ForceDistanceChanged(int distance);
    void ForceLayerActivated(int index);

    //scene signals
    void SceneActivated(DAVA::SceneEditor2* scene);
    void SceneDeactivated(DAVA::SceneEditor2* scene);

    //distance signals
    void LODDistanceChangedByDistanceWidget();
    void LODDistanceIsChangingBySlider();
    void LODDistanceChangedBySlider();

    //mode signals
    void SceneModeToggled(bool toggled);
    void SelectionModeToggled(bool toggled);
    void RecursiveModeSelected(bool recursive);

    //action
    void CopyLastLODToLOD0Clicked();
    void CreatePlaneLODClicked();
    void DeleteLOD();

private:
    void OnSelectionChanged(const DAVA::Any& selection);

    void SetupSceneSignals();
    void SetupInternalUI();

    void SetupForceUI();
    void UpdateForceSliderRange();

    void UpdatePanelsUI(DAVA::SceneEditor2* forScene);
    void UpdatePanelsForCurrentScene();

    void SetupDistancesUI();
    void UpdateDistanceSpinboxesUI(const DAVA::Vector<DAVA::float32>& distances, const DAVA::Vector<bool>& multiple, DAVA::int32 count);

    void SetupActionsUI();

    //EditorLODSystemV2UIDelegate
    void UpdateModeUI(DAVA::EditorLODSystem* forSystem, const DAVA::eEditorMode mode, bool recursive) override;
    void UpdateForceUI(DAVA::EditorLODSystem* forSystem, const DAVA::ForceValues& forceValues) override;
    void UpdateDistanceUI(DAVA::EditorLODSystem* forSystem, const DAVA::LODComponentHolder* lodData) override;
    void UpdateActionUI(DAVA::EditorLODSystem* forSystem) override;
    //end of EditorLODSystemV2UIDelegate

    //EditorStatisticsSystemUIDelegate
    void UpdateTrianglesUI(DAVA::EditorStatisticsSystem* forSystem) override;
    //end of EditorStatisticsSystemUIDelegate

    DAVA::EditorLODSystem* GetCurrentEditorLODSystem() const;
    DAVA::EditorStatisticsSystem* GetCurrentEditorStatisticsSystem() const;

    std::unique_ptr<Ui::LODEditor> ui;

    DAVA::Vector<LODDistanceWidget*> distanceWidgets;

    LazyUpdater* panelsUpdater = nullptr;
    DAVA::SceneEditor2* activeScene = nullptr;

    std::unique_ptr<DAVA::FieldBinder> selectionFieldBinder;
};
