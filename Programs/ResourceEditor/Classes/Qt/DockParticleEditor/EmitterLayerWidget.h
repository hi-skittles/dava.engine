#pragma once

#include "Classes/Qt/DockParticleEditor/BaseParticleEditorContentWidget.h"
#include "Classes/Qt/DockParticleEditor/GradientPickerWidget.h"
#include "Classes/Qt/DockParticleEditor/TimeLineWidget.h"
#include "Classes/Qt/Tools/EventFilterDoubleSpinBox/EventFilterDoubleSpinBox.h"

#include <TArc/Utils/QtDelayedExecutor.h>

#include <Scene3D/Lod/LodComponent.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class EmitterLayerWidget : public BaseParticleEditorContentWidget
{
    Q_OBJECT

public:
    enum class eLayerMode
    {
        SUPEREMITTER,
        STRIPE,
        REGULAR
    };

    explicit EmitterLayerWidget(QWidget* parent = 0);

    void Init(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter,
              DAVA::ParticleLayer* layer, bool updateMinimized);

    DAVA::ParticleLayer* GetLayer() const
    {
        return layer;
    };
    void Update(bool updateMinimized);

    bool eventFilter(QObject*, QEvent*) override;

    void StoreVisualState(DAVA::KeyedArchive* visualStateProps) override;
    void RestoreVisualState(DAVA::KeyedArchive* visualStateProps) override;

    // Switch from/to SuperEmitter mode.
    void SetLayerMode(eLayerMode layerMode);

    // Notify the widget layer value is changed.
    void OnLayerValueChanged();

signals:
    void ValueChanged();

protected slots:
    void OnLodsChanged();
    void OnValueChanged();
    void OnFresnelToAlphaChanged();
    void OnLayerMaterialValueChanged();
    void OnFlowPropertiesChanged();
    void OnStripePropertiesChanged();
    void OnNoisePropertiesChanged();
    void OnAlphaRemapPropertiesChanged();
    void OnThreePointGradientPropertiesChanged();
    void OnSpriteBtn();
    void OnSpriteFolderBtn();
    void OnSpritePathChanged(const QString& text);
    void OnSpritePathEdited(const QString& text);
    void OnFlowSpritePathEdited(const QString& text);
    void OnNoiseSpritePathEdited(const QString& text);
    void OnAlphaRemapSpritePathEdited(const QString& text);

    void OnFlowSpriteBtn();
    void OnFlowFolderBtn();
    void OnFlowTexturePathChanged(const QString& text);

    void OnNoiseSpriteBtn();
    void OnNoiseFolderBtn();
    void OnNoiseTexturePathChanged(const QString& text);

    void OnAlphaRemapBtn();
    void OnAlphaRemapFolderBtn();
    void OnAlphaRemapTexturePathChanged(const QString& text);

    void OnPivotPointReset();
    void OnSpriteUpdateTimerExpired();

private:
    void InitWidget(QWidget*);
    void UpdateTooltip(QLineEdit* label);
    void UpdateLayerSprite();
    void UpdateFlowmapSprite();
    void UpdateNoiseSprite();
    void UpdateAlphaRemapSprite();
    void UpdateEditorTexture(DAVA::Sprite* sprite, DAVA::FilePath& filePath, QLineEdit* pathLabel, QLabel* spriteLabel, DAVA::Stack<std::pair<rhi::HSyncObject, DAVA::Texture*>>& textureStack);
    void CreateFlowmapLayoutWidget();
    void CreateNoiseLayoutWidget();
    void CreateStripeLayoutWidget();
    void CreateAlphaRemapLayoutWidget();
    QLayout* CreateFresnelToAlphaLayout();
    void OnChangeSpriteButton(const DAVA::FilePath& initialFilePath, QLineEdit* spriteLabel, QString&& caption, DAVA::Function<void(const QString&)> pathEditFunc);
    void OnChangeFolderButton(const DAVA::FilePath& initialFilePath, QLineEdit* pathLabel, DAVA::Function<void(const QString&)> pathEditFunc);
    void CheckPath(const QString& text);
    void FillLayerTypes();
    DAVA::int32 LayerTypeToIndex(DAVA::ParticleLayer::eType layerType);
    void FillTimeLineWidgetIndentifiers();

private:
    struct LayerTypeMap
    {
        DAVA::ParticleLayer::eType layerType;
        QString layerName;
    };

    struct BlendPreset
    {
        DAVA::eBlending blending;
        QString presetName;
    };

private:
    static const LayerTypeMap layerTypeMap[];
    static const BlendPreset blendPresetsMap[];

    DAVA::ParticleLayer* layer = nullptr;

    QTimer* spriteUpdateTimer = nullptr;
    DAVA::Stack<std::pair<rhi::HSyncObject, DAVA::Texture*>> spriteUpdateTexturesStack;
    DAVA::Stack<std::pair<rhi::HSyncObject, DAVA::Texture*>> flowSpriteUpdateTexturesStack;
    DAVA::Stack<std::pair<rhi::HSyncObject, DAVA::Texture*>> noiseSpriteUpdateTexturesStack;
    DAVA::Stack<std::pair<rhi::HSyncObject, DAVA::Texture*>> alphaRemapSpriteUpdateTexturesStack;

    QVBoxLayout* mainBox = nullptr;
    QVBoxLayout* pivotPointLayout = nullptr;

    QLabel* scaleVelocityBaseLabel = nullptr;
    QLabel* scaleVelocityFactorLabel = nullptr;
    QLabel* fresnelBiasLabel = nullptr;
    QLabel* fresnelPowerLabel = nullptr;
    QLabel* layerTypeLabel = nullptr;

    QLabel* spriteLabel = nullptr;
    QLabel* flowSpriteLabel = nullptr;
    QLabel* noiseSpriteLabel = nullptr;
    QLabel* alphaRemapSpriteLabel = nullptr;

    QLabel* innerEmitterLabel = nullptr;
    QLabel* pivotPointLabel = nullptr;
    QLabel* pivotPointXSpinBoxLabel = nullptr;
    QLabel* pivotPointYSpinBoxLabel = nullptr;
    QLabel* particleOrientationLabel = nullptr;
    QLabel* blendOptionsLabel = nullptr;
    QLabel* presetLabel = nullptr;
    QLabel* frameOverlifeFPSLabel = nullptr;
    QLabel* deltaSpinLabel = nullptr;
    QLabel* deltaVariationSpinLabel = nullptr;
    QLabel* loopEndSpinLabel = nullptr;
    QLabel* loopVariationSpinLabel = nullptr;

    QCheckBox* enableCheckBox = nullptr;
    QCheckBox* applyGlobalForcesCheckBox = nullptr;
    QCheckBox* enableFlowCheckBox = nullptr;
    QCheckBox* enableFlowAnimationCheckBox = nullptr;
    QCheckBox* enableNoiseCheckBox = nullptr;
    QCheckBox* enableAlphaRemapCheckBox = nullptr;
    QCheckBox* enableNoiseScrollCheckBox = nullptr;
    QCheckBox* isLongCheckBox = nullptr;
    QCheckBox* fresnelToAlphaCheckbox = nullptr;
    QCheckBox* isLoopedCheckBox = nullptr;
    QCheckBox* inheritPostionCheckBox = nullptr;
    QCheckBox* layerLodsCheckBox[DAVA::LodComponent::MAX_LOD_LAYERS];
    QCheckBox* frameBlendingCheckBox = nullptr;
    QCheckBox* cameraFacingCheckBox = nullptr;
    QCheckBox* xFacingCheckBox = nullptr;
    QCheckBox* yFacingCheckBox = nullptr;
    QCheckBox* zFacingCheckBox = nullptr;
    QCheckBox* worldAlignCheckBox = nullptr;
    QCheckBox* cameraFacingStripeSphericalCheckBox = nullptr;
    QCheckBox* fogCheckBox = nullptr;
    QCheckBox* frameOverlifeCheckBox = nullptr;
    QCheckBox* randomSpinDirectionCheckBox = nullptr;
    QCheckBox* randomFrameOnStartCheckBox = nullptr;
    QCheckBox* loopSpriteAnimationCheckBox = nullptr;

    QComboBox* degradeStrategyComboBox = nullptr;
    QComboBox* layerTypeComboBox = nullptr;
    QComboBox* presetComboBox = nullptr;

    QLineEdit* layerNameLineEdit = nullptr;
    QLineEdit* spritePathLabel = nullptr;
    QLineEdit* flowSpritePathLabel = nullptr;
    QLineEdit* noiseSpritePathLabel = nullptr;
    QLineEdit* alphaRemapSpritePathLabel = nullptr;
    QLineEdit* innerEmitterPathLabel = nullptr;

    QPushButton* spriteBtn = nullptr;
    QPushButton* spriteFolderBtn = nullptr;
    QPushButton* flowTextureBtn = nullptr;
    QPushButton* flowTextureFolderBtn = nullptr;
    QPushButton* noiseTextureBtn = nullptr;
    QPushButton* noiseTextureFolderBtn = nullptr;
    QPushButton* alphaRemapTextureBtn = nullptr;
    QPushButton* alphaRemapTextureFolderBtn = nullptr;

    QPushButton* pivotPointResetButton = nullptr;

    TimeLineWidget* flowSpeedTimeLine = nullptr;
    TimeLineWidget* flowSpeedVariationTimeLine = nullptr;

    TimeLineWidget* flowOffsetTimeLine = nullptr;
    TimeLineWidget* flowOffsetVariationTimeLine = nullptr;

    TimeLineWidget* noiseScaleTimeLine = nullptr;
    TimeLineWidget* noiseScaleVariationTimeLine = nullptr;
    TimeLineWidget* noiseScaleOverLifeTimeLine = nullptr;
    TimeLineWidget* noiseUVScrollSpeedTimeLine = nullptr;
    TimeLineWidget* noiseUVScrollSpeedVariationTimeLine = nullptr;
    TimeLineWidget* noiseUVScrollSpeedOverLifeTimeLine = nullptr;

    TimeLineWidget* alphaRemapOverLifeTimeLine = nullptr;

    TimeLineWidget* lifeTimeLine = nullptr;
    TimeLineWidget* numberTimeLine = nullptr;
    TimeLineWidget* sizeTimeLine = nullptr;
    TimeLineWidget* sizeVariationTimeLine = nullptr;
    TimeLineWidget* sizeOverLifeTimeLine = nullptr;
    TimeLineWidget* velocityTimeLine = nullptr;
    TimeLineWidget* velocityOverLifeTimeLine = nullptr;
    TimeLineWidget* spinTimeLine = nullptr;
    TimeLineWidget* spinOverLifeTimeLine = nullptr;
    TimeLineWidget* alphaOverLifeTimeLine = nullptr;
    TimeLineWidget* animSpeedOverLifeTimeLine = nullptr;
    TimeLineWidget* angleTimeLine = nullptr;

    EventFilterDoubleSpinBox* scaleVelocityBaseSpinBox = nullptr;
    EventFilterDoubleSpinBox* scaleVelocityFactorSpinBox = nullptr;
    EventFilterDoubleSpinBox* pivotPointXSpinBox = nullptr;
    EventFilterDoubleSpinBox* pivotPointYSpinBox = nullptr;
    EventFilterDoubleSpinBox* startTimeSpin = nullptr;
    EventFilterDoubleSpinBox* endTimeSpin = nullptr;
    EventFilterDoubleSpinBox* deltaSpin = nullptr;
    EventFilterDoubleSpinBox* loopEndSpin = nullptr;
    EventFilterDoubleSpinBox* deltaVariationSpin = nullptr;
    EventFilterDoubleSpinBox* loopVariationSpin = nullptr;
    EventFilterDoubleSpinBox* fresnelBiasSpinBox = nullptr;
    EventFilterDoubleSpinBox* fresnelPowerSpinBox = nullptr;

    EventFilterDoubleSpinBox* alphaRemapLoopCountSpin = nullptr;
    QLabel* alphaRemapLoopLabel = nullptr;

    //////////////////////////////////////////////////////////////////////////
    TimeLineWidget* stripeSizeOverLifeTimeLine = nullptr;
    TimeLineWidget* stripeTextureTileTimeLine = nullptr;
    TimeLineWidget* stripeNoiseScrollSpeedOverLifeTimeLine = nullptr;
    EventFilterDoubleSpinBox* stripeVertexSpawnStepSpin = nullptr;
    EventFilterDoubleSpinBox* stripeLifetimeSpin = nullptr;
    EventFilterDoubleSpinBox* stripeStartSizeSpin = nullptr;
    EventFilterDoubleSpinBox* stripeUScrollSpeedSpin = nullptr;
    EventFilterDoubleSpinBox* stripeVScrollSpeedSpin = nullptr;
    EventFilterDoubleSpinBox* stripeFadeDistanceFromTopSpin = nullptr;
    QLabel* stripeLabel = nullptr;
    QLabel* stripeLifetimeLabel = nullptr;
    QLabel* stripeVertexSpawnStepLabel = nullptr;
    QLabel* stripeStartSizeLabel = nullptr;
    QLabel* stripeUScrollSpeedLabel = nullptr;
    QLabel* stripeVScrollSpeedLabel = nullptr;
    QLabel* stripeFadeDistanceFromTopLabel = nullptr;
    QCheckBox* stripeInheritPositionForBaseCheckBox = nullptr;
    QCheckBox* stripeUsePerspectiveMappingCheckBox = nullptr;
    GradientPickerWidget* stripeColorOverLifeGradient = nullptr;
    //////////////////////////////////////////////////////////////////////////
    QSpinBox* frameOverlifeFPSSpin = nullptr;

    GradientPickerWidget* colorRandomGradient = nullptr;
    GradientPickerWidget* colorOverLifeGradient = nullptr;

    GradientPickerWidget* gradientColorForBlackPicker = nullptr;
    GradientPickerWidget* gradientColorForWhitePicker = nullptr;
    GradientPickerWidget* gradientColorForMiddlePicker = nullptr;
    TimeLineWidget* gradientMiddlePointTimeLine = nullptr;
    QCheckBox* useThreePointGradientBox = nullptr;
    QLabel* gradientMiddlePointLabel = nullptr;
    EventFilterDoubleSpinBox* gradientMiddlePointSpin = nullptr;

    QWidget* flowLayoutWidget = nullptr;
    QWidget* stripeLayoutWidget = nullptr;
    QWidget* flowSettingsLayoutWidget = nullptr;
    QWidget* noiseLayoutWidget = nullptr;
    QWidget* noiseScrollWidget = nullptr;
    QWidget* alphaRemapLayoutWidget = nullptr;

    bool blockSignals = false;
    DAVA::Vector<std::pair<std::string, TimeLineWidget*>> timeLineWidgetsIdentifiers;

    DAVA::QtDelayedExecutor executor;
};
