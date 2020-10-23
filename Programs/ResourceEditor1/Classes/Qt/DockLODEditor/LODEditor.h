#pragma once

#include "Base/BaseTypes.h"

#include "Scene/System/EditorLODSystem.h"
#include "Scene/System/EditorStatisticsSystem.h"
#include "Tools/QtPosSaver/QtPosSaver.h"

#include <QWidget>

namespace Ui
{
class LODEditor;
}

namespace DAVA
{
namespace TArc
{
class FieldBinder;
}
}

class GlobalOperations;
class SceneEditor2;
class SelectableGroup;
class QFrame;
class QPushButton;
class LODDistanceWidget;

class LazyUpdater;
class LODEditor : public QWidget, private EditorLODSystemUIDelegate, EditorStatisticsSystemUIDelegate
{
    Q_OBJECT

public:
    explicit LODEditor(QWidget* parent = nullptr);
    ~LODEditor() override;

    void Init(const std::shared_ptr<GlobalOperations>& globalOperations);

private slots:

    //force signals
    void ForceDistanceStateChanged(bool checked);
    void ForceDistanceChanged(int distance);
    void ForceLayerActivated(int index);

    //scene signals
    void SceneActivated(SceneEditor2* scene);
    void SceneDeactivated(SceneEditor2* scene);

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

    void UpdatePanelsUI(SceneEditor2* forScene);
    void UpdatePanelsForCurrentScene();

    void SetupDistancesUI();
    void UpdateDistanceSpinboxesUI(const DAVA::Vector<DAVA::float32>& distances, const DAVA::Vector<bool>& multiple, DAVA::int32 count);

    void SetupActionsUI();

    //EditorLODSystemV2UIDelegate
    void UpdateModeUI(EditorLODSystem* forSystem, const eEditorMode mode, bool recursive) override;
    void UpdateForceUI(EditorLODSystem* forSystem, const ForceValues& forceValues) override;
    void UpdateDistanceUI(EditorLODSystem* forSystem, const LODComponentHolder* lodData) override;
    void UpdateActionUI(EditorLODSystem* forSystem) override;
    //end of EditorLODSystemV2UIDelegate

    //EditorStatisticsSystemUIDelegate
    void UpdateTrianglesUI(EditorStatisticsSystem* forSystem) override;
    //end of EditorStatisticsSystemUIDelegate

    EditorLODSystem* GetCurrentEditorLODSystem() const;
    EditorStatisticsSystem* GetCurrentEditorStatisticsSystem() const;

    std::unique_ptr<Ui::LODEditor> ui;

    DAVA::Vector<LODDistanceWidget*> distanceWidgets;

    LazyUpdater* panelsUpdater = nullptr;
    SceneEditor2* activeScene = nullptr;
    std::shared_ptr<GlobalOperations> globalOperations;

    std::unique_ptr<DAVA::TArc::FieldBinder> selectionFieldBinder;
};
