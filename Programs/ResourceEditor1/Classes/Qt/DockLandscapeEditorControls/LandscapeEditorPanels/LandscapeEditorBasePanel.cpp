#include "LandscapeEditorBasePanel.h"

#include "../../Scene/SceneSignals.h"
#include "../../Scene/SceneEditor2.h"

LandscapeEditorBasePanel::LandscapeEditorBasePanel(QWidget* parent)
    : QWidget(parent)
    , activeScene(NULL)
{
}

LandscapeEditorBasePanel::~LandscapeEditorBasePanel()
{
}

SceneEditor2* LandscapeEditorBasePanel::GetActiveScene()
{
    return activeScene;
}

void LandscapeEditorBasePanel::OnEditorEnabled()
{
}

void LandscapeEditorBasePanel::OnEditorDisabled()
{
}

void LandscapeEditorBasePanel::InitPanel(SceneEditor2* scene)
{
    activeScene = scene;

    bool enabled = GetEditorEnabled();
    SetWidgetsState(enabled);
    BlockAllSignals(!enabled);
    RestoreState();
    ConnectToShortcuts();
}

void LandscapeEditorBasePanel::DeinitPanel()
{
    StoreState();
    SetWidgetsState(false);
    BlockAllSignals(true);
    DisconnectFromShortcuts();

    activeScene = NULL;
}

void LandscapeEditorBasePanel::EditorToggled(SceneEditor2* scene)
{
    if (scene != GetActiveScene())
    {
        return;
    }

    if (GetEditorEnabled())
    {
        OnEditorEnabled();
    }
    else
    {
        OnEditorDisabled();
    }
}

void LandscapeEditorBasePanel::ConnectToShortcuts()
{
}

void LandscapeEditorBasePanel::DisconnectFromShortcuts()
{
}
