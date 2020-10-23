#ifndef __RESOURCEEDITORQT__LANDSCAPEEDITORCONTROLSPLACEHOLDER__
#define __RESOURCEEDITORQT__LANDSCAPEEDITORCONTROLSPLACEHOLDER__

#include <QWidget>
#include "DAVAEngine.h"

#include "LandscapeEditorPanels/LandscapeEditorBasePanel.h"

class CustomColorsPanel;
class RulerToolPanel;
class TilemaskEditorPanel;
class HeightmapEditorPanel;

class LandscapeEditorControlsPlaceholder : public QWidget
{
    Q_OBJECT

public:
    explicit LandscapeEditorControlsPlaceholder(QWidget* parent = 0);
    ~LandscapeEditorControlsPlaceholder();

    void SetPanel(LandscapeEditorBasePanel* panel);
    void RemovePanel();

public slots:
    void SceneActivated(DAVA::SceneEditor2* scene);
    void SceneDeactivated(DAVA::SceneEditor2* scene);
    void OnOpenGLInitialized();

private slots:
    void EditorToggled(DAVA::SceneEditor2* scene);

private:
    DAVA::SceneEditor2* activeScene;
    LandscapeEditorBasePanel* currentPanel;

    CustomColorsPanel* customColorsPanel;
    RulerToolPanel* rulerToolPanel;
    TilemaskEditorPanel* tilemaskEditorPanel;
    HeightmapEditorPanel* heightmapEditorPanel;

    void InitUI();
    void ConnectToSignals();
    void CreatePanels();

    void UpdatePanels();
};

#endif /* defined(__RESOURCEEDITORQT__LANDSCAPEEDITORCONTROLSPLACEHOLDER__) */
