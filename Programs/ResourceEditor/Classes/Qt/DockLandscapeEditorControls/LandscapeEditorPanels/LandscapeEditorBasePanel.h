#pragma once

#include <QWidget>

namespace DAVA
{
class SceneEditor2;
} // namespace DAVA

class LandscapeEditorBasePanel : public QWidget
{
    Q_OBJECT

public:
    explicit LandscapeEditorBasePanel(QWidget* parent = 0);
    virtual ~LandscapeEditorBasePanel();

    void InitPanel(DAVA::SceneEditor2* scene);
    void DeinitPanel();

protected slots:
    virtual void EditorToggled(DAVA::SceneEditor2* scene);

protected:
    virtual void OnEditorEnabled();
    virtual void OnEditorDisabled();

    DAVA::SceneEditor2* GetActiveScene();
    virtual bool GetEditorEnabled() = 0;

    virtual void SetWidgetsState(bool enabled) = 0;
    virtual void BlockAllSignals(bool block) = 0;

    virtual void InitUI() = 0;
    virtual void ConnectToSignals() = 0;

    virtual void StoreState() = 0;
    virtual void RestoreState() = 0;

    virtual void ConnectToShortcuts();
    virtual void DisconnectFromShortcuts();

private:
    DAVA::SceneEditor2* activeScene;
};
