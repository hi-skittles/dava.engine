#ifndef __RESOURCEEDITORQT__HEIGHTMAPEDITORPANEL__
#define __RESOURCEEDITORQT__HEIGHTMAPEDITORPANEL__

#include "LandscapeEditorBasePanel.h"
#include "DAVAEngine.h"

#include <REPlatform/Scene/Systems/HeightmapEditorSystem.h>

class QComboBox;
class QRadioButton;
class QLineEdit;
class SliderWidget;
class QDoubleSpinBox;

class HeightmapEditorPanel : public LandscapeEditorBasePanel
{
    Q_OBJECT

public:
    static const int DEF_BRUSH_MIN_SIZE = 3;
    static const int DEF_BRUSH_MAX_SIZE = 40;
    static const int DEF_STRENGTH_MAX_VALUE = 30;
    static const int DEF_AVERAGE_STRENGTH_MIN_VALUE = 0;
    static const int DEF_AVERAGE_STRENGTH_MAX_VALUE = 60;
    static const int STRENGTH_MIN_BOUNDARY = -999;
    static const int STRENGTH_MAX_BOUNDARY = 999;
    static const int AVG_STRENGTH_MIN_BOUNDARY = 0;
    static const int AVG_STRENGTH_MAX_BOUNDARY = 999;

    explicit HeightmapEditorPanel(QWidget* parent = 0);
    ~HeightmapEditorPanel();

private slots:
    void SetDropperHeight(DAVA::SceneEditor2* scene, double height);
    void HeightUpdatedManually();

    void SetBrushSize(int brushSize);
    void SetToolImage(int toolImage);
    void SetRelativeDrawing();
    void SetAverageDrawing();
    void SetAbsoluteDrawing();
    void SetAbsDropDrawing();
    void SetDropper();
    void SetHeightmapCopyPaste();
    void SetStrength(int strength);
    void SetAverageStrength(int averageStrength);

    void IncreaseBrushSize();
    void DecreaseBrushSize();
    void IncreaseBrushSizeLarge();
    void DecreaseBrushSizeLarge();

    void IncreaseStrength();
    void DecreaseStrength();
    void IncreaseStrengthLarge();
    void DecreaseStrengthLarge();

    void IncreaseAvgStrength();
    void DecreaseAvgStrength();
    void IncreaseAvgStrengthLarge();
    void DecreaseAvgStrengthLarge();

    void PrevTool();
    void NextTool();

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

    bool eventFilter(QObject* o, QEvent* e);

private:
    SliderWidget* sliderWidgetBrushSize;
    SliderWidget* sliderWidgetStrength;
    SliderWidget* sliderWidgetAverageStrength;
    QComboBox* comboBrushImage;
    QRadioButton* radioCopyPaste;
    QRadioButton* radioAbsDrop;
    QRadioButton* radioAbsolute;
    QRadioButton* radioAverage;
    QRadioButton* radioDropper;
    QRadioButton* radioRelative;
    QDoubleSpinBox* editHeight;

    void InitBrushImages();
    void UpdateRadioState(DAVA::HeightmapEditorSystem::eHeightmapDrawType type);
    void SetDrawingType(DAVA::HeightmapEditorSystem::eHeightmapDrawType type);

    DAVA::float32 GetBrushScaleCoef();
    DAVA::int32 BrushSizeUIToSystem(DAVA::int32 uiValue);
    DAVA::int32 BrushSizeSystemToUI(DAVA::int32 systemValue);

    DAVA::float32 StrengthUIToSystem(DAVA::int32 uiValue);
    DAVA::int32 StrengthSystemToUI(DAVA::float32 systemValue);

    DAVA::float32 AverageStrengthUIToSystem(DAVA::int32 uiValue);
    DAVA::int32 AverageStrengthSystemToUI(DAVA::float32 systemValue);
};

#endif /* defined(__RESOURCEEDITORQT__HEIGHTMAPEDITORPANEL__) */
