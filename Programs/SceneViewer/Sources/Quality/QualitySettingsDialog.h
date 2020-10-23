#pragma once

#include "Settings.h"

#include "UIControls/TriggerBox.h"
#include "UIControls/BinaryTriggerBox.h"

#include <UI/UIControl.h>
#include <UI/UIListDelegate.h>
#include <UI/UIButton.h>
#include <Render/2D/Font.h>

class QualitySettingsDialogDelegate
{
public:
    virtual ~QualitySettingsDialogDelegate()
    {
    }
    virtual void OnQualitySettingsEditDone() = 0;
};

namespace DAVA
{
class Scene;
}

class QualitySettingsDialog final
: public DAVA::UIControl,
  public DAVA::UIListDelegate,
  public TriggerBoxListener
{
public:
    QualitySettingsDialog(Settings& settings);
    ~QualitySettingsDialog();

    void SetParentControl(UIControl*);
    void SetDelegate(QualitySettingsDialogDelegate*);

    void SetCurrentScene(DAVA::Scene*);

    void Show();

private:
    // UIListDelegate
    DAVA::float32 CellHeight(DAVA::UIList* list, DAVA::int32 index) override;
    DAVA::int32 ElementsCount(DAVA::UIList* list) override;
    DAVA::UIListCell* CellAtIndex(DAVA::UIList* list, DAVA::int32 index) override;

    // ExclusiveSetListener
    void OnOptionChanged(TriggerBox*) override;

    void OnButtonOk(DAVA::BaseObject* caller, void* param, void* callerData);
    void OnButtonCancel(DAVA::BaseObject* caller, void* param, void* callerData);
    void OnButtonApply(DAVA::BaseObject* caller, void* param, void* callerData);

    void BuildQualityControls();
    void ApplyQualitySettings();
    void ResetQualitySettings();

    void ApplyTextureQuality();
    void ApplyMaterialQuality();
    void ApplyParticlesQuality();
    void ReloadEntityEmitters(DAVA::Entity*);

private:
    Settings& settings;

    DAVA::UIControl* parentControl = nullptr;
    QualitySettingsDialogDelegate* delegate = nullptr;

    DAVA::Scene* scene = nullptr;

    DAVA::float32 cellHeight = 0.f;
    DAVA::ScopedPtr<DAVA::Font> font = nullptr;
    DAVA::float32 fontSize = 14.f;

    struct CellData
    {
        CellData(DAVA::UIListCell* cell)
            : cell(cell)
        {
        }
        DAVA::UIListCell* cell = nullptr;
        bool wasRequested = false;
    };
    DAVA::Vector<CellData> cells;

    DAVA::ScopedPtr<TriggerBox> textureQualityBox;
    DAVA::ScopedPtr<TriggerBox> anisotropyQualityBox;
    DAVA::ScopedPtr<TriggerBox> multisamplingQualityBox;
    DAVA::Vector<DAVA::ScopedPtr<TriggerBox>> materialQualityBoxes;
    DAVA::ScopedPtr<TriggerBox> particleQualityBox;
    DAVA::Vector<DAVA::ScopedPtr<BinaryTriggerBox>> qualityOptionBoxes;

    DAVA::ScopedPtr<DAVA::UIButton> applyButton;
    DAVA::ScopedPtr<DAVA::UIStaticText> captionText;
    DAVA::ScopedPtr<DAVA::UIList> scrollableOptionsList;
};
