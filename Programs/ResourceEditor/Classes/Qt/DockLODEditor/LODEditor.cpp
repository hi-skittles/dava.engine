#include "ui_LODEditor.h"

#include "Classes/Qt/DockLODEditor/LODEditor.h"
#include "Classes/Qt/DockLODEditor/DistanceSlider.h"
#include "Classes/Qt/DockLODEditor/LODDistanceWidget.h"
#include "Classes/Qt/PlaneLODDialog/PlaneLODDialog.h"
#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/Commands/AddComponentCommand.h>
#include <REPlatform/Commands/RemoveComponentCommand.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Scene/Systems/EditorLODSystem.h>
#include <REPlatform/Scene/Systems/EditorStatisticsSystem.h>

#include <QtTools/Updaters/LazyUpdater.h>

#include <TArc/Utils/Utils.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Core/Deprecated.h>

#include <Utils/StringFormat.h>

#include <QLabel>
#include <QWidget>
#include <QLineEdit>
#include <QInputDialog>
#include <QFrame>
#include <QPushButton>
#include <QSignalBlocker>

using namespace DAVA;

namespace LODEditorDetail
{
const QString multiplePlaceHolder = "multiple values";
}

LODEditor::LODEditor(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LODEditor)
{
    ui->setupUi(this);

    Function<void()> fnUpdatePanels(this, &LODEditor::UpdatePanelsForCurrentScene);
    panelsUpdater = new LazyUpdater(fnUpdatePanels, this);

    SetupSceneSignals();
    SetupInternalUI();

    new QtPosSaver(this);
}

LODEditor::~LODEditor() = default;

void LODEditor::SetupSceneSignals()
{
    selectionFieldBinder.reset(new DAVA::FieldBinder(DAVA::Deprecated::GetAccessor()));
    {
        DAVA::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, DAVA::MakeFunction(this, &LODEditor::OnSelectionChanged));
    }

    connect(SceneSignals::Instance(), &SceneSignals::Activated, this, &LODEditor::SceneActivated);
    connect(SceneSignals::Instance(), &SceneSignals::Deactivated, this, &LODEditor::SceneDeactivated);
}

void LODEditor::SetupInternalUI()
{
    connect(ui->checkboxRecursive, &QCheckBox::clicked, this, &LODEditor::RecursiveModeSelected);
    connect(ui->radioButtonAllScene, &QRadioButton::toggled, this, &LODEditor::SceneModeToggled);
    connect(ui->radioButtonSelection, &QRadioButton::toggled, this, &LODEditor::SelectionModeToggled);

    SetupForceUI();
    SetupDistancesUI();
    SetupActionsUI();

    UpdatePanelsUI(nullptr);
    UpdateForceSliderRange();
}

//MODE
void LODEditor::SceneModeToggled(bool toggled)
{
    DAVA::CommonInternalSettings* settings = DAVA::Deprecated::GetGlobalContext()->GetData<DAVA::CommonInternalSettings>();
    settings->lodEditorSceneMode = toggled;

    EditorLODSystem* system = GetCurrentEditorLODSystem();
    system->SetMode(toggled ? eEditorMode::MODE_ALL_SCENE : eEditorMode::MODE_SELECTION);
}

void LODEditor::SelectionModeToggled(bool toggled)
{
    bool allSceneModeActivated = !toggled;
    DAVA::CommonInternalSettings* settings = DAVA::Deprecated::GetGlobalContext()->GetData<DAVA::CommonInternalSettings>();
    settings->lodEditorSceneMode = allSceneModeActivated;

    EditorLODSystem* system = GetCurrentEditorLODSystem();
    system->SetMode(toggled ? eEditorMode::MODE_SELECTION : eEditorMode::MODE_ALL_SCENE);
}

void LODEditor::RecursiveModeSelected(bool recursive)
{
    DAVA::CommonInternalSettings* settings = DAVA::Deprecated::GetGlobalContext()->GetData<DAVA::CommonInternalSettings>();
    settings->lodEditorRecursive = recursive;

    EditorLODSystem* system = GetCurrentEditorLODSystem();
    system->SetRecursive(recursive);
}

//ENDOF MODE

//PANELS
void LODEditor::UpdatePanelsUI(SceneEditor2* forScene)
{
    if (forScene != nullptr)
    {
        panelsUpdater->Update();
    }
    else
    {
        ui->frameLOD->setVisible(false);
    }
}

void LODEditor::UpdatePanelsForCurrentScene()
{
    bool panelVisible = false;

    EditorLODSystem* system = GetCurrentEditorLODSystem();
    if (system != nullptr)
    {
        const LODComponentHolder* lodData = system->GetActiveLODData();
        panelVisible = (lodData->GetLODLayersCount() > 0);
    }

    ui->frameLOD->setVisible(panelVisible);
}

//ENDOF PANELS

//FORCE
void LODEditor::SetupForceUI()
{
    connect(ui->enableForceDistance, &QCheckBox::toggled, this, &LODEditor::ForceDistanceStateChanged);

    ui->forceSlider->setRange(LodComponent::INVALID_DISTANCE, LodComponent::MAX_LOD_DISTANCE);
    ui->forceSlider->setValue(LodComponent::INVALID_DISTANCE);
    connect(ui->forceSlider, &QSlider::valueChanged, this, &LODEditor::ForceDistanceChanged);

    ui->forceLayer->clear();
    ui->forceLayer->addItem("Auto", LodComponent::INVALID_LOD_LAYER);
    for (uint32 i = 0; i < LodComponent::MAX_LOD_LAYERS; ++i)
    {
        ui->forceLayer->addItem(Format("%u", i).c_str(), QVariant(i));
    }
    ui->forceLayer->addItem("Last", EditorLODSystem::LAST_LOD_LAYER);
    ui->forceLayer->setCurrentIndex(0);

    connect(ui->forceLayer, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &LODEditor::ForceLayerActivated);
}

void LODEditor::ForceDistanceStateChanged(bool checked)
{
    EditorLODSystem* system = GetCurrentEditorLODSystem();

    ForceValues forceValues = system->GetForceValues();
    forceValues.flag = (checked) ? ForceValues::APPLY_LAYER : ForceValues::APPLY_DISTANCE;
    system->SetForceValues(forceValues);
}

void LODEditor::ForceDistanceChanged(int distance)
{
    EditorLODSystem* system = GetCurrentEditorLODSystem();

    ForceValues forceValues = system->GetForceValues();
    forceValues.distance = static_cast<float32>(distance);
    system->SetForceValues(forceValues);
}

void LODEditor::ForceLayerActivated(int index)
{
    EditorLODSystem* system = GetCurrentEditorLODSystem();
    ForceValues forceValues = system->GetForceValues();

    forceValues.layer = index - 1;
    if (forceValues.layer == LodComponent::MAX_LOD_LAYERS)
    {
        forceValues.layer = EditorLODSystem::LAST_LOD_LAYER;
    }

    system->SetForceValues(forceValues);
}

//ENDOF FORCE

//DISTANCES
void LODEditor::SetupDistancesUI()
{
    connect(ui->distanceSlider, &DistanceSlider::DistanceHandleMoved, this, &LODEditor::LODDistanceIsChangingBySlider);
    connect(ui->distanceSlider, &DistanceSlider::DistanceHandleReleased, this, &LODEditor::LODDistanceChangedBySlider);

    DVASSERT(distanceWidgets.empty());

    distanceWidgets.resize(LodComponent::MAX_LOD_LAYERS);

    for (DAVA::int32 i = 0; i < LodComponent::MAX_LOD_LAYERS; ++i)
    {
        distanceWidgets[i] = new LODDistanceWidget(ui->frameLOD);
        ui->verticalLayout->insertWidget(i + 3, distanceWidgets[i]);

        distanceWidgets[i]->SetMinMax(LodComponent::MIN_LOD_DISTANCE, LodComponent::MAX_LOD_DISTANCE);
        distanceWidgets[i]->SetDistance(LodComponent::MIN_LOD_DISTANCE, false);
        distanceWidgets[i]->SetIndex(i);

        connect(distanceWidgets[i], &LODDistanceWidget::DistanceChanged, this, &LODEditor::LODDistanceChangedByDistanceWidget);
        connect(distanceWidgets[i], &LODDistanceWidget::DistanceRemoved, this, &LODEditor::DeleteLOD);
    }
}

void LODEditor::UpdateDistanceSpinboxesUI(const DAVA::Vector<DAVA::float32>& distances, const DAVA::Vector<bool>& multiple, int32 count)
{
    DVASSERT(distances.size() == DAVA::LodComponent::MAX_LOD_LAYERS);
    DVASSERT(multiple.size() == DAVA::LodComponent::MAX_LOD_LAYERS);

    for (int32 i = 0; i < DAVA::LodComponent::MAX_LOD_LAYERS; ++i)
    {
        const QSignalBlocker guardWidget(distanceWidgets[i]);

        float64 minDistance = (i == 0) ? LodComponent::MIN_LOD_DISTANCE : distances[i - 1];
        float64 maxDistance = (i == DAVA::LodComponent::MAX_LOD_LAYERS - 1) ? std::numeric_limits<DAVA::float64>::max() : distances[i + 1];

        distanceWidgets[i]->SetMinMax(minDistance, maxDistance); //distance
        distanceWidgets[i]->SetDistance(distances[i], multiple[i]);

        distanceWidgets[i]->SetActive(i < count);
    }
}

void LODEditor::LODDistanceIsChangingBySlider()
{
    //update only UI
    const Vector<float32>& distances = ui->distanceSlider->GetDistances();

    EditorLODSystem* system = GetCurrentEditorLODSystem();
    const LODComponentHolder* lodData = system->GetActiveLODData();

    UpdateDistanceSpinboxesUI(distances, lodData->GetMultiple(), lodData->GetLODLayersCount());
}

void LODEditor::LODDistanceChangedBySlider()
{
    //apply changes to LOD objects
    const Vector<float32>& distances = ui->distanceSlider->GetDistances();

    EditorLODSystem* system = GetCurrentEditorLODSystem();
    system->SetLODDistances(distances);
}

void LODEditor::LODDistanceChangedByDistanceWidget()
{
    Vector<float32> distances(LodComponent::MAX_LOD_LAYERS, EditorLODSystem::LOD_DISTANCE_INFINITY);
    for (uint32 i = 0; i < distances.size(); ++i)
    {
        distances[i] = distanceWidgets[i]->GetDistance();
    }

    EditorLODSystem* system = GetCurrentEditorLODSystem();
    system->SetLODDistances(distances);
}

void LODEditor::DeleteLOD()
{
    LODDistanceWidget* dw = static_cast<LODDistanceWidget*>(sender());
    for (int32 i = 0; i < DAVA::LodComponent::MAX_LOD_LAYERS; ++i)
    {
        if (dw == distanceWidgets[i])
        {
            EditorLODSystem* system = GetCurrentEditorLODSystem();
            system->DeleteLOD(i);
            break;
        }
    }
}

//ENDOF DISTANCES

//SCENE SIGNALS

void LODEditor::SceneActivated(SceneEditor2* scene)
{
    DVASSERT(scene);
    activeScene = scene;

    DAVA::EditorLODSystem* lodSystem = activeScene->GetSystem<DAVA::EditorLODSystem>();
    DAVA::EditorStatisticsSystem* statisticSystem = activeScene->GetSystem<DAVA::EditorStatisticsSystem>();
    lodSystem->AddDelegate(this);
    statisticSystem->AddDelegate(this);

    DAVA::CommonInternalSettings* settings = DAVA::Deprecated::GetGlobalContext()->GetData<DAVA::CommonInternalSettings>();
    lodSystem->SetRecursive(settings->lodEditorRecursive);
}

void LODEditor::SceneDeactivated(SceneEditor2* scene)
{
    DVASSERT(scene);
    DAVA::EditorLODSystem* lodSystem = activeScene->GetSystem<DAVA::EditorLODSystem>();
    DAVA::EditorStatisticsSystem* statisticSystem = activeScene->GetSystem<DAVA::EditorStatisticsSystem>();

    lodSystem->RemoveDelegate(this);
    statisticSystem->RemoveDelegate(this);

    activeScene = nullptr;
    UpdatePanelsUI(nullptr);
}

void LODEditor::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<SelectableGroup>())
    {
        DVASSERT(activeScene != nullptr);
        EditorLODSystem* system = activeScene->GetSystem<DAVA::EditorLODSystem>();

        const SelectableGroup& selection = selectionAny.Get<SelectableGroup>();
        system->SelectionChanged(selection);
    }
}

//ENDOF SCENE SIGNALS

//ACTIONS

void LODEditor::SetupActionsUI()
{
    connect(ui->lastLodToFrontButton, &QPushButton::clicked, this, &LODEditor::CopyLastLODToLOD0Clicked);
    connect(ui->createPlaneLodButton, &QPushButton::clicked, this, &LODEditor::CreatePlaneLODClicked);
}

void LODEditor::CopyLastLODToLOD0Clicked()
{
    EditorLODSystem* system = GetCurrentEditorLODSystem();
    system->CopyLastLODToFirst();
}

void LODEditor::CreatePlaneLODClicked()
{
    EditorLODSystem* system = GetCurrentEditorLODSystem();
    const LODComponentHolder* lodData = system->GetActiveLODData();

    FilePath defaultTexturePath = system->GetPathForPlaneEntity();
    PlaneLODDialog dialog(lodData->GetLODLayersCount(), defaultTexturePath, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        DAVA::WaitDialogParams params;
        params.cancelable = false;
        params.needProgressBar = false;
        params.message = "Creating Plane LOD, Please wait...";

        std::unique_ptr<DAVA::WaitHandle> waitHandle = DAVA::Deprecated::GetUI()->ShowWaitDialog(DAVA::mainWindowKey, params);

        system->CreatePlaneLOD(dialog.GetSelectedLayer(), dialog.GetSelectedTextureSize(), dialog.GetSelectedTexturePath());
    }
}

//ENDOF ACTIONS

//DELEGATE
void LODEditor::UpdateModeUI(EditorLODSystem* forSystem, const eEditorMode mode, bool recursive)
{
    const QSignalBlocker guardRecursive(ui->checkboxRecursive);
    const QSignalBlocker guardAllScene(ui->radioButtonAllScene);
    const QSignalBlocker guardSelection(ui->radioButtonSelection);

    ui->radioButtonAllScene->setChecked(mode == eEditorMode::MODE_ALL_SCENE);
    ui->radioButtonSelection->setChecked(mode == eEditorMode::MODE_SELECTION);

    ui->checkboxRecursive->setChecked(recursive);

    panelsUpdater->Update();
}

void LODEditor::UpdateForceUI(EditorLODSystem* forSystem, const ForceValues& forceValues)
{
    const QSignalBlocker guard1(ui->enableForceDistance);
    const QSignalBlocker guard2(ui->forceSlider);
    const QSignalBlocker guard3(ui->forceLayer);

    const bool forceLayerSelected = (forceValues.flag & ForceValues::APPLY_LAYER) == ForceValues::APPLY_LAYER;
    ui->enableForceDistance->setChecked(forceLayerSelected);
    ui->forceSlider->setEnabled(!forceLayerSelected);
    ui->forceLayer->setEnabled(forceLayerSelected);

    UpdateForceSliderRange();
    ui->forceSlider->setValue(forceValues.distance);

    if (forceValues.layer == EditorLODSystem::LAST_LOD_LAYER)
    {
        ui->forceLayer->setCurrentIndex(ui->forceLayer->count() - 1);
    }
    else
    {
        int32 forceIndex = Min(forceValues.layer + 1, ui->forceLayer->count() - 1);
        ui->forceLayer->setCurrentIndex(forceIndex);
    }
}

void LODEditor::UpdateForceSliderRange()
{
    DAVA::float32 maxDistanceValue = DAVA::LodComponent::MAX_LOD_DISTANCE;
    EditorLODSystem* system = GetCurrentEditorLODSystem();
    if (system != nullptr)
    {
        const LODComponentHolder* lodData = system->GetActiveLODData();
        const DAVA::Vector<DAVA::float32>& distances = lodData->GetDistances();

        if (EditorLODSystem::IsFitModeEnabled(distances))
        {
            for (DAVA::float32 dist : distances)
            {
                if (fabs(dist - EditorLODSystem::LOD_DISTANCE_INFINITY) < DAVA::EPSILON)
                {
                    break;
                }
                maxDistanceValue = dist;
            }
        }
    }

    ui->forceSlider->setRange(LodComponent::INVALID_DISTANCE, maxDistanceValue);

    QString text = QString("%1 to %2").arg(DAVA::LodComponent::MIN_LOD_DISTANCE).arg(maxDistanceValue);
    ui->forceSlider->setToolTip(text);
}

void LODEditor::UpdateDistanceUI(EditorLODSystem* forSystem, const LODComponentHolder* lodData)
{
    DVASSERT(lodData != nullptr);

    const QSignalBlocker guard(ui->distanceSlider);

    const Vector<float32>& distances = lodData->GetDistances();
    const Vector<bool>& multiple = lodData->GetMultiple();

    int32 count = static_cast<int32>(lodData->GetLODLayersCount());
    ui->distanceSlider->SetLayersCount(count);
    ui->distanceSlider->SetDistances(distances, multiple);

    UpdateDistanceSpinboxesUI(distances, lodData->GetMultiple(), count);
    UpdateTrianglesUI(GetCurrentEditorStatisticsSystem());
    UpdateForceSliderRange();
}

void LODEditor::UpdateActionUI(EditorLODSystem* forSystem)
{
    const bool canDeleteLod = forSystem->CanDeleteLOD();
    for (LODDistanceWidget* dw : distanceWidgets)
    {
        dw->SetCanDelete(canDeleteLod);
    }

    bool canCreateLod = forSystem->CanCreateLOD();
    ui->lastLodToFrontButton->setEnabled(canCreateLod);
    ui->createPlaneLodButton->setEnabled(canCreateLod);
}

void LODEditor::UpdateTrianglesUI(EditorStatisticsSystem* forSystem)
{
    EditorLODSystem* lodSystem = GetCurrentEditorLODSystem();
    const Vector<uint32>& triangles = forSystem->GetTriangles(lodSystem->GetMode(), true);

    int32 index = EditorStatisticsSystem::INDEX_OF_FIRST_LOD_TRIANGLES;
    for (LODDistanceWidget* dw : distanceWidgets)
    {
        dw->SetTrianglesCount(triangles[index]);
        ++index;
    }
}

//ENDOF DELEGATE

EditorLODSystem* LODEditor::GetCurrentEditorLODSystem() const
{
    if (activeScene != nullptr)
    {
        return activeScene->GetSystem<EditorLODSystem>();
    }

    return nullptr;
}

EditorStatisticsSystem* LODEditor::GetCurrentEditorStatisticsSystem() const
{
    if (activeScene != nullptr)
    {
        return activeScene->GetSystem<EditorStatisticsSystem>();
    }

    return nullptr;
}
