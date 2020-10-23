#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorControlsPlaceholder.h"
#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorPanels/CustomColorsPanel.h"
#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorPanels/RulerToolPanel.h"
#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorPanels/TilemaskEditorPanel.h"
#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorPanels/HeightmapEditorPanel.h"
#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/Scene/SceneEditor2.h>

#include <QVBoxLayout>

LandscapeEditorControlsPlaceholder::LandscapeEditorControlsPlaceholder(QWidget* parent)
    : QWidget(parent)
    , activeScene(nullptr)
    , currentPanel(nullptr)
    , customColorsPanel(nullptr)
    , rulerToolPanel(nullptr)
    , tilemaskEditorPanel(nullptr)
    , heightmapEditorPanel(nullptr)
{
    InitUI();
    ConnectToSignals();
}

void LandscapeEditorControlsPlaceholder::OnOpenGLInitialized()
{
    DVASSERT(!customColorsPanel && !rulerToolPanel && !tilemaskEditorPanel && !heightmapEditorPanel);

    customColorsPanel = new CustomColorsPanel();
    rulerToolPanel = new RulerToolPanel();
    tilemaskEditorPanel = new TilemaskEditorPanel();
    heightmapEditorPanel = new HeightmapEditorPanel();
}

LandscapeEditorControlsPlaceholder::~LandscapeEditorControlsPlaceholder()
{
    DAVA::SafeDelete(customColorsPanel);
    DAVA::SafeDelete(rulerToolPanel);
    DAVA::SafeDelete(tilemaskEditorPanel);
    DAVA::SafeDelete(heightmapEditorPanel);
}

void LandscapeEditorControlsPlaceholder::InitUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);
}

void LandscapeEditorControlsPlaceholder::ConnectToSignals()
{
    connect(SceneSignals::Instance(), SIGNAL(Activated(DAVA::SceneEditor2*)), this, SLOT(SceneActivated(DAVA::SceneEditor2*)));
    connect(SceneSignals::Instance(), SIGNAL(Deactivated(DAVA::SceneEditor2*)), this, SLOT(SceneDeactivated(DAVA::SceneEditor2*)));

    connect(SceneSignals::Instance(), &SceneSignals::LandscapeEditorToggled, this, &LandscapeEditorControlsPlaceholder::EditorToggled);
}

void LandscapeEditorControlsPlaceholder::SceneActivated(DAVA::SceneEditor2* scene)
{
    activeScene = scene;

    UpdatePanels();
}

void LandscapeEditorControlsPlaceholder::SceneDeactivated(DAVA::SceneEditor2* scene)
{
    RemovePanel();

    activeScene = NULL;
}

void LandscapeEditorControlsPlaceholder::SetPanel(LandscapeEditorBasePanel* panel)
{
    if (!panel || panel == currentPanel)
    {
        return;
    }

    RemovePanel();

    currentPanel = panel;
    layout()->addWidget(panel);
    panel->show();

    panel->InitPanel(activeScene);
}

void LandscapeEditorControlsPlaceholder::RemovePanel()
{
    if (!currentPanel)
    {
        return;
    }

    currentPanel->DeinitPanel();

    currentPanel->hide();
    layout()->removeWidget(currentPanel);
    currentPanel->setParent(NULL);
    currentPanel = NULL;
}

void LandscapeEditorControlsPlaceholder::EditorToggled(DAVA::SceneEditor2* scene)
{
    if (scene != activeScene)
    {
        return;
    }

    UpdatePanels();
}

void LandscapeEditorControlsPlaceholder::UpdatePanels()
{
    RemovePanel();

    DAVA::int32 tools = activeScene->GetEnabledTools();
    if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_CUSTOM_COLOR)
    {
        SetPanel(customColorsPanel);
    }
    else if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_HEIGHTMAP_EDITOR)
    {
        SetPanel(heightmapEditorPanel);
    }
    else if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_RULER)
    {
        SetPanel(rulerToolPanel);
    }
    else if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_TILEMAP_EDITOR)
    {
        SetPanel(tilemaskEditorPanel);
    }
}
