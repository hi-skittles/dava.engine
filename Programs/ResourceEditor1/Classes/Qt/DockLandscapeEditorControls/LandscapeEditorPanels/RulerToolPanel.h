#ifndef __RESOURCEEDITORQT__RULERTOOLPANEL__
#define __RESOURCEEDITORQT__RULERTOOLPANEL__

#include "LandscapeEditorBasePanel.h"
#include "DAVAEngine.h"

class QLabel;
class SliderWidget;

class RulerToolPanel : public LandscapeEditorBasePanel
{
    Q_OBJECT

public:
    explicit RulerToolPanel(QWidget* parent = 0);
    ~RulerToolPanel();

private slots:
    void UpdateLengths(SceneEditor2* scene, double length, double previewLength);

protected:
    virtual bool GetEditorEnabled();

    virtual void SetWidgetsState(bool enabled);
    virtual void BlockAllSignals(bool block);

    virtual void InitUI();
    virtual void ConnectToSignals();

    virtual void StoreState();
    virtual void RestoreState();

    virtual void ConnectToShortcuts();
    virtual void DisconnectFromShortcuts();

private:
    QLabel* labelLength;
    QLabel* labelPreview;
};

#endif /* defined(__RESOURCEEDITORQT__RULERTOOLPANEL__) */
