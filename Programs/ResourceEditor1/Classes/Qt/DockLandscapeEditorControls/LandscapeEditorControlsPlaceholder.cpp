#include "LandscapeEditorControlsPlaceholder.h"
#include "../Scene/SceneSignals.h"
#include "../Scene/SceneEditor2.h"

#include "LandscapeEditorPanels/CustomColorsPanel.h"
#include "LandscapeEditorPanels/RulerToolPanel.h"
#include "LandscapeEditorPanels/TilemaskEditorPanel.h"
#include "LandscapeEditorPanels/HeightmapEditorPanel.h"

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
    connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2*)), this, SLOT(SceneActivated(SceneEditor2*)));
    connect(SceneSignals::Instance(), SIGNAL(Deactivated(SceneEditor2*)), this, SLOT(SceneDeactivated(SceneEditor2*)));

    connect(SceneSignals::Instance(), SIGNAL(LandscapeEditorToggled(SceneEditor2*)),
            this, SLOT(EditorToggled(SceneEditor2*)));
}

void LandscapeEditorControlsPlaceholder::SceneActivated(SceneEditor2* scene)
{
    activeScene = scene;

    UpdatePanels();
}

void LandscapeEditorControlsPlaceholder::SceneDeactivated(SceneEditor2* scene)
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

void LandscapeEditorControlsPlaceholder::EditorToggled(SceneEditor2* scene)
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
    if (tools & SceneEditor2::LANDSCAPE_TOOL_CUSTOM_COLOR)
    {
        SetPanel(customColorsPanel);
    }
    else if (tools & SceneEditor2::LANDSCAPE_TOOL_HEIGHTMAP_EDITOR)
    {
        SetPanel(heightmapEditorPanel);
    }
    else if (tools & SceneEditor2::LANDSCAPE_TOOL_RULER)
    {
        SetPanel(rulerToolPanel);
    }
    else if (tools & SceneEditor2::LANDSCAPE_TOOL_TILEMAP_EDITOR)
    {
        SetPanel(tilemaskEditorPanel);
    }
}
