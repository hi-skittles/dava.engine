#pragma once

#include "LandscapeEditorBasePanel.h"
#include "DAVAEngine.h"

#include "TArc/DataProcessing/DataWrapper.h"
#include "TArc/DataProcessing/DataListener.h"

class QComboBox;
class QPushButton;
class SliderWidget;

class CustomColorsPanel : public LandscapeEditorBasePanel, private DAVA::DataListener
{
    Q_OBJECT

public:
    static const int DEF_BRUSH_MIN_SIZE = 3;
    static const int DEF_BRUSH_MAX_SIZE = 40;

    explicit CustomColorsPanel(QWidget* parent = 0);

private slots:
    void SetBrushSize(int brushSize);
    void SetColor(int color);
    bool SaveTexture();
    void LoadTexture();

    void IncreaseBrushSize();
    void DecreaseBrushSize();
    void IncreaseBrushSizeLarge();
    void DecreaseBrushSizeLarge();

    void PrevTexture();
    void NextTexture();

protected:
    bool GetEditorEnabled() override;

    void SetWidgetsState(bool enabled) override;
    void BlockAllSignals(bool block) override;

    void InitUI() override;
    void ConnectToSignals() override;

    void StoreState() override;
    void RestoreState() override;

    void ConnectToShortcuts() override;
    void DisconnectFromShortcuts() override;

private:
    void InitColors();

    DAVA::int32 BrushSizeUIToSystem(DAVA::int32 uiValue);
    DAVA::int32 BrushSizeSystemToUI(DAVA::int32 systemValue);

    void OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields) override;

private:
    QComboBox* comboColor = nullptr;
    SliderWidget* sliderWidgetBrushSize = nullptr;
    QPushButton* buttonSaveTexture = nullptr;
    QPushButton* buttonLoadTexture = nullptr;
    DAVA::DataWrapper projectDataWrapper;
};
