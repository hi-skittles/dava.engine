#ifndef __RESOURCEEDITORQT__TILEMASKEDITORPANEL__
#define __RESOURCEEDITORQT__TILEMASKEDITORPANEL__

#include "LandscapeEditorBasePanel.h"
#include "DAVAEngine.h"

#include "Render/UniqueStateSet.h"

namespace DAVA
{
class RECommandNotificationObject;
} // namespace DAVA

class QComboBox;
class QRadioButton;
class SliderWidget;
class TileTexturePreviewWidget;
class QFrame;

class TilemaskEditorPanel : public LandscapeEditorBasePanel
{
    Q_OBJECT

public:
    static const int DEF_BRUSH_MIN_SIZE = 1;
    static const int DEF_BRUSH_MAX_SIZE = 40;
    static const int DEF_STRENGTH_MIN_VALUE = 0;
    static const int DEF_STRENGTH_MAX_VALUE = 60;
    static const int STRENGTH_MAX_BOUNDARY = 999;

    explicit TilemaskEditorPanel(QWidget* parent = 0);
    virtual ~TilemaskEditorPanel();

private slots:
    void SetBrushSize(int brushSize);
    void SetToolImage(int imageIndex);
    void SetStrength(int strength);
    void SetDrawTexture(int textureIndex);
    void OnTileColorChanged(DAVA::int32 tileNumber, DAVA::Color color);
    void SetNormalDrawing();
    void SetCopyPaste();

    void IncreaseBrushSize();
    void DecreaseBrushSize();
    void IncreaseBrushSizeLarge();
    void DecreaseBrushSizeLarge();

    void IncreaseStrength();
    void DecreaseStrength();
    void IncreaseStrengthLarge();
    void DecreaseStrengthLarge();

    void PrevTexture();
    void NextTexture();

    void PrevTool();
    void NextTool();

    void OnCommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification);

protected:
    virtual bool GetEditorEnabled();

    virtual void OnEditorEnabled();

    virtual void SetWidgetsState(bool enabled);
    virtual void BlockAllSignals(bool block);

    virtual void InitUI();
    virtual void ConnectToSignals();

    virtual void StoreState();
    virtual void RestoreState();

    virtual void ConnectToShortcuts();
    virtual void DisconnectFromShortcuts();

private:
    SliderWidget* sliderWidgetBrushSize;
    SliderWidget* sliderWidgetStrength;
    QComboBox* comboBrushImage;
    TileTexturePreviewWidget* tileTexturePreviewWidget;
    QRadioButton* radioDraw;
    QRadioButton* radioCopyPaste;
    QFrame* frameStrength;
    QFrame* frameTileTexturesPreview;

    void InitBrushImages();
    void UpdateTileTextures();

    void UpdateControls();

    DAVA::int32 BrushSizeUIToSystem(DAVA::int32 uiValue);
    DAVA::int32 BrushSizeSystemToUI(DAVA::int32 systemValue);

    DAVA::float32 StrengthUIToSystem(DAVA::int32 uiValue);
    DAVA::int32 StrengthSystemToUI(DAVA::float32 systemValue);

    void SplitImageToChannels(DAVA::Image* image, DAVA::Image*& r, DAVA::Image*& g, DAVA::Image*& b, DAVA::Image*& a);
};

#endif /* defined(__RESOURCEEDITORQT__TILEMASKEDITORPANEL__) */
