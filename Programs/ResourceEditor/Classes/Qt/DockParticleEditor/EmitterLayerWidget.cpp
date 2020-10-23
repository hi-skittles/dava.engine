#include "Classes/Qt/DockParticleEditor/EmitterLayerWidget.h"
#include "Classes/Qt/DockParticleEditor/WheellIgnorantComboBox.h"
#include "Classes/Qt/TextureBrowser/TextureConvertor.h"

#include <REPlatform/Commands/ParticleLayerCommands.h>
#include <REPlatform/Commands/ParticleEditorCommands.h>
#include <REPlatform/Commands/ParticleLayerCommands.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Scene/Utils/ImageTools.h>

#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Core/Deprecated.h>

#include <Base/Result.h>
#include <Render/2D/Sprite.h>

#include <QFile>
#include <QGraphicsWidget>
#include <QHBoxLayout>

namespace EmitterLayerWidgetDetails
{
QString ConvertSpritePathToPSD(QString& pathToSprite)
{
    return pathToSprite.replace("/Data/", "/DataSource/");
}

QString ConvertPSDPathToSprite(QString& pathToSprite)
{
    return pathToSprite.replace("/DataSource/", "/Data/");
}
}

static const DAVA::uint32 SPRITE_SIZE = 60;

static const DAVA::float32 ANGLE_MIN_LIMIT_DEGREES = -360.0f;
static const DAVA::float32 ANGLE_MAX_LIMIT_DEGREES = 360.0f;

static const DAVA::int32 NOISE_PRECISION_DIGITS = 4;
static const DAVA::int32 FLOW_PRECISION_DIGITS = 4;
static const DAVA::int32 STRIPE_TILE_PRECISION_DIGITS = 4;

const EmitterLayerWidget::LayerTypeMap EmitterLayerWidget::layerTypeMap[] =
{
  { DAVA::ParticleLayer::TYPE_SINGLE_PARTICLE, "Single Particle" },
  { DAVA::ParticleLayer::TYPE_PARTICLES, "Particles" },
  { DAVA::ParticleLayer::TYPE_PARTICLE_STRIPE, "Particle Stripe" },
  { DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES, "SuperEmitter" }
};

const EmitterLayerWidget::BlendPreset EmitterLayerWidget::blendPresetsMap[] =
{
  { DAVA::BLENDING_ALPHABLEND, "Alpha blend" },
  { DAVA::BLENDING_ADDITIVE, "Additive" },
  { DAVA::BLENDING_ALPHA_ADDITIVE, "Alpha additive" },
  { DAVA::BLENDING_SOFT_ADDITIVE, "Soft additive" },
  /*{BLEND_DST_COLOR, BLEND_ZERO, "Multiplicative"},
    {BLEND_DST_COLOR, BLEND_SRC_COLOR, "2x Multiplicative"}*/
};

EmitterLayerWidget::EmitterLayerWidget(QWidget* parent)
    : BaseParticleEditorContentWidget(parent)
{
    mainBox = new QVBoxLayout;
    this->setLayout(mainBox);

    layerNameLineEdit = new QLineEdit();
    mainBox->addWidget(layerNameLineEdit);
    connect(layerNameLineEdit, SIGNAL(editingFinished()), this, SLOT(OnValueChanged()));

    QVBoxLayout* lodsLayout = new QVBoxLayout();
    QLabel* lodsLabel = new QLabel("Active in LODs", this);
    lodsLayout->addWidget(lodsLabel);
    QHBoxLayout* lodsInnerLayout = new QHBoxLayout();

    for (DAVA::int32 i = 0; i < DAVA::LodComponent::MAX_LOD_LAYERS; ++i)
    {
        layerLodsCheckBox[i] = new QCheckBox(QString("LOD") + QString::number(i));
        lodsInnerLayout->addWidget(layerLodsCheckBox[i]);
        connect(layerLodsCheckBox[i],
                SIGNAL(stateChanged(int)),
                this,
                SLOT(OnLodsChanged()));
    }
    lodsLayout->addLayout(lodsInnerLayout);

    QHBoxLayout* lodsDegradeLayout = new QHBoxLayout();
    lodsDegradeLayout->addWidget(new QLabel("Lod0 degrade strategy"));
    degradeStrategyComboBox = new WheellIgnorantComboBox();
    degradeStrategyComboBox->addItem("Keep everything");
    degradeStrategyComboBox->addItem("Reduce particles");
    degradeStrategyComboBox->addItem("Clear");
    connect(degradeStrategyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnValueChanged()));
    lodsDegradeLayout->addWidget(degradeStrategyComboBox);
    lodsLayout->addLayout(lodsDegradeLayout);
    mainBox->addLayout(lodsLayout);

    layerTypeLabel = new QLabel(this);
    layerTypeLabel->setText("Layer type");
    mainBox->addWidget(layerTypeLabel);

    layerTypeComboBox = new WheellIgnorantComboBox(this);
    FillLayerTypes();
    mainBox->addWidget(layerTypeComboBox);
    connect(layerTypeComboBox,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(OnValueChanged()));

    enableCheckBox = new QCheckBox("Enable layer");
    mainBox->addWidget(enableCheckBox);
    connect(enableCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnValueChanged()));

    inheritPostionCheckBox = new QCheckBox("Inherit Position");
    mainBox->addWidget(inheritPostionCheckBox);
    connect(inheritPostionCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnValueChanged()));

    applyGlobalForcesCheckBox = new QCheckBox("Apply Global Forces");
    mainBox->addWidget(applyGlobalForcesCheckBox);
    connect(applyGlobalForcesCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnValueChanged()));

    QHBoxLayout* longLayout = new QHBoxLayout();
    isLongCheckBox = new QCheckBox("Long");
    longLayout->addWidget(isLongCheckBox);
    connect(isLongCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnValueChanged()));

    scaleVelocityBaseSpinBox = new EventFilterDoubleSpinBox();
    scaleVelocityBaseSpinBox->setMinimum(-100);
    scaleVelocityBaseSpinBox->setMaximum(100);
    scaleVelocityBaseSpinBox->setSingleStep(0.1);
    scaleVelocityBaseSpinBox->setDecimals(3);

    scaleVelocityFactorSpinBox = new EventFilterDoubleSpinBox();
    scaleVelocityFactorSpinBox->setMinimum(-100);
    scaleVelocityFactorSpinBox->setMaximum(100);
    scaleVelocityFactorSpinBox->setSingleStep(0.1);
    scaleVelocityFactorSpinBox->setDecimals(3);
    scaleVelocityBaseLabel = new QLabel("Velocity scale base: ");
    scaleVelocityFactorLabel = new QLabel("Velocity scale factor: ");
    longLayout->addWidget(scaleVelocityBaseLabel);
    longLayout->addWidget(scaleVelocityBaseSpinBox);
    longLayout->addWidget(scaleVelocityFactorLabel);
    longLayout->addWidget(scaleVelocityFactorSpinBox);
    connect(scaleVelocityBaseSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnValueChanged()));
    connect(scaleVelocityFactorSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnValueChanged()));
    mainBox->addLayout(longLayout);

    QHBoxLayout* spriteHBox2 = new QHBoxLayout;
    spriteBtn = new QPushButton("Set sprite", this);
    spriteBtn->setMinimumHeight(30);
    spriteFolderBtn = new QPushButton("Change sprite folder", this);
    spriteFolderBtn->setMinimumHeight(30);
    spriteHBox2->addWidget(spriteBtn);
    spriteHBox2->addWidget(spriteFolderBtn);

    QVBoxLayout* spriteVBox = new QVBoxLayout;
    spritePathLabel = new QLineEdit(this);
    spritePathLabel->setReadOnly(false);
    spriteVBox->addLayout(spriteHBox2);
    spriteVBox->addWidget(spritePathLabel);

    QHBoxLayout* spriteHBox = new QHBoxLayout;
    spriteLabel = new QLabel(this);
    spriteLabel->setMinimumSize(SPRITE_SIZE, SPRITE_SIZE);
    spriteHBox->addWidget(spriteLabel);
    spriteHBox->addLayout(spriteVBox);

    mainBox->addLayout(spriteHBox);

    enableFlowCheckBox = new QCheckBox("Enable flowmap");
    mainBox->addWidget(enableFlowCheckBox);
    connect(enableFlowCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnFlowPropertiesChanged()));
    CreateFlowmapLayoutWidget();
    mainBox->addWidget(flowLayoutWidget);

    enableNoiseCheckBox = new QCheckBox("Enable noise");
    mainBox->addWidget(enableNoiseCheckBox);
    connect(enableNoiseCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnNoisePropertiesChanged()));
    CreateNoiseLayoutWidget();
    mainBox->addWidget(noiseLayoutWidget);

    enableAlphaRemapCheckBox = new QCheckBox("Enable alpha remap");
    mainBox->addWidget(enableAlphaRemapCheckBox);
    connect(enableAlphaRemapCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnAlphaRemapPropertiesChanged()));
    CreateAlphaRemapLayoutWidget();
    mainBox->addWidget(alphaRemapLayoutWidget);

    connect(spriteBtn, SIGNAL(clicked(bool)), this, SLOT(OnSpriteBtn()));
    connect(spriteFolderBtn, SIGNAL(clicked(bool)), this, SLOT(OnSpriteFolderBtn()));
    connect(spritePathLabel, SIGNAL(textChanged(const QString&)), this, SLOT(OnSpritePathChanged(const QString&)));
    connect(spritePathLabel, SIGNAL(textEdited(const QString&)), this, SLOT(OnSpritePathEdited(const QString&)));

    QVBoxLayout* innerEmitterLayout = new QVBoxLayout();
    innerEmitterLabel = new QLabel("Inner Emitter", this);
    innerEmitterPathLabel = new QLineEdit(this);
    innerEmitterPathLabel->setReadOnly(true);
    innerEmitterLayout->addWidget(innerEmitterLabel);
    innerEmitterLayout->addWidget(innerEmitterPathLabel);
    mainBox->addLayout(innerEmitterLayout);

    QVBoxLayout* pivotPointLayout = new QVBoxLayout();
    pivotPointLabel = new QLabel("Pivot Point", this);
    pivotPointLayout->addWidget(pivotPointLabel);
    QHBoxLayout* pivotPointInnerLayout = new QHBoxLayout();

    pivotPointXSpinBoxLabel = new QLabel("X:", this);
    pivotPointInnerLayout->addWidget(pivotPointXSpinBoxLabel);
    pivotPointXSpinBox = new EventFilterDoubleSpinBox(this);
    pivotPointXSpinBox->setMinimum(-99);
    pivotPointXSpinBox->setMaximum(99);
    pivotPointXSpinBox->setSingleStep(0.1);
    pivotPointXSpinBox->setDecimals(3);
    pivotPointInnerLayout->addWidget(pivotPointXSpinBox);

    pivotPointYSpinBoxLabel = new QLabel("Y:", this);
    pivotPointInnerLayout->addWidget(pivotPointYSpinBoxLabel);
    pivotPointYSpinBox = new EventFilterDoubleSpinBox(this);
    pivotPointYSpinBox->setMinimum(-99);
    pivotPointYSpinBox->setMaximum(99);
    pivotPointYSpinBox->setSingleStep(0.1);
    pivotPointYSpinBox->setDecimals(3);
    pivotPointInnerLayout->addWidget(pivotPointYSpinBox);

    pivotPointResetButton = new QPushButton("Reset", this);
    pivotPointInnerLayout->addWidget(pivotPointResetButton);
    connect(pivotPointResetButton, SIGNAL(clicked(bool)), this, SLOT(OnPivotPointReset()));

    connect(pivotPointXSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnValueChanged()));
    connect(pivotPointYSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnValueChanged()));

    pivotPointLayout->addLayout(pivotPointInnerLayout);
    mainBox->addLayout(pivotPointLayout);

    frameBlendingCheckBox = new QCheckBox("Enable frame blending");
    connect(frameBlendingCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnLayerMaterialValueChanged()));
    mainBox->addWidget(frameBlendingCheckBox);

    //particle orieantation
    QVBoxLayout* orientationLayout = new QVBoxLayout();
    particleOrientationLabel = new QLabel("Particle Orientation");
    orientationLayout->addWidget(particleOrientationLabel);
    QHBoxLayout* facingLayout = new QHBoxLayout();

    cameraFacingCheckBox = new QCheckBox("Camera Facing");
    facingLayout->addWidget(cameraFacingCheckBox);
    connect(cameraFacingCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));

    xFacingCheckBox = new QCheckBox("X-Facing");
    facingLayout->addWidget(xFacingCheckBox);
    connect(xFacingCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));

    yFacingCheckBox = new QCheckBox("Y-Facing");
    facingLayout->addWidget(yFacingCheckBox);
    connect(yFacingCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));

    zFacingCheckBox = new QCheckBox("Z-Facing");
    facingLayout->addWidget(zFacingCheckBox);
    connect(zFacingCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));

    cameraFacingStripeSphericalCheckBox = new QCheckBox("Camera Facing Spherical");
    facingLayout->addWidget(cameraFacingStripeSphericalCheckBox);
    connect(cameraFacingStripeSphericalCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));

    orientationLayout->addLayout(facingLayout);

    worldAlignCheckBox = new QCheckBox("World Align");
    orientationLayout->addWidget(worldAlignCheckBox);
    connect(worldAlignCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));

    mainBox->addLayout(orientationLayout);
    mainBox->addLayout(CreateFresnelToAlphaLayout());

    blendOptionsLabel = new QLabel("Blending Options");
    mainBox->addWidget(blendOptionsLabel);

    presetLabel = new QLabel("Preset");

    presetComboBox = new WheellIgnorantComboBox();
    presetComboBox->setFocusPolicy(Qt::StrongFocus);
    DAVA::int32 presetsCount = sizeof(blendPresetsMap) / sizeof(BlendPreset);
    for (DAVA::int32 i = 0; i < presetsCount; i++)
    {
        presetComboBox->addItem(blendPresetsMap[i].presetName);
    }

    QHBoxLayout* blendLayout = new QHBoxLayout();
    QVBoxLayout* presetLayout = new QVBoxLayout();

    presetLayout->addWidget(presetLabel);
    presetLayout->addWidget(presetComboBox);

    blendLayout->addLayout(presetLayout);
    mainBox->addLayout(blendLayout);

    connect(presetComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnLayerMaterialValueChanged()));

    fogCheckBox = new QCheckBox("Enable fog");
    connect(fogCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnLayerMaterialValueChanged()));
    mainBox->addWidget(fogCheckBox);

    CreateStripeLayoutWidget();
    mainBox->addWidget(stripeLayoutWidget);

    lifeTimeLine = new TimeLineWidget(this);
    InitWidget(lifeTimeLine);
    numberTimeLine = new TimeLineWidget(this);
    InitWidget(numberTimeLine);
    sizeTimeLine = new TimeLineWidget(this);
    InitWidget(sizeTimeLine);
    sizeVariationTimeLine = new TimeLineWidget(this);
    InitWidget(sizeVariationTimeLine);
    sizeOverLifeTimeLine = new TimeLineWidget(this);
    InitWidget(sizeOverLifeTimeLine);
    velocityTimeLine = new TimeLineWidget(this);
    InitWidget(velocityTimeLine);
    velocityOverLifeTimeLine = new TimeLineWidget(this);
    InitWidget(velocityOverLifeTimeLine);
    spinTimeLine = new TimeLineWidget(this);
    InitWidget(spinTimeLine);
    spinOverLifeTimeLine = new TimeLineWidget(this);
    InitWidget(spinOverLifeTimeLine);

    randomSpinDirectionCheckBox = new QCheckBox("random spin direction", this);
    connect(randomSpinDirectionCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(OnValueChanged()));
    mainBox->addWidget(randomSpinDirectionCheckBox);

    colorRandomGradient = new GradientPickerWidget(this);
    InitWidget(colorRandomGradient);
    colorOverLifeGradient = new GradientPickerWidget(this);
    InitWidget(colorOverLifeGradient);
    alphaOverLifeTimeLine = new TimeLineWidget(this);
    InitWidget(alphaOverLifeTimeLine);

    useThreePointGradientBox = new QCheckBox("Use three point gradient");
    mainBox->addWidget(useThreePointGradientBox);
    connect(useThreePointGradientBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnThreePointGradientPropertiesChanged()));
    gradientColorForBlackPicker = new GradientPickerWidget(this);
    mainBox->addWidget(gradientColorForBlackPicker);
    connect(gradientColorForBlackPicker,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnThreePointGradientPropertiesChanged()));
    gradientColorForMiddlePicker = new GradientPickerWidget(this);
    mainBox->addWidget(gradientColorForMiddlePicker);
    connect(gradientColorForMiddlePicker,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnThreePointGradientPropertiesChanged()));
    gradientColorForWhitePicker = new GradientPickerWidget(this);
    mainBox->addWidget(gradientColorForWhitePicker);
    connect(gradientColorForWhitePicker,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnThreePointGradientPropertiesChanged()));
    gradientMiddlePointTimeLine = new TimeLineWidget(this);
    connect(gradientMiddlePointTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnThreePointGradientPropertiesChanged()));
    mainBox->addWidget(gradientMiddlePointTimeLine);
    QHBoxLayout* threePointGradHLayout = new QHBoxLayout();
    gradientMiddlePointLabel = new QLabel("Three point gradient middle point");
    gradientMiddlePointSpin = new EventFilterDoubleSpinBox();
    gradientMiddlePointSpin->setMinimum(0.0f);
    gradientMiddlePointSpin->setMaximum(1.0f);
    gradientMiddlePointSpin->setSingleStep(0.01f);
    gradientMiddlePointSpin->setDecimals(3);
    gradientMiddlePointSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    threePointGradHLayout->addWidget(gradientMiddlePointLabel);
    threePointGradHLayout->addWidget(gradientMiddlePointSpin);
    connect(gradientMiddlePointSpin,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(OnThreePointGradientPropertiesChanged()));
    mainBox->addLayout(threePointGradHLayout);

    QHBoxLayout* frameOverlifeLayout = new QHBoxLayout();
    frameOverlifeCheckBox = new QCheckBox("frame over life", this);
    connect(frameOverlifeCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(OnValueChanged()));

    frameOverlifeFPSSpin = new QSpinBox(this);
    frameOverlifeFPSSpin->setMinimum(0);
    frameOverlifeFPSSpin->setMaximum(1000);
    connect(frameOverlifeFPSSpin, SIGNAL(valueChanged(int)),
            this, SLOT(OnValueChanged()));

    frameOverlifeFPSLabel = new QLabel("FPS", this);

    frameOverlifeLayout->addWidget(frameOverlifeCheckBox);
    frameOverlifeLayout->addWidget(frameOverlifeFPSSpin);
    frameOverlifeLayout->addWidget(frameOverlifeFPSLabel);
    mainBox->addLayout(frameOverlifeLayout);

    randomFrameOnStartCheckBox = new QCheckBox("random frame on start", this);
    connect(randomFrameOnStartCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(OnValueChanged()));
    mainBox->addWidget(randomFrameOnStartCheckBox);
    loopSpriteAnimationCheckBox = new QCheckBox("loop sprite animation", this);
    connect(loopSpriteAnimationCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(OnValueChanged()));
    mainBox->addWidget(loopSpriteAnimationCheckBox);

    animSpeedOverLifeTimeLine = new TimeLineWidget(this);
    InitWidget(animSpeedOverLifeTimeLine);

    angleTimeLine = new TimeLineWidget(this);
    InitWidget(angleTimeLine);

    QHBoxLayout* startTimeHBox = new QHBoxLayout;
    startTimeHBox->addWidget(new QLabel("startTime", this));
    startTimeSpin = new EventFilterDoubleSpinBox(this);
    startTimeSpin->setMinimum(-std::numeric_limits<double>::max());
    startTimeSpin->setMaximum(std::numeric_limits<double>::max());
    startTimeHBox->addWidget(startTimeSpin);
    mainBox->addLayout(startTimeHBox);
    connect(startTimeSpin,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(OnValueChanged()));

    QHBoxLayout* endTimeHBox = new QHBoxLayout;
    endTimeHBox->addWidget(new QLabel("endTime", this));
    endTimeSpin = new EventFilterDoubleSpinBox(this);
    endTimeSpin->setMinimum(-std::numeric_limits<double>::max());
    endTimeSpin->setMaximum(std::numeric_limits<double>::max());
    endTimeHBox->addWidget(endTimeSpin);
    mainBox->addLayout(endTimeHBox);
    connect(endTimeSpin,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(OnValueChanged()));

    QHBoxLayout* loopHBox = new QHBoxLayout;
    isLoopedCheckBox = new QCheckBox("Loop layer");
    loopHBox->addWidget(isLoopedCheckBox);
    connect(isLoopedCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnValueChanged()));

    loopEndSpinLabel = new QLabel("loopEnd", this);
    loopEndSpin = new EventFilterDoubleSpinBox(this);
    loopEndSpin->setMinimum(-std::numeric_limits<double>::max());
    loopEndSpin->setMaximum(std::numeric_limits<double>::max());
    loopHBox->addWidget(loopEndSpinLabel);
    loopHBox->addWidget(loopEndSpin);
    connect(loopEndSpin,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(OnValueChanged()));

    loopVariationSpinLabel = new QLabel("loopVariation", this);
    loopVariationSpin = new EventFilterDoubleSpinBox(this);
    loopVariationSpin->setMinimum(-std::numeric_limits<double>::max());
    loopVariationSpin->setMaximum(std::numeric_limits<double>::max());
    loopHBox->addWidget(loopVariationSpinLabel);
    loopHBox->addWidget(loopVariationSpin);
    connect(loopVariationSpin,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(OnValueChanged()));

    loopHBox->setStretch(0, 1);
    loopHBox->setStretch(2, 1);
    loopHBox->setStretch(4, 1);
    mainBox->addLayout(loopHBox);

    QHBoxLayout* deltaHBox = new QHBoxLayout();

    deltaSpinLabel = new QLabel("delta", this);
    deltaSpin = new EventFilterDoubleSpinBox(this);
    deltaSpin->setMinimum(-std::numeric_limits<double>::max());
    deltaSpin->setMaximum(std::numeric_limits<double>::max());
    deltaHBox->addWidget(deltaSpinLabel);
    deltaHBox->addWidget(deltaSpin);
    connect(deltaSpin,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(OnValueChanged()));

    deltaVariationSpinLabel = new QLabel("deltaVariation", this);
    deltaVariationSpin = new EventFilterDoubleSpinBox(this);
    deltaVariationSpin->setMinimum(-std::numeric_limits<double>::max());
    deltaVariationSpin->setMaximum(std::numeric_limits<double>::max());
    deltaHBox->addWidget(deltaVariationSpinLabel);
    deltaHBox->addWidget(deltaVariationSpin);
    connect(deltaVariationSpin,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(OnValueChanged()));
    deltaHBox->setStretch(1, 1);
    deltaHBox->setStretch(3, 1);
    mainBox->addLayout(deltaHBox);

    Q_FOREACH (QAbstractSpinBox* sp, findChildren<QAbstractSpinBox*>())
    {
        sp->installEventFilter(this);
    }
    spritePathLabel->installEventFilter(this);

    spriteUpdateTimer = new QTimer(this);
    connect(spriteUpdateTimer, SIGNAL(timeout()), this, SLOT(OnSpriteUpdateTimerExpired()));

    FillTimeLineWidgetIndentifiers();
}

void EmitterLayerWidget::InitWidget(QWidget* widget)
{
    mainBox->addWidget(widget);
    connect(widget,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnValueChanged()));
}

void EmitterLayerWidget::Init(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect_, DAVA::ParticleEmitterInstance* instance_,
                              DAVA::ParticleLayer* layer_, bool updateMinimized)
{
    if ((instance_ == nullptr) || (layer_ == nullptr))
        return;

    layer = layer_;
    SetObjectsForScene(scene, effect_, instance_);
    Update(updateMinimized);
}

void EmitterLayerWidget::RestoreVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;
    for (auto& timeLineData : timeLineWidgetsIdentifiers)
    {
        if (timeLineData.second != nullptr)
            timeLineData.second->SetVisualState(visualStateProps->GetArchive(timeLineData.first));
    }
}

void EmitterLayerWidget::StoreVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;

    DAVA::ScopedPtr<DAVA::KeyedArchive> props(new DAVA::KeyedArchive());

    for (auto& timeLineData : timeLineWidgetsIdentifiers)
    {
        props->DeleteAllKeys();
        timeLineData.second->GetVisualState(props);
        visualStateProps->SetArchive(timeLineData.first, props);
    }
}

void EmitterLayerWidget::OnSpriteBtn()
{
    OnChangeSpriteButton(layer->spritePath, spritePathLabel, QString("Open particle sprite"), std::bind(&EmitterLayerWidget::OnSpritePathEdited, this, std::placeholders::_1));
}

void EmitterLayerWidget::OnSpriteFolderBtn()
{
    OnChangeFolderButton(layer->spritePath, spritePathLabel, std::bind(&EmitterLayerWidget::OnSpritePathEdited, this, std::placeholders::_1));
}

void EmitterLayerWidget::OnValueChanged()
{
    if (blockSignals)
        return;

    DAVA::PropLineWrapper<DAVA::float32> propLife;
    DAVA::PropLineWrapper<DAVA::float32> propLifeVariation;
    lifeTimeLine->GetValue(0, propLife.GetPropsPtr());
    lifeTimeLine->GetValue(1, propLifeVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propNumber;
    DAVA::PropLineWrapper<DAVA::float32> propNumberVariation;
    numberTimeLine->GetValue(0, propNumber.GetPropsPtr());
    numberTimeLine->GetValue(1, propNumberVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::Vector2> propSize;
    sizeTimeLine->GetValues(propSize.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::Vector2> propSizeVariation;
    sizeVariationTimeLine->GetValues(propSizeVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::Vector2> propsizeOverLife;
    sizeOverLifeTimeLine->GetValues(propsizeOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propVelocity;
    DAVA::PropLineWrapper<DAVA::float32> propVelocityVariation;
    velocityTimeLine->GetValue(0, propVelocity.GetPropsPtr());
    velocityTimeLine->GetValue(1, propVelocityVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propVelocityOverLife;
    velocityOverLifeTimeLine->GetValue(0, propVelocityOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propSpin;
    DAVA::PropLineWrapper<DAVA::float32> propSpinVariation;
    spinTimeLine->GetValue(0, propSpin.GetPropsPtr());
    spinTimeLine->GetValue(1, propSpinVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propSpinOverLife;
    spinOverLifeTimeLine->GetValue(0, propSpinOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propAnimSpeedOverLife;
    animSpeedOverLifeTimeLine->GetValue(0, propAnimSpeedOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::Color> propColorRandom;
    colorRandomGradient->GetValues(propColorRandom.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::Color> propColorOverLife;
    colorOverLifeGradient->GetValues(propColorOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propAlphaOverLife;
    alphaOverLifeTimeLine->GetValue(0, propAlphaOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propAngle;
    DAVA::PropLineWrapper<DAVA::float32> propAngleVariation;
    angleTimeLine->GetValue(0, propAngle.GetPropsPtr());
    angleTimeLine->GetValue(1, propAngleVariation.GetPropsPtr());

    DAVA::ParticleLayer::eType propLayerType = layerTypeMap[layerTypeComboBox->currentIndex()].layerType;

    DAVA::int32 particleOrientation = 0;
    if (cameraFacingCheckBox->isChecked())
        particleOrientation += DAVA::ParticleLayer::PARTICLE_ORIENTATION_CAMERA_FACING;
    if (xFacingCheckBox->isChecked())
        particleOrientation += DAVA::ParticleLayer::PARTICLE_ORIENTATION_X_FACING;
    if (yFacingCheckBox->isChecked())
        particleOrientation += DAVA::ParticleLayer::PARTICLE_ORIENTATION_Y_FACING;
    if (zFacingCheckBox->isChecked())
        particleOrientation += DAVA::ParticleLayer::PARTICLE_ORIENTATION_Z_FACING;
    if (worldAlignCheckBox->isChecked())
        particleOrientation += DAVA::ParticleLayer::PARTICLE_ORIENTATION_WORLD_ALIGN;
    if (cameraFacingStripeSphericalCheckBox->isChecked())
        particleOrientation += DAVA::ParticleLayer::PARTICLE_ORIENTATION_CAMERA_FACING_STRIPE_SPHERICAL;

    DAVA::ParticleLayer::eDegradeStrategy degradeStrategy = DAVA::ParticleLayer::eDegradeStrategy(degradeStrategyComboBox->currentIndex());
    bool superemitterStatusChanged = (layer->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES) != (propLayerType == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES);

    DAVA::SceneEditor2* activeScene = GetActiveScene();
    std::unique_ptr<DAVA::CommandUpdateParticleLayer> updateLayerCmd(new DAVA::CommandUpdateParticleLayer(GetEmitterInstance(activeScene), layer));
    updateLayerCmd->Init(layerNameLineEdit->text().toStdString(),
                         propLayerType,
                         degradeStrategy,
                         !enableCheckBox->isChecked(),
                         inheritPostionCheckBox->isChecked(),
                         isLongCheckBox->isChecked(),
                         scaleVelocityBaseSpinBox->value(),
                         scaleVelocityFactorSpinBox->value(),
                         isLoopedCheckBox->isChecked(),
                         particleOrientation,
                         propLife.GetPropLine(),
                         propLifeVariation.GetPropLine(),
                         propNumber.GetPropLine(),
                         propNumberVariation.GetPropLine(),
                         propSize.GetPropLine(),
                         propSizeVariation.GetPropLine(),
                         propsizeOverLife.GetPropLine(),
                         propVelocity.GetPropLine(),
                         propVelocityVariation.GetPropLine(),
                         propVelocityOverLife.GetPropLine(),
                         propSpin.GetPropLine(),
                         propSpinVariation.GetPropLine(),
                         propSpinOverLife.GetPropLine(),
                         randomSpinDirectionCheckBox->isChecked(),

                         propColorRandom.GetPropLine(),
                         propAlphaOverLife.GetPropLine(),
                         propColorOverLife.GetPropLine(),
                         propAngle.GetPropLine(),
                         propAngleVariation.GetPropLine(),

                         static_cast<DAVA::float32>(startTimeSpin->value()),
                         static_cast<DAVA::float32>(endTimeSpin->value()),
                         static_cast<DAVA::float32>(deltaSpin->value()),
                         static_cast<DAVA::float32>(deltaVariationSpin->value()),
                         static_cast<DAVA::float32>(loopEndSpin->value()),
                         static_cast<DAVA::float32>(loopVariationSpin->value()),
                         frameOverlifeCheckBox->isChecked(),
                         static_cast<DAVA::float32>(frameOverlifeFPSSpin->value()),
                         randomFrameOnStartCheckBox->isChecked(),
                         loopSpriteAnimationCheckBox->isChecked(),
                         propAnimSpeedOverLife.GetPropLine(),
                         static_cast<DAVA::float32>(pivotPointXSpinBox->value()),
                         static_cast<DAVA::float32>(pivotPointYSpinBox->value()),
                         applyGlobalForcesCheckBox->isChecked());

    DVASSERT(GetActiveScene() != nullptr);
    GetActiveScene()->Exec(std::move(updateLayerCmd));
    GetActiveScene()->MarkAsChanged();

    if (layer->particleOrientation & DAVA::ParticleLayer::PARTICLE_ORIENTATION_CAMERA_FACING && layer->useFresnelToAlpha)
    {
        DAVA::NotificationParams params;
        params.message.message = "The check boxes Fresnel to alpha and Camera facing are both set.";
        params.message.type = DAVA::Result::RESULT_WARNING;
        params.title = "Particle system warning.";
        DAVA::Deprecated::ShowNotification(params);
    }

    if (superemitterStatusChanged)
    {
        if (!GetEffect(activeScene)->IsStopped())
            GetEffect(activeScene)->Restart(true);
    }
    emit ValueChanged();
}

void EmitterLayerWidget::OnFresnelToAlphaChanged()
{
    if (blockSignals)
        return;

    DVASSERT(GetActiveScene() != nullptr);
    DAVA::CommandChangeFresnelToAlphaProperties::FresnelToAlphaParams params;
    params.useFresnelToAlpha = fresnelToAlphaCheckbox->isChecked();
    params.fresnelToAlphaPower = static_cast<DAVA::float32>(fresnelPowerSpinBox->value());
    params.fresnelToAlphaBias = static_cast<DAVA::float32>(fresnelBiasSpinBox->value());

    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandChangeFresnelToAlphaProperties(layer, std::move(params))));

    if (layer->particleOrientation & DAVA::ParticleLayer::PARTICLE_ORIENTATION_CAMERA_FACING && layer->useFresnelToAlpha)
    {
        DAVA::NotificationParams notificationParams;
        notificationParams.message.message = "The check boxes Fresnel to alpha and Camera facing are both set.";
        notificationParams.message.type = DAVA::Result::RESULT_WARNING;
        notificationParams.title = "Particle system warning.";
        DAVA::Deprecated::ShowNotification(notificationParams);
    }

    emit ValueChanged();
}

void EmitterLayerWidget::OnLayerMaterialValueChanged()
{
    if (blockSignals)
        return;

    const DAVA::eBlending blending = blendPresetsMap[presetComboBox->currentIndex()].blending;

    QString path = spritePathLabel->text();
    path = EmitterLayerWidgetDetails::ConvertPSDPathToSprite(path);
    const DAVA::FilePath spritePath(path.toStdString());

    DVASSERT(GetActiveScene() != nullptr);
    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandChangeLayerMaterialProperties(layer, spritePath, blending, fogCheckBox->isChecked(), frameBlendingCheckBox->isChecked())));

    UpdateLayerSprite();

    emit ValueChanged();
}

void EmitterLayerWidget::OnFlowPropertiesChanged()
{
    if (blockSignals)
        return;

    DAVA::PropLineWrapper<DAVA::float32> propFlowSpeed;
    flowSpeedTimeLine->GetValue(0, propFlowSpeed.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propFlowSpeedVariation;
    flowSpeedVariationTimeLine->GetValue(0, propFlowSpeedVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propFlowOffset;
    flowOffsetTimeLine->GetValue(0, propFlowOffset.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propFlowOffsetVariation;
    flowOffsetVariationTimeLine->GetValue(0, propFlowOffsetVariation.GetPropsPtr());

    QString path = flowSpritePathLabel->text();
    path = EmitterLayerWidgetDetails::ConvertPSDPathToSprite(path);
    const DAVA::FilePath spritePath(path.toStdString());

    DVASSERT(GetActiveScene() != nullptr);
    DAVA::CommandChangeFlowProperties::FlowParams params;
    params.spritePath = spritePath;
    params.enableFlow = enableFlowCheckBox->isChecked();
    params.enabelFlowAnimation = enableFlowAnimationCheckBox->isChecked();
    params.flowSpeed = propFlowSpeed.GetPropLine();
    params.flowSpeedVariation = propFlowSpeedVariation.GetPropLine();
    params.flowOffset = propFlowOffset.GetPropLine();
    params.flowOffsetVariation = propFlowOffsetVariation.GetPropLine();
    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandChangeFlowProperties(layer, std::move(params))));

    UpdateFlowmapSprite();

    emit ValueChanged();
}

void EmitterLayerWidget::OnStripePropertiesChanged()
{
    if (blockSignals)
        return;

    DVASSERT(GetActiveScene() != nullptr);
    DAVA::PropLineWrapper<DAVA::float32> propStripeSizeOverLife;
    stripeSizeOverLifeTimeLine->GetValue(0, propStripeSizeOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propStripeTileOverLife;
    stripeTextureTileTimeLine->GetValue(0, propStripeTileOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propstripeNoiseUScrollSpeedOverLife;
    stripeNoiseScrollSpeedOverLifeTimeLine->GetValue(0, propstripeNoiseUScrollSpeedOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propstripeNoiseVScrollSpeedOverLife;
    stripeNoiseScrollSpeedOverLifeTimeLine->GetValue(1, propstripeNoiseVScrollSpeedOverLife.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::Color> propStripeColorOverLife;
    stripeColorOverLifeGradient->GetValues(propStripeColorOverLife.GetPropsPtr());

    DAVA::CommandChangeParticlesStripeProperties::StripeParams params;
    params.stripeLifetime = static_cast<DAVA::float32>(stripeLifetimeSpin->value());
    params.stripeVertexSpawnStep = static_cast<DAVA::float32>(stripeVertexSpawnStepSpin->value());
    params.stripeStartSize = static_cast<DAVA::float32>(stripeStartSizeSpin->value());
    params.stripeUScrollSpeed = static_cast<DAVA::float32>(stripeUScrollSpeedSpin->value());
    params.stripeVScrollSpeed = static_cast<DAVA::float32>(stripeVScrollSpeedSpin->value());
    params.stripeFadeDistanceFromTop = static_cast<DAVA::float32>(stripeFadeDistanceFromTopSpin->value());
    params.stripeInheritPositionForBase = stripeInheritPositionForBaseCheckBox->isChecked();
    params.usePerspectiveMapping = stripeUsePerspectiveMappingCheckBox->isChecked();
    params.stripeTextureTileOverLife = propStripeTileOverLife.GetPropLine();
    params.stripeSizeOverLife = propStripeSizeOverLife.GetPropLine();
    params.stripeNoiseUScrollSpeedOverLife = propstripeNoiseUScrollSpeedOverLife.GetPropLine();
    params.stripeNoiseVScrollSpeedOverLife = propstripeNoiseVScrollSpeedOverLife.GetPropLine();
    params.stripeColorOverLife = propStripeColorOverLife.GetPropLine();
    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandChangeParticlesStripeProperties(layer, std::move(params))));

    emit ValueChanged();
}

void EmitterLayerWidget::OnNoisePropertiesChanged()
{
    if (blockSignals)
        return;

    // Scale.
    DAVA::PropLineWrapper<DAVA::float32> propNoiseScale;
    noiseScaleTimeLine->GetValue(0, propNoiseScale.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propNoiseScaleVariation;
    noiseScaleVariationTimeLine->GetValue(0, propNoiseScaleVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propNoiseScaleOverLife;
    noiseScaleOverLifeTimeLine->GetValue(0, propNoiseScaleOverLife.GetPropsPtr());

    // U scroll speed.
    DAVA::PropLineWrapper<DAVA::float32> propNoiseUScrollSpeed;
    noiseUVScrollSpeedTimeLine->GetValue(0, propNoiseUScrollSpeed.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propNoiseUScrollSpeedVariation;
    noiseUVScrollSpeedVariationTimeLine->GetValue(0, propNoiseUScrollSpeedVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propNoiseUScrollSpeedOverLife;
    noiseUVScrollSpeedOverLifeTimeLine->GetValue(0, propNoiseUScrollSpeedOverLife.GetPropsPtr());

    // V scroll speed.
    DAVA::PropLineWrapper<DAVA::float32> propNoiseVScrollSpeed;
    noiseUVScrollSpeedTimeLine->GetValue(1, propNoiseVScrollSpeed.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propNoiseVScrollSpeedVariation;
    noiseUVScrollSpeedVariationTimeLine->GetValue(1, propNoiseVScrollSpeedVariation.GetPropsPtr());

    DAVA::PropLineWrapper<DAVA::float32> propNoiseVScrollSpeedOverLife;
    noiseUVScrollSpeedOverLifeTimeLine->GetValue(1, propNoiseVScrollSpeedOverLife.GetPropsPtr());

    QString path = noiseSpritePathLabel->text();
    path = EmitterLayerWidgetDetails::ConvertPSDPathToSprite(path);
    const DAVA::FilePath noisePath(path.toStdString());

    DAVA::CommandChangeNoiseProperties::NoiseParams params;
    params.noisePath = noisePath;
    params.enableNoise = enableNoiseCheckBox->isChecked();

    params.noiseScale = propNoiseScale.GetPropLine();
    params.noiseScaleVariation = propNoiseScaleVariation.GetPropLine();
    params.noiseScaleOverLife = propNoiseScaleOverLife.GetPropLine();

    params.enableNoiseScroll = enableNoiseScrollCheckBox->isChecked();
    params.noiseUScrollSpeed = propNoiseUScrollSpeed.GetPropLine();
    params.noiseUScrollSpeedVariation = propNoiseUScrollSpeedVariation.GetPropLine();
    params.noiseUScrollSpeedOverLife = propNoiseUScrollSpeedOverLife.GetPropLine();

    params.enableNoiseScroll = enableNoiseScrollCheckBox->isChecked();
    params.noiseVScrollSpeed = propNoiseVScrollSpeed.GetPropLine();
    params.noiseVScrollSpeedVariation = propNoiseVScrollSpeedVariation.GetPropLine();
    params.noiseVScrollSpeedOverLife = propNoiseVScrollSpeedOverLife.GetPropLine();

    DVASSERT(GetActiveScene() != nullptr);
    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandChangeNoiseProperties(layer, std::move(params))));

    UpdateNoiseSprite();

    emit ValueChanged();
}

void EmitterLayerWidget::OnAlphaRemapPropertiesChanged()
{
    if (blockSignals)
        return;
    DAVA::PropLineWrapper<DAVA::float32> propAlphaRemapOverLife;
    alphaRemapOverLifeTimeLine->GetValue(0, propAlphaRemapOverLife.GetPropsPtr());

    QString path = alphaRemapSpritePathLabel->text();
    path = EmitterLayerWidgetDetails::ConvertPSDPathToSprite(path);
    const DAVA::FilePath alphaPath(path.toStdString());

    DAVA::CommandChangeAlphaRemapProperties::AlphaRemapParams params;
    params.alphaRemapOverLife = propAlphaRemapOverLife.GetPropLine();
    params.enableAlphaRemap = enableAlphaRemapCheckBox->isChecked();
    params.alphaRemapLoopCount = static_cast<DAVA::float32>(alphaRemapLoopCountSpin->value());
    params.alphaRemapPath = alphaPath;

    DVASSERT(GetActiveScene() != nullptr);
    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandChangeAlphaRemapProperties(layer, std::move(params))));

    UpdateAlphaRemapSprite();

    emit ValueChanged();
}

void EmitterLayerWidget::OnThreePointGradientPropertiesChanged()
{
    if (blockSignals)
        return;

    DAVA::PropLineWrapper<DAVA::Color> propGradientColorForBlack;
    gradientColorForBlackPicker->GetValues(propGradientColorForBlack.GetPropsPtr());
    DAVA::PropLineWrapper<DAVA::Color> propGradientColorForMiddle;
    gradientColorForMiddlePicker->GetValues(propGradientColorForMiddle.GetPropsPtr());
    DAVA::PropLineWrapper<DAVA::Color> propGradientColorForWhite;
    gradientColorForWhitePicker->GetValues(propGradientColorForWhite.GetPropsPtr());
    DAVA::PropLineWrapper<DAVA::float32> propGradientMiddlePoint;
    gradientMiddlePointTimeLine->GetValue(0, propGradientMiddlePoint.GetPropsPtr());

    DAVA::CommandChangeThreePointGradientProperties::ThreePointGradientParams params;
    params.useThreePointGradient = useThreePointGradientBox->isChecked();
    params.gradientMiddlePointLine = propGradientMiddlePoint.GetPropLine();
    params.gradientColorForBlack = propGradientColorForBlack.GetPropLine();
    params.gradientColorForMiddle = propGradientColorForMiddle.GetPropLine();
    params.gradientColorForWhite = propGradientColorForWhite.GetPropLine();
    params.gradientMiddlePoint = static_cast<DAVA::float32>(gradientMiddlePointSpin->value());

    DVASSERT(GetActiveScene() != nullptr);
    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandChangeThreePointGradientProperties(layer, std::move(params))));

    emit ValueChanged();
}

void EmitterLayerWidget::OnLodsChanged()
{
    if (blockSignals)
        return;

    DAVA::Vector<bool> lods;
    lods.resize(DAVA::LodComponent::MAX_LOD_LAYERS, true);
    for (DAVA::int32 i = 0; i < DAVA::LodComponent::MAX_LOD_LAYERS; ++i)
    {
        lods[i] = layerLodsCheckBox[i]->isChecked();
    }

    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandUpdateParticleLayerLods(layer, lods)));
    GetActiveScene()->MarkAsChanged();
    emit ValueChanged();
}

void EmitterLayerWidget::OnSpriteUpdateTimerExpired()
{
    if (spriteUpdateTexturesStack.size() > 0 && rhi::SyncObjectSignaled(spriteUpdateTexturesStack.top().first))
    {
        DAVA::ScopedPtr<DAVA::Image> image(spriteUpdateTexturesStack.top().second->CreateImageFromMemory());
        spriteLabel->setPixmap(QPixmap::fromImage(DAVA::ImageTools::FromDavaImage(image)));

        while (!spriteUpdateTexturesStack.empty())
        {
            SafeRelease(spriteUpdateTexturesStack.top().second);
            spriteUpdateTexturesStack.pop();
        }

        spriteUpdateTimer->stop();
    }
    if (flowSpriteUpdateTexturesStack.size() > 0 && rhi::SyncObjectSignaled(flowSpriteUpdateTexturesStack.top().first))
    {
        DAVA::ScopedPtr<DAVA::Image> image(flowSpriteUpdateTexturesStack.top().second->CreateImageFromMemory());
        flowSpriteLabel->setPixmap(QPixmap::fromImage(DAVA::ImageTools::FromDavaImage(image)));

        while (!flowSpriteUpdateTexturesStack.empty())
        {
            SafeRelease(flowSpriteUpdateTexturesStack.top().second);
            flowSpriteUpdateTexturesStack.pop();
        }

        spriteUpdateTimer->stop();
    }
    if (noiseSpriteUpdateTexturesStack.size() > 0 && rhi::SyncObjectSignaled(noiseSpriteUpdateTexturesStack.top().first))
    {
        DAVA::ScopedPtr<DAVA::Image> image(noiseSpriteUpdateTexturesStack.top().second->CreateImageFromMemory());
        noiseSpriteLabel->setPixmap(QPixmap::fromImage(DAVA::ImageTools::FromDavaImage(image)));

        while (!noiseSpriteUpdateTexturesStack.empty())
        {
            SafeRelease(noiseSpriteUpdateTexturesStack.top().second);
            noiseSpriteUpdateTexturesStack.pop();
        }

        spriteUpdateTimer->stop();
    }
    if (alphaRemapSpriteUpdateTexturesStack.size() > 0 && rhi::SyncObjectSignaled(alphaRemapSpriteUpdateTexturesStack.top().first))
    {
        DAVA::ScopedPtr<DAVA::Image> image(alphaRemapSpriteUpdateTexturesStack.top().second->CreateImageFromMemory());
        alphaRemapSpriteLabel->setPixmap(QPixmap::fromImage(DAVA::ImageTools::FromDavaImage(image)));

        while (!alphaRemapSpriteUpdateTexturesStack.empty())
        {
            SafeRelease(alphaRemapSpriteUpdateTexturesStack.top().second);
            alphaRemapSpriteUpdateTexturesStack.pop();
        }

        spriteUpdateTimer->stop();
    }
}

void EmitterLayerWidget::Update(bool updateMinimized)
{
    blockSignals = true;
    DAVA::float32 lifeTime = layer->endTime;

    layerNameLineEdit->setText(QString::fromStdString(layer->layerName));
    layerTypeComboBox->setCurrentIndex(LayerTypeToIndex(layer->type));

    enableCheckBox->setChecked(!layer->isDisabled);
    inheritPostionCheckBox->setChecked(layer->GetInheritPosition());

    useThreePointGradientBox->setChecked(layer->useThreePointGradient);
    gradientMiddlePointSpin->setValue(layer->gradientMiddlePoint);

    applyGlobalForcesCheckBox->setChecked(layer->applyGlobalForces);

    isLongCheckBox->setChecked(layer->isLong);
    scaleVelocityBaseSpinBox->setValue((double)layer->scaleVelocityBase);
    scaleVelocityFactorSpinBox->setValue((double)layer->scaleVelocityFactor);

    bool scaleVelocityVisible = layer->isLong;
    scaleVelocityBaseLabel->setVisible(scaleVelocityVisible);
    scaleVelocityBaseSpinBox->setVisible(scaleVelocityVisible);
    scaleVelocityFactorLabel->setVisible(scaleVelocityVisible);
    scaleVelocityFactorSpinBox->setVisible(scaleVelocityVisible);

    fresnelToAlphaCheckbox->setChecked(layer->useFresnelToAlpha);
    fresnelBiasSpinBox->setValue(layer->fresnelToAlphaBias);
    fresnelPowerSpinBox->setValue(layer->fresnelToAlphaPower);

    stripeLifetimeSpin->setValue(layer->stripeLifetime);
    stripeVertexSpawnStepSpin->setValue(layer->stripeVertexSpawnStep);
    stripeStartSizeSpin->setValue(layer->stripeStartSize);
    stripeUScrollSpeedSpin->setValue(layer->stripeUScrollSpeed);
    stripeVScrollSpeedSpin->setValue(layer->stripeVScrollSpeed);
    stripeFadeDistanceFromTopSpin->setValue(layer->stripeFadeDistanceFromTop);
    stripeInheritPositionForBaseCheckBox->setChecked(layer->GetInheritPositionForStripeBase());
    stripeUsePerspectiveMappingCheckBox->setChecked(layer->usePerspectiveMapping);

    bool fresToAlphaVisible = layer->useFresnelToAlpha;
    fresnelBiasLabel->setVisible(fresToAlphaVisible);
    fresnelBiasSpinBox->setVisible(fresToAlphaVisible);
    fresnelPowerLabel->setVisible(fresToAlphaVisible);
    fresnelPowerSpinBox->setVisible(fresToAlphaVisible);

    stripeNoiseScrollSpeedOverLifeTimeLine->setVisible(layer->enableNoise);

    enableFlowCheckBox->setChecked(layer->enableFlow);
    enableFlowAnimationCheckBox->setChecked(layer->enableFlowAnimation);
    flowLayoutWidget->setVisible(enableFlowCheckBox->isChecked());
    flowSettingsLayoutWidget->setVisible(enableFlowCheckBox->isChecked() && !enableFlowAnimationCheckBox->isChecked());

    enableNoiseScrollCheckBox->setChecked(layer->enableNoiseScroll);

    enableNoiseCheckBox->setChecked(layer->enableNoise);
    noiseLayoutWidget->setVisible(enableNoiseCheckBox->isChecked());
    noiseScrollWidget->setVisible(enableNoiseCheckBox->isChecked() && enableNoiseScrollCheckBox->isChecked());

    enableAlphaRemapCheckBox->setChecked(layer->enableAlphaRemap);
    alphaRemapLoopCountSpin->setValue(layer->alphaRemapLoopCount);
    alphaRemapLayoutWidget->setVisible(enableAlphaRemapCheckBox->isChecked());

    isLoopedCheckBox->setChecked(layer->isLooped);

    for (DAVA::int32 i = 0; i < DAVA::LodComponent::MAX_LOD_LAYERS; ++i)
    {
        layerLodsCheckBox[i]->setChecked(layer->IsLodActive(i));
    }

    degradeStrategyComboBox->setCurrentIndex(static_cast<DAVA::int32>(layer->degradeStrategy));

    UpdateLayerSprite();
    UpdateFlowmapSprite();
    UpdateNoiseSprite();
    UpdateAlphaRemapSprite();

    //particle orientation
    cameraFacingCheckBox->setChecked(layer->particleOrientation & DAVA::ParticleLayer::PARTICLE_ORIENTATION_CAMERA_FACING);
    xFacingCheckBox->setChecked(layer->particleOrientation & DAVA::ParticleLayer::PARTICLE_ORIENTATION_X_FACING);
    yFacingCheckBox->setChecked(layer->particleOrientation & DAVA::ParticleLayer::PARTICLE_ORIENTATION_Y_FACING);
    zFacingCheckBox->setChecked(layer->particleOrientation & DAVA::ParticleLayer::PARTICLE_ORIENTATION_Z_FACING);
    worldAlignCheckBox->setChecked(layer->particleOrientation & DAVA::ParticleLayer::PARTICLE_ORIENTATION_WORLD_ALIGN);
    cameraFacingStripeSphericalCheckBox->setChecked(layer->particleOrientation & DAVA::ParticleLayer::PARTICLE_ORIENTATION_CAMERA_FACING_STRIPE_SPHERICAL);

    //blend and fog

    DAVA::int32 presetsCount = sizeof(blendPresetsMap) / sizeof(BlendPreset);
    DAVA::int32 presetId;
    for (presetId = 0; presetId < presetsCount; presetId++)
    {
        if (blendPresetsMap[presetId].blending == layer->blending)
            break;
    }
    presetComboBox->setCurrentIndex(presetId);

    fogCheckBox->setChecked(layer->enableFog);

    frameBlendingCheckBox->setChecked(layer->enableFrameBlend);

    stripeSizeOverLifeTimeLine->Init(0.0f, 1.0f, updateMinimized);
    stripeSizeOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->stripeSizeOverLife)).GetProps(), Qt::red, "Stripe edge size over life");
    stripeSizeOverLifeTimeLine->EnableLock(true);

    stripeNoiseScrollSpeedOverLifeTimeLine->Init(0.0f, 1.0f, updateMinimized);
    stripeNoiseScrollSpeedOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->stripeNoiseUScrollSpeedOverLife)).GetProps(), Qt::red, "Noise U scroll speed over stripe life");
    stripeNoiseScrollSpeedOverLifeTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->stripeNoiseVScrollSpeedOverLife)).GetProps(), Qt::green, "Noise V scroll speed over stripe life");
    stripeNoiseScrollSpeedOverLifeTimeLine->EnableLock(true);

    stripeTextureTileTimeLine->Init(0.0f, 1.0f, updateMinimized, false, true, false, STRIPE_TILE_PRECISION_DIGITS);
    stripeTextureTileTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->stripeTextureTileOverLife)).GetProps(), Qt::red, "Stripe texture tile over life");
    stripeTextureTileTimeLine->EnableLock(true);

    stripeColorOverLifeGradient->Init(0, 1, "Stripe vertex color over life");
    stripeColorOverLifeGradient->SetValues(DAVA::PropLineWrapper<DAVA::Color>(DAVA::PropertyLineHelper::GetValueLine(layer->stripeColorOverLife)).GetProps());

    // FLOW_STUFF
    flowSpeedTimeLine->Init(layer->startTime, lifeTime, updateMinimized, false, true, false, FLOW_PRECISION_DIGITS);
    flowSpeedTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->flowSpeed)).GetProps(), Qt::red, "Flow speed");
    flowSpeedTimeLine->EnableLock(true);

    flowSpeedVariationTimeLine->Init(layer->startTime, lifeTime, updateMinimized, false, true, false, FLOW_PRECISION_DIGITS);
    flowSpeedVariationTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->flowSpeedVariation)).GetProps(), Qt::green, "Flow speed variation");
    flowSpeedVariationTimeLine->EnableLock(true);

    flowOffsetTimeLine->Init(layer->startTime, lifeTime, updateMinimized, false, true, false, FLOW_PRECISION_DIGITS);
    flowOffsetTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->flowOffset)).GetProps(), Qt::red, "Flow offset");
    flowOffsetTimeLine->EnableLock(true);

    flowOffsetVariationTimeLine->Init(layer->startTime, lifeTime, updateMinimized, false, true, false, FLOW_PRECISION_DIGITS);
    flowOffsetVariationTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->flowOffsetVariation)).GetProps(), Qt::green, "Flow offset variation");
    flowOffsetVariationTimeLine->EnableLock(true);

    // NOISE_STUFF
    noiseScaleTimeLine->Init(layer->startTime, lifeTime, updateMinimized, false, true, false, NOISE_PRECISION_DIGITS);
    noiseScaleTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseScale)).GetProps(), Qt::red, "Noise scale");
    noiseScaleTimeLine->EnableLock(true);

    noiseScaleVariationTimeLine->Init(layer->startTime, lifeTime, updateMinimized, false, true, false, NOISE_PRECISION_DIGITS);
    noiseScaleVariationTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseScaleVariation)).GetProps(), Qt::green, "Noise scale variation");
    noiseScaleVariationTimeLine->EnableLock(true);

    noiseScaleOverLifeTimeLine->Init(0.0f, 1.0f, updateMinimized, false, true, false, NOISE_PRECISION_DIGITS);
    noiseScaleOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseScaleOverLife)).GetProps(), Qt::blue, "Noise scale over life");
    noiseScaleOverLifeTimeLine->EnableLock(true);

    noiseUVScrollSpeedTimeLine->Init(layer->startTime, lifeTime, updateMinimized);
    noiseUVScrollSpeedTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseUScrollSpeed)).GetProps(), Qt::red, "Noise U scroll speed");
    noiseUVScrollSpeedTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseVScrollSpeed)).GetProps(), Qt::green, "Noise V scroll speed");
    noiseUVScrollSpeedTimeLine->EnableLock(true);

    noiseUVScrollSpeedVariationTimeLine->Init(layer->startTime, lifeTime, updateMinimized);
    noiseUVScrollSpeedVariationTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseUScrollSpeedVariation)).GetProps(), Qt::red, "Noise U scroll speed variation");
    noiseUVScrollSpeedVariationTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseVScrollSpeedVariation)).GetProps(), Qt::green, "Noise V scroll speed variation");
    noiseUVScrollSpeedVariationTimeLine->EnableLock(true);

    noiseUVScrollSpeedOverLifeTimeLine->Init(0.0f, 1.0f, updateMinimized);
    noiseUVScrollSpeedOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseUScrollSpeedOverLife)).GetProps(), Qt::red, "Noise U scroll speed over life");
    noiseUVScrollSpeedOverLifeTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->noiseVScrollSpeedOverLife)).GetProps(), Qt::green, "Noise V scroll speed over life");
    noiseUVScrollSpeedOverLifeTimeLine->EnableLock(true);

    alphaRemapOverLifeTimeLine->Init(0.0f, 1.0f, updateMinimized);
    alphaRemapOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->alphaRemapOverLife)).GetProps(), Qt::red, "Alpha remap over life");
    alphaRemapOverLifeTimeLine->EnableLock(true);

    //LAYER_LIFE, LAYER_LIFE_VARIATION,
    lifeTimeLine->Init(layer->startTime, lifeTime, updateMinimized);
    lifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->life)).GetProps(), Qt::blue, "life");
    lifeTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->lifeVariation)).GetProps(), Qt::darkGreen, "life variation");
    lifeTimeLine->SetMinLimits(0.0f);

    //LAYER_NUMBER, LAYER_NUMBER_VARIATION,
    numberTimeLine->Init(layer->startTime, lifeTime, updateMinimized, false, true);
    numberTimeLine->SetMinLimits(0);
    numberTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->number)).GetProps(), Qt::blue, "number");
    numberTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->numberVariation)).GetProps(), Qt::darkGreen, "number variation");

    DAVA::ParticleLayer::eType propLayerType = layerTypeMap[layerTypeComboBox->currentIndex()].layerType;
    numberTimeLine->setVisible(propLayerType != DAVA::ParticleLayer::TYPE_SINGLE_PARTICLE);

    //LAYER_SIZE, LAYER_SIZE_VARIATION, LAYER_SIZE_OVER_LIFE,
    DAVA::Vector<QColor> colors;
    colors.push_back(Qt::red);
    colors.push_back(Qt::darkGreen);
    DAVA::Vector<QString> legends;
    legends.push_back("size X");
    legends.push_back("size Y");
    sizeTimeLine->Init(layer->startTime, lifeTime, updateMinimized, true);
    sizeTimeLine->SetMinLimits(0);
    sizeTimeLine->AddLines(DAVA::PropLineWrapper<DAVA::Vector2>(DAVA::PropertyLineHelper::GetValueLine(layer->size)).GetProps(), colors, legends);
    sizeTimeLine->EnableLock(true);

    legends.clear();
    legends.push_back("size variation X");
    legends.push_back("size variation Y");
    sizeVariationTimeLine->Init(layer->startTime, lifeTime, updateMinimized, true);
    sizeVariationTimeLine->SetMinLimits(0);
    sizeVariationTimeLine->AddLines(DAVA::PropLineWrapper<DAVA::Vector2>(DAVA::PropertyLineHelper::GetValueLine(layer->sizeVariation)).GetProps(), colors, legends);
    sizeVariationTimeLine->EnableLock(true);

    legends.clear();
    legends.push_back("size over life X");
    legends.push_back("size over life Y");
    sizeOverLifeTimeLine->Init(0, 1, updateMinimized, true);
    sizeOverLifeTimeLine->SetMinLimits(0);
    sizeOverLifeTimeLine->AddLines(DAVA::PropLineWrapper<DAVA::Vector2>(DAVA::PropertyLineHelper::GetValueLine(layer->sizeOverLifeXY)).GetProps(), colors, legends);
    sizeOverLifeTimeLine->EnableLock(true);

    //LAYER_VELOCITY, LAYER_VELOCITY_VARIATION,
    velocityTimeLine->Init(layer->startTime, lifeTime, updateMinimized);
    velocityTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->velocity)).GetProps(), Qt::blue, "velocity");
    velocityTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->velocityVariation)).GetProps(), Qt::darkGreen, "velocity variation");

    //LAYER_VELOCITY_OVER_LIFE,
    velocityOverLifeTimeLine->Init(0, 1, updateMinimized);
    velocityOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->velocityOverLife)).GetProps(), Qt::blue, "velocity over life");

    //LAYER_FORCES, LAYER_FORCES_VARIATION, LAYER_FORCES_OVER_LIFE,

    //LAYER_SPIN, LAYER_SPIN_VARIATION,
    spinTimeLine->Init(layer->startTime, lifeTime, updateMinimized);
    spinTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->spin)).GetProps(), Qt::blue, "spin");
    spinTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->spinVariation)).GetProps(), Qt::darkGreen, "spin variation");

    //LAYER_SPIN_OVER_LIFE,
    spinOverLifeTimeLine->Init(0, 1, updateMinimized);
    spinOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->spinOverLife)).GetProps(), Qt::blue, "spin over life");

    randomSpinDirectionCheckBox->setChecked(layer->randomSpinDirection);

    //LAYER_COLOR_RANDOM, LAYER_ALPHA_OVER_LIFE, LAYER_COLOR_OVER_LIFE,
    colorRandomGradient->Init(0, 1, "random color");
    colorRandomGradient->SetValues(DAVA::PropLineWrapper<DAVA::Color>(DAVA::PropertyLineHelper::GetValueLine(layer->colorRandom)).GetProps());

    colorOverLifeGradient->Init(0, 1, "color over life");
    colorOverLifeGradient->SetValues(DAVA::PropLineWrapper<DAVA::Color>(DAVA::PropertyLineHelper::GetValueLine(layer->colorOverLife)).GetProps());
    alphaOverLifeTimeLine->Init(0, 1, updateMinimized);
    alphaOverLifeTimeLine->SetMinLimits(0);
    alphaOverLifeTimeLine->SetMaxLimits(1.f);
    alphaOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->alphaOverLife)).GetProps(), Qt::blue, "alpha over life");

    gradientColorForBlackPicker->Init(0, 1, "Gradient color for black");
    gradientColorForBlackPicker->SetValues(DAVA::PropLineWrapper<DAVA::Color>(DAVA::PropertyLineHelper::GetValueLine(layer->gradientColorForBlack)).GetProps());

    gradientColorForMiddlePicker->Init(0, 1, "Gradient color for middle");
    gradientColorForMiddlePicker->SetValues(DAVA::PropLineWrapper<DAVA::Color>(DAVA::PropertyLineHelper::GetValueLine(layer->gradientColorForMiddle)).GetProps());

    gradientColorForWhitePicker->Init(0, 1, "Gradient color for white");
    gradientColorForWhitePicker->SetValues(DAVA::PropLineWrapper<DAVA::Color>(DAVA::PropertyLineHelper::GetValueLine(layer->gradientColorForWhite)).GetProps());

    gradientMiddlePointTimeLine->Init(0.0f, 1.0f, updateMinimized);
    gradientMiddlePointTimeLine->SetMinLimits(0.001f);
    gradientMiddlePointTimeLine->SetMaxLimits(0.999f);
    gradientMiddlePointTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->gradientMiddlePointLine)).GetProps(), Qt::red, "gradient middle point");

    frameOverlifeCheckBox->setChecked(layer->frameOverLifeEnabled);
    frameOverlifeFPSSpin->setValue(layer->frameOverLifeFPS);
    frameOverlifeFPSSpin->setEnabled(layer->frameOverLifeEnabled);
    randomFrameOnStartCheckBox->setChecked(layer->randomFrameOnStart);
    loopSpriteAnimationCheckBox->setChecked(layer->loopSpriteAnimation);

    animSpeedOverLifeTimeLine->Init(0, 1, updateMinimized);
    animSpeedOverLifeTimeLine->SetMinLimits(0);
    animSpeedOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->animSpeedOverLife)).GetProps(), Qt::blue, "anim speed over life");

    angleTimeLine->Init(layer->startTime, lifeTime, updateMinimized);
    angleTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->angle)).GetProps(), Qt::blue, "angle");
    angleTimeLine->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(layer->angleVariation)).GetProps(), Qt::darkGreen, "angle variation");
    angleTimeLine->SetMinLimits(ANGLE_MIN_LIMIT_DEGREES);
    angleTimeLine->SetMaxLimits(ANGLE_MAX_LIMIT_DEGREES);
    angleTimeLine->SetYLegendMark(DEGREE_MARK_CHARACTER);

    //LAYER_START_TIME, LAYER_END_TIME
    startTimeSpin->setMinimum(0);
    startTimeSpin->setMaximum(layer->endTime);
    startTimeSpin->setValue(layer->startTime);
    endTimeSpin->setMinimum(0);
    endTimeSpin->setValue(layer->endTime);

    // LAYER delta, deltaVariation, loopEnd and loopVariation
    bool isLoopedChecked = isLoopedCheckBox->isChecked();
    deltaSpin->setMinimum(0);
    deltaSpin->setValue(layer->deltaTime);
    deltaSpin->setVisible(isLoopedChecked);
    deltaSpinLabel->setVisible(isLoopedChecked);

    deltaVariationSpin->setMinimum(0);
    deltaVariationSpin->setValue(layer->deltaVariation);
    deltaVariationSpin->setVisible(isLoopedChecked);
    deltaVariationSpinLabel->setVisible(isLoopedChecked);

    loopEndSpin->setMinimum(0);
    loopEndSpin->setValue(layer->loopEndTime);
    loopEndSpin->setVisible(isLoopedChecked);
    loopEndSpinLabel->setVisible(isLoopedChecked);

    loopVariationSpin->setMinimum(0);
    loopVariationSpin->setValue(layer->loopVariation);
    loopVariationSpin->setVisible(isLoopedChecked);
    loopVariationSpinLabel->setVisible(isLoopedChecked);

    const DAVA::Vector2& layerPivotPoint = layer->layerPivotPoint;
    pivotPointXSpinBox->setValue((double)layerPivotPoint.x);
    pivotPointYSpinBox->setValue((double)layerPivotPoint.y);

    blockSignals = false;

    adjustSize();
}

void EmitterLayerWidget::UpdateLayerSprite()
{
    UpdateEditorTexture(layer->sprite, layer->spritePath, spritePathLabel, spriteLabel, spriteUpdateTexturesStack);
}

void EmitterLayerWidget::UpdateFlowmapSprite()
{
    UpdateEditorTexture(layer->flowmap, layer->flowmapPath, flowSpritePathLabel, flowSpriteLabel, flowSpriteUpdateTexturesStack);
}

void EmitterLayerWidget::UpdateNoiseSprite()
{
    UpdateEditorTexture(layer->noise, layer->noisePath, noiseSpritePathLabel, noiseSpriteLabel, noiseSpriteUpdateTexturesStack);
}

void EmitterLayerWidget::UpdateAlphaRemapSprite()
{
    UpdateEditorTexture(layer->alphaRemapSprite, layer->alphaRemapPath, alphaRemapSpritePathLabel, alphaRemapSpriteLabel, alphaRemapSpriteUpdateTexturesStack);
}

void EmitterLayerWidget::UpdateEditorTexture(DAVA::Sprite* sprite, DAVA::FilePath& filePath, QLineEdit* pathLabel, QLabel* spriteLabel, DAVA::Stack<std::pair<rhi::HSyncObject, DAVA::Texture*>>& textureStack)
{
    if (sprite && !filePath.IsEmpty())
    {
        DAVA::RenderSystem2D::RenderTargetPassDescriptor desc;
        DAVA::Texture* dstTex = DAVA::Texture::CreateFBO(SPRITE_SIZE, SPRITE_SIZE, DAVA::FORMAT_RGBA8888);
        desc.colorAttachment = dstTex->handle;
        desc.depthAttachment = dstTex->handleDepthStencil;
        desc.width = dstTex->GetWidth();
        desc.height = dstTex->GetHeight();
        desc.clearTarget = true;
        desc.transformVirtualToPhysical = false;
        desc.clearColor = DAVA::Color::Clear;
        DAVA::RenderSystem2D::Instance()->BeginRenderTargetPass(desc);
        {
            DAVA::SpriteDrawState drawState = {};
            drawState.SetScaleSize(SPRITE_SIZE, SPRITE_SIZE, sprite->GetWidth(), sprite->GetHeight());
            DAVA::RenderSystem2D::Instance()->Draw(sprite, &drawState, DAVA::Color::White);
        }
        DAVA::RenderSystem2D::Instance()->EndRenderTargetPass();
        textureStack.push({ rhi::GetCurrentFrameSyncObject(), dstTex });
        spriteUpdateTimer->start(0);

        QString path = QString::fromStdString(filePath.GetAbsolutePathname());
        path = EmitterLayerWidgetDetails::ConvertSpritePathToPSD(path);
        pathLabel->setText(path);
    }
    else
    {
        spriteLabel->setPixmap(QPixmap());
        pathLabel->setText("");
    }
}

void EmitterLayerWidget::CreateFlowmapLayoutWidget()
{
    flowLayoutWidget = new QWidget();
    QVBoxLayout* flowMainLayout = new QVBoxLayout(flowLayoutWidget);

    QHBoxLayout* flowTextureHBox2 = new QHBoxLayout();
    flowTextureBtn = new QPushButton("Set flow texture", this);
    flowTextureBtn->setMinimumHeight(30);
    flowTextureFolderBtn = new QPushButton("Change flow texture folder", this);
    flowTextureFolderBtn->setMinimumHeight(30);
    flowTextureHBox2->addWidget(flowTextureBtn);
    flowTextureHBox2->addWidget(flowTextureFolderBtn);

    QVBoxLayout* flowTextureVBox = new QVBoxLayout();
    flowSpritePathLabel = new QLineEdit(this);
    flowSpritePathLabel->setReadOnly(false);
    flowTextureVBox->addLayout(flowTextureHBox2);
    flowTextureVBox->addWidget(flowSpritePathLabel);

    QHBoxLayout* flowTextureHBox = new QHBoxLayout();
    flowSpriteLabel = new QLabel(this);
    flowSpriteLabel->setMinimumSize(SPRITE_SIZE, SPRITE_SIZE);
    flowTextureHBox->addWidget(flowSpriteLabel);
    flowTextureHBox->addLayout(flowTextureVBox);

    flowMainLayout->addLayout(flowTextureHBox);

    enableFlowAnimationCheckBox = new QCheckBox("Enable flowmap animation");
    mainBox->addWidget(enableFlowAnimationCheckBox);
    connect(enableFlowAnimationCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnFlowPropertiesChanged()));
    flowMainLayout->addWidget(enableFlowAnimationCheckBox);

    flowSettingsLayoutWidget = new QWidget();
    flowMainLayout->addWidget(flowSettingsLayoutWidget);

    QVBoxLayout* flowVBox = new QVBoxLayout(flowSettingsLayoutWidget);
    flowSpeedTimeLine = new TimeLineWidget(this);
    connect(flowSpeedTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnFlowPropertiesChanged()));
    flowVBox->addWidget(flowSpeedTimeLine);

    flowSpeedVariationTimeLine = new TimeLineWidget(this);
    connect(flowSpeedVariationTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnFlowPropertiesChanged()));
    flowVBox->addWidget(flowSpeedVariationTimeLine);

    // Offset.
    flowOffsetTimeLine = new TimeLineWidget(this);
    connect(flowOffsetTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnFlowPropertiesChanged()));
    flowVBox->addWidget(flowOffsetTimeLine);

    flowOffsetVariationTimeLine = new TimeLineWidget(this);
    connect(flowOffsetVariationTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnFlowPropertiesChanged()));
    flowVBox->addWidget(flowOffsetVariationTimeLine);

    connect(flowTextureBtn, SIGNAL(clicked(bool)), this, SLOT(OnFlowSpriteBtn()));
    connect(flowTextureFolderBtn, SIGNAL(clicked(bool)), this, SLOT(OnFlowFolderBtn()));
    connect(flowSpritePathLabel, SIGNAL(textChanged(const QString&)), this, SLOT(OnFlowTexturePathChanged(const QString&)));
    connect(flowSpritePathLabel, SIGNAL(textEdited(const QString&)), this, SLOT(OnFlowSpritePathEdited(const QString&)));
    flowSpritePathLabel->installEventFilter(this);
}

void EmitterLayerWidget::CreateNoiseLayoutWidget()
{
    noiseLayoutWidget = new QWidget();
    QVBoxLayout* noiseMainLayout = new QVBoxLayout(noiseLayoutWidget);

    QHBoxLayout* noiseTextureHBox2 = new QHBoxLayout();
    noiseTextureBtn = new QPushButton("Set noise texture", this);
    noiseTextureBtn->setMinimumHeight(30);
    noiseTextureFolderBtn = new QPushButton("Change noise texture folder", this);
    noiseTextureFolderBtn->setMinimumHeight(30);
    noiseTextureHBox2->addWidget(noiseTextureBtn);
    noiseTextureHBox2->addWidget(noiseTextureFolderBtn);

    QVBoxLayout* noiseTextureVBox = new QVBoxLayout();
    noiseSpritePathLabel = new QLineEdit(this);
    noiseSpritePathLabel->setReadOnly(false);
    noiseTextureVBox->addLayout(noiseTextureHBox2);
    noiseTextureVBox->addWidget(noiseSpritePathLabel);

    QHBoxLayout* noiseTextureHBox = new QHBoxLayout();
    noiseSpriteLabel = new QLabel(this);
    noiseSpriteLabel->setMinimumSize(SPRITE_SIZE, SPRITE_SIZE);
    noiseTextureHBox->addWidget(noiseSpriteLabel);
    noiseTextureHBox->addLayout(noiseTextureVBox);

    noiseMainLayout->addLayout(noiseTextureHBox);

    QVBoxLayout* timelineVBox = new QVBoxLayout();
    noiseScaleTimeLine = new TimeLineWidget(this);
    connect(noiseScaleTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnNoisePropertiesChanged()));
    timelineVBox->addWidget(noiseScaleTimeLine);

    noiseScaleVariationTimeLine = new TimeLineWidget(this);
    connect(noiseScaleVariationTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnNoisePropertiesChanged()));
    timelineVBox->addWidget(noiseScaleVariationTimeLine);

    noiseScaleOverLifeTimeLine = new TimeLineWidget(this);
    connect(noiseScaleOverLifeTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnNoisePropertiesChanged()));
    timelineVBox->addWidget(noiseScaleOverLifeTimeLine);

    noiseMainLayout->addLayout(timelineVBox);

    enableNoiseScrollCheckBox = new QCheckBox("Use noise scroll");
    mainBox->addWidget(enableNoiseScrollCheckBox);
    connect(enableNoiseScrollCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnNoisePropertiesChanged()));
    noiseMainLayout->addWidget(enableNoiseScrollCheckBox);

    noiseScrollWidget = new QWidget();
    noiseScrollWidget->setVisible(true);
    QVBoxLayout* noiseUvScrollLayout = new QVBoxLayout(noiseScrollWidget);
    noiseMainLayout->addWidget(noiseScrollWidget);
    noiseUVScrollSpeedTimeLine = new TimeLineWidget(this);
    connect(noiseUVScrollSpeedTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnNoisePropertiesChanged()));
    noiseUvScrollLayout->addWidget(noiseUVScrollSpeedTimeLine);

    noiseUVScrollSpeedVariationTimeLine = new TimeLineWidget(this);
    connect(noiseUVScrollSpeedVariationTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnNoisePropertiesChanged()));
    noiseUvScrollLayout->addWidget(noiseUVScrollSpeedVariationTimeLine);

    noiseUVScrollSpeedOverLifeTimeLine = new TimeLineWidget(this);
    connect(noiseUVScrollSpeedOverLifeTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnNoisePropertiesChanged()));
    noiseUvScrollLayout->addWidget(noiseUVScrollSpeedOverLifeTimeLine);

    connect(noiseTextureBtn, SIGNAL(clicked(bool)), this, SLOT(OnNoiseSpriteBtn()));
    connect(noiseTextureFolderBtn, SIGNAL(clicked(bool)), this, SLOT(OnNoiseSpriteBtn()));
    connect(noiseSpritePathLabel, SIGNAL(textChanged(const QString&)), this, SLOT(OnNoiseTexturePathChanged(const QString&)));
    connect(noiseSpritePathLabel, SIGNAL(textEdited(const QString&)), this, SLOT(OnNoiseSpritePathEdited(const QString&)));
    noiseSpritePathLabel->installEventFilter(this);
}

void EmitterLayerWidget::CreateStripeLayoutWidget()
{
    stripeLayoutWidget = new QWidget();
    QVBoxLayout* vertStripeLayout = new QVBoxLayout(stripeLayoutWidget);
    vertStripeLayout->setContentsMargins(0, 20, 0, 20);
    stripeLabel = new QLabel("Stripe Settings:");
    vertStripeLayout->addWidget(stripeLabel);

    stripeColorOverLifeGradient = new GradientPickerWidget(this);
    connect(stripeColorOverLifeGradient,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnStripePropertiesChanged()));
    vertStripeLayout->addWidget(stripeColorOverLifeGradient);

    stripeSizeOverLifeTimeLine = new TimeLineWidget(this);
    connect(stripeSizeOverLifeTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnStripePropertiesChanged()));

    stripeTextureTileTimeLine = new TimeLineWidget(this);
    connect(stripeTextureTileTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnStripePropertiesChanged()));

    stripeNoiseScrollSpeedOverLifeTimeLine = new TimeLineWidget(this);
    connect(stripeNoiseScrollSpeedOverLifeTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnStripePropertiesChanged()));

    stripeInheritPositionForBaseCheckBox = new QCheckBox("Inherit position. Affect only base verts.");
    vertStripeLayout->addWidget(stripeInheritPositionForBaseCheckBox);
    connect(stripeInheritPositionForBaseCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnStripePropertiesChanged()));

    stripeUsePerspectiveMappingCheckBox = new QCheckBox("Use perspective mapping.");
    vertStripeLayout->addWidget(stripeUsePerspectiveMappingCheckBox);
    connect(stripeUsePerspectiveMappingCheckBox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnStripePropertiesChanged()));

    stripeLifetimeSpin = new EventFilterDoubleSpinBox();
    stripeLifetimeSpin->setMinimum(-100);
    stripeLifetimeSpin->setMaximum(100);
    stripeLifetimeSpin->setSingleStep(0.01);
    stripeLifetimeSpin->setDecimals(4);

    stripeVertexSpawnStepSpin = new EventFilterDoubleSpinBox();
    stripeVertexSpawnStepSpin->setMinimum(-100);
    stripeVertexSpawnStepSpin->setMaximum(100);
    stripeVertexSpawnStepSpin->setSingleStep(0.01);
    stripeVertexSpawnStepSpin->setDecimals(4);

    stripeStartSizeSpin = new EventFilterDoubleSpinBox();
    stripeStartSizeSpin->setMinimum(-std::numeric_limits<double>::infinity());
    stripeStartSizeSpin->setMaximum(std::numeric_limits<double>::infinity());
    stripeStartSizeSpin->setSingleStep(0.01);
    stripeStartSizeSpin->setDecimals(4);
    stripeLifetimeLabel = new QLabel("Edge lifetime:");
    stripeVertexSpawnStepLabel = new QLabel("Vertex Spawn Step:");
    stripeStartSizeLabel = new QLabel("Start size:");

    vertStripeLayout->addWidget(stripeStartSizeLabel);
    vertStripeLayout->addWidget(stripeStartSizeSpin);

    vertStripeLayout->addWidget(stripeSizeOverLifeTimeLine);
    vertStripeLayout->addWidget(stripeTextureTileTimeLine);

    vertStripeLayout->addWidget(stripeLifetimeLabel);
    vertStripeLayout->addWidget(stripeLifetimeSpin);
    vertStripeLayout->addWidget(stripeVertexSpawnStepLabel);
    vertStripeLayout->addWidget(stripeVertexSpawnStepSpin);

    stripeUScrollSpeedSpin = new EventFilterDoubleSpinBox();
    stripeUScrollSpeedSpin->setMinimum(-100);
    stripeUScrollSpeedSpin->setMaximum(100);
    stripeUScrollSpeedSpin->setSingleStep(0.01);
    stripeUScrollSpeedSpin->setDecimals(4);

    stripeVScrollSpeedSpin = new EventFilterDoubleSpinBox();
    stripeVScrollSpeedSpin->setMinimum(-100);
    stripeVScrollSpeedSpin->setMaximum(100);
    stripeVScrollSpeedSpin->setSingleStep(0.01);
    stripeVScrollSpeedSpin->setDecimals(4);

    stripeUScrollSpeedLabel = new QLabel("U scroll");
    stripeVScrollSpeedLabel = new QLabel("V scroll");

    stripeFadeDistanceFromTopSpin = new EventFilterDoubleSpinBox();
    stripeFadeDistanceFromTopSpin->setMinimum(0);
    stripeFadeDistanceFromTopSpin->setMaximum(100);
    stripeFadeDistanceFromTopSpin->setSingleStep(0.1f);
    stripeFadeDistanceFromTopSpin->setDecimals(2);

    stripeFadeDistanceFromTopLabel = new QLabel("Stripe fade distance from top");

    vertStripeLayout->addWidget(stripeUScrollSpeedLabel);
    vertStripeLayout->addWidget(stripeUScrollSpeedSpin);
    vertStripeLayout->addWidget(stripeVScrollSpeedLabel);
    vertStripeLayout->addWidget(stripeVScrollSpeedSpin);
    vertStripeLayout->addWidget(stripeFadeDistanceFromTopLabel);
    vertStripeLayout->addWidget(stripeFadeDistanceFromTopSpin);

    vertStripeLayout->addWidget(stripeNoiseScrollSpeedOverLifeTimeLine);

    connect(stripeLifetimeSpin, SIGNAL(valueChanged(double)), this, SLOT(OnStripePropertiesChanged()));
    connect(stripeVertexSpawnStepSpin, SIGNAL(valueChanged(double)), this, SLOT(OnStripePropertiesChanged()));
    connect(stripeStartSizeSpin, SIGNAL(valueChanged(double)), this, SLOT(OnStripePropertiesChanged()));
    connect(stripeFadeDistanceFromTopSpin, SIGNAL(valueChanged(double)), this, SLOT(OnStripePropertiesChanged()));

    connect(stripeUScrollSpeedSpin, SIGNAL(valueChanged(double)), this, SLOT(OnStripePropertiesChanged()));
    connect(stripeVScrollSpeedSpin, SIGNAL(valueChanged(double)), this, SLOT(OnStripePropertiesChanged()));
}

void EmitterLayerWidget::CreateAlphaRemapLayoutWidget()
{
    alphaRemapLayoutWidget = new QWidget();
    QVBoxLayout* alphaRemapMainLayout = new QVBoxLayout(alphaRemapLayoutWidget);

    QHBoxLayout* alphaTextureHBox2 = new QHBoxLayout();
    alphaRemapTextureBtn = new QPushButton("Set alpha remap texture", this);
    alphaRemapTextureBtn->setMinimumHeight(30);
    alphaRemapTextureFolderBtn = new QPushButton("Change alpha remap texture folder", this);
    alphaRemapTextureFolderBtn->setMinimumHeight(30);
    alphaTextureHBox2->addWidget(alphaRemapTextureBtn);
    alphaTextureHBox2->addWidget(alphaRemapTextureFolderBtn);

    QVBoxLayout* alphaRemapTextureVBox = new QVBoxLayout();
    alphaRemapSpritePathLabel = new QLineEdit(this);
    alphaRemapSpritePathLabel->setReadOnly(false);
    alphaRemapTextureVBox->addLayout(alphaTextureHBox2);
    alphaRemapTextureVBox->addWidget(alphaRemapSpritePathLabel);

    QHBoxLayout* alphaTextureHBox = new QHBoxLayout();
    alphaRemapSpriteLabel = new QLabel(this);
    alphaRemapSpriteLabel->setMinimumSize(SPRITE_SIZE, SPRITE_SIZE);
    alphaTextureHBox->addWidget(alphaRemapSpriteLabel);
    alphaTextureHBox->addLayout(alphaRemapTextureVBox);

    alphaRemapMainLayout->addLayout(alphaTextureHBox);

    QVBoxLayout* alphaVBox = new QVBoxLayout();
    alphaRemapOverLifeTimeLine = new TimeLineWidget(this);
    connect(alphaRemapOverLifeTimeLine,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnAlphaRemapPropertiesChanged()));
    alphaVBox->addWidget(alphaRemapOverLifeTimeLine);
    alphaRemapMainLayout->addLayout(alphaVBox);

    alphaRemapLoopCountSpin = new EventFilterDoubleSpinBox();
    alphaRemapLoopCountSpin->setMinimum(1);
    alphaRemapLoopCountSpin->setMaximum(50);
    alphaRemapLoopCountSpin->setSingleStep(1);
    alphaRemapLoopCountSpin->setDecimals(0);

    alphaRemapLoopLabel = new QLabel("Loop count");
    QHBoxLayout* alphaHBox = new QHBoxLayout();
    alphaHBox->addWidget(alphaRemapLoopLabel);
    alphaHBox->addWidget(alphaRemapLoopCountSpin);
    alphaRemapMainLayout->addLayout(alphaHBox);

    connect(alphaRemapTextureBtn, SIGNAL(clicked(bool)), this, SLOT(OnAlphaRemapBtn()));
    connect(alphaRemapTextureFolderBtn, SIGNAL(clicked(bool)), this, SLOT(OnAlphaRemapFolderBtn()));
    connect(alphaRemapSpritePathLabel, SIGNAL(textChanged(const QString&)), this, SLOT(OnAlphaRemapTexturePathChanged(const QString&)));
    connect(alphaRemapSpritePathLabel, SIGNAL(textEdited(const QString&)), this, SLOT(OnAlphaRemapSpritePathEdited(const QString&)));
    connect(alphaRemapLoopCountSpin, SIGNAL(valueChanged(double)), this, SLOT(OnAlphaRemapPropertiesChanged()));
    alphaRemapSpritePathLabel->installEventFilter(this);
}

QLayout* EmitterLayerWidget::CreateFresnelToAlphaLayout()
{
    QHBoxLayout* longFresLayout = new QHBoxLayout();
    longFresLayout->setContentsMargins(0, 10, 0, 0);
    fresnelToAlphaCheckbox = new QCheckBox("Fresnel to alpha");
    longFresLayout->addWidget(fresnelToAlphaCheckbox);
    connect(fresnelToAlphaCheckbox,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(OnFresnelToAlphaChanged()));

    fresnelBiasSpinBox = new EventFilterDoubleSpinBox();
    fresnelBiasSpinBox->setMinimum(0);
    fresnelBiasSpinBox->setMaximum(1);
    fresnelBiasSpinBox->setSingleStep(0.01);
    fresnelBiasSpinBox->setDecimals(3);

    fresnelPowerSpinBox = new EventFilterDoubleSpinBox();
    fresnelPowerSpinBox->setMinimum(0);
    fresnelPowerSpinBox->setMaximum(50);
    fresnelPowerSpinBox->setSingleStep(1);
    fresnelPowerSpinBox->setDecimals(0);

    fresnelBiasLabel = new QLabel("Fresnel to alpha bias:");
    fresnelPowerLabel = new QLabel("Fresnel to alpha power:");
    longFresLayout->addWidget(fresnelPowerLabel);
    longFresLayout->addWidget(fresnelPowerSpinBox);
    longFresLayout->addWidget(fresnelBiasLabel);
    longFresLayout->addWidget(fresnelBiasSpinBox);
    connect(fresnelPowerSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnFresnelToAlphaChanged()));
    connect(fresnelBiasSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnFresnelToAlphaChanged()));
    return longFresLayout;
}

void EmitterLayerWidget::OnChangeSpriteButton(const DAVA::FilePath& initialFilePath, QLineEdit* spriteLabel, QString&& caption, DAVA::Function<void(const QString&)> pathEditFunc)
{
    QString startPath;
    if (initialFilePath.IsEmpty())
    {
        DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
        DVASSERT(data != nullptr);
        startPath = QString::fromStdString(data->GetParticlesGfxPath().GetAbsolutePathname());
    }
    else
    {
        startPath = QString::fromStdString(initialFilePath.GetDirectory().GetStringValue());
        startPath = EmitterLayerWidgetDetails::ConvertSpritePathToPSD(startPath);
    }

    DAVA::FileDialogParams params;
    params.dir = startPath;
    params.filters = QString("Sprite File (*.psd)");
    params.title = caption;

    QString selectedPath = DAVA::Deprecated::GetUI()->GetOpenFileName(DAVA::mainWindowKey, params);
    std::string s = selectedPath.toStdString();
    if (selectedPath.isEmpty())
    {
        return;
    }

    selectedPath.truncate(selectedPath.lastIndexOf('.'));
    s = selectedPath.toStdString();
    if (selectedPath == spriteLabel->text())
    {
        return;
    }
    s = selectedPath.toStdString();
    spriteLabel->setText(selectedPath);

    pathEditFunc(selectedPath);
}

void EmitterLayerWidget::OnChangeFolderButton(const DAVA::FilePath& initialFilePath, QLineEdit* pathLabel, DAVA::Function<void(const QString&)> pathEditFunc)
{
    if (initialFilePath.IsEmpty())
    {
        return;
    }

    QString startPath = QString::fromStdString(initialFilePath.GetDirectory().GetStringValue());
    startPath = EmitterLayerWidgetDetails::ConvertSpritePathToPSD(startPath);

    QString spriteName = QString::fromStdString(initialFilePath.GetBasename());

    DAVA::DirectoryDialogParams params;
    params.dir = startPath;
    params.title = QString("Select particle sprites directory");
    QString selectedPath = DAVA::Deprecated::GetUI()->GetExistingDirectory(DAVA::mainWindowKey, params);
    if (selectedPath.isEmpty())
    {
        return;
    }
    selectedPath += "/";
    selectedPath += spriteName;

    pathLabel->setText(selectedPath);

    pathEditFunc(selectedPath);
}

void EmitterLayerWidget::CheckPath(const QString& text)
{
    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data != nullptr);
    const DAVA::FilePath& particlesGfxPath = data->GetParticlesGfxPath();
    const DAVA::FilePath spritePath = text.toStdString();
    const DAVA::String relativePathForParticlesPath = spritePath.GetRelativePathname(particlesGfxPath);

    if (relativePathForParticlesPath.find("../") != DAVA::String::npos)
    {
        DAVA::ModalMessageParams params;
        params.title = "Warning";
        params.message = QString("You've opened particle sprite from incorrect path (%1).\n Correct one is %2.").arg(QString::fromStdString(spritePath.GetDirectory().GetAbsolutePathname())).arg(QString::fromStdString(particlesGfxPath.GetAbsolutePathname()));
        params.icon = DAVA::ModalMessageParams::Warning;
        params.buttons = DAVA::ModalMessageParams::Yes;
        params.defaultButton = DAVA::ModalMessageParams::Yes;

        DAVA::Deprecated::GetUI()->ShowModalMessage(DAVA::mainWindowKey, params);
    }
}

void EmitterLayerWidget::UpdateTooltip(QLineEdit* label)
{
    QFontMetrics fm = label->fontMetrics();
    if (fm.width(label->text()) >= label->width())
    {
        label->setToolTip(label->text());
    }
    else
    {
        label->setToolTip("");
    }
}

bool EmitterLayerWidget::eventFilter(QObject* o, QEvent* e)
{
    if (e->type() == QEvent::Wheel &&
        qobject_cast<QAbstractSpinBox*>(o))
    {
        e->ignore();
        return true;
    }

    QLineEdit* label = qobject_cast<QLineEdit*>(o);
    if (e->type() == QEvent::Resize && label != nullptr)
    {
        UpdateTooltip(label);
        return true;
    }

    return QWidget::eventFilter(o, e);
}

void EmitterLayerWidget::OnSpritePathChanged(const QString& text)
{
    UpdateTooltip(spritePathLabel);
}

void EmitterLayerWidget::OnSpritePathEdited(const QString& text)
{
    CheckPath(text);
    OnLayerMaterialValueChanged();
}

void EmitterLayerWidget::OnFlowSpritePathEdited(const QString& text)
{
    CheckPath(text);
    OnFlowPropertiesChanged();
}

void EmitterLayerWidget::OnNoiseSpritePathEdited(const QString& text)
{
    CheckPath(text);
    OnNoisePropertiesChanged();
}

void EmitterLayerWidget::OnAlphaRemapSpritePathEdited(const QString& text)
{
    CheckPath(text);
    OnAlphaRemapPropertiesChanged();
}

void EmitterLayerWidget::OnFlowSpriteBtn()
{
    OnChangeSpriteButton(layer->flowmapPath, flowSpritePathLabel, QString("Open flow texture"), std::bind(&EmitterLayerWidget::OnFlowSpritePathEdited, this, std::placeholders::_1));
}

void EmitterLayerWidget::OnFlowFolderBtn()
{
    OnChangeFolderButton(layer->flowmapPath, flowSpritePathLabel, std::bind(&EmitterLayerWidget::OnFlowSpritePathEdited, this, std::placeholders::_1));
}

void EmitterLayerWidget::OnFlowTexturePathChanged(const QString& text)
{
    UpdateTooltip(flowSpritePathLabel);
}

void EmitterLayerWidget::OnNoiseSpriteBtn()
{
    OnChangeSpriteButton(layer->noisePath, noiseSpritePathLabel, QString("Open noise texture"), std::bind(&EmitterLayerWidget::OnNoiseSpritePathEdited, this, std::placeholders::_1));
}

void EmitterLayerWidget::OnNoiseFolderBtn()
{
    OnChangeFolderButton(layer->noisePath, noiseSpritePathLabel, std::bind(&EmitterLayerWidget::OnNoiseSpritePathEdited, this, std::placeholders::_1));
}

void EmitterLayerWidget::OnNoiseTexturePathChanged(const QString& text)
{
    UpdateTooltip(noiseSpritePathLabel);
}

void EmitterLayerWidget::OnAlphaRemapBtn()
{
    OnChangeSpriteButton(layer->alphaRemapPath, alphaRemapSpritePathLabel, QString("Open alpha remap texture"), std::bind(&EmitterLayerWidget::OnAlphaRemapSpritePathEdited, this, std::placeholders::_1));
}

void EmitterLayerWidget::OnAlphaRemapFolderBtn()
{
    OnChangeFolderButton(layer->alphaRemapPath, alphaRemapSpritePathLabel, std::bind(&EmitterLayerWidget::OnAlphaRemapSpritePathEdited, this, std::placeholders::_1));
}

void EmitterLayerWidget::OnAlphaRemapTexturePathChanged(const QString& text)
{
    UpdateTooltip(alphaRemapSpritePathLabel);
}

void EmitterLayerWidget::FillLayerTypes()
{
    DAVA::int32 layerTypes = sizeof(layerTypeMap) / sizeof(*layerTypeMap);
    for (DAVA::int32 i = 0; i < layerTypes; i++)
    {
        layerTypeComboBox->addItem(layerTypeMap[i].layerName);
    }
}

DAVA::int32 EmitterLayerWidget::LayerTypeToIndex(DAVA::ParticleLayer::eType layerType)
{
    DAVA::int32 layerTypes = sizeof(layerTypeMap) / sizeof(*layerTypeMap);
    for (DAVA::int32 i = 0; i < layerTypes; i++)
    {
        if (layerTypeMap[i].layerType == layerType)
        {
            return i;
        }
    }

    return 0;
}

void EmitterLayerWidget::FillTimeLineWidgetIndentifiers()
{
    timeLineWidgetsIdentifiers =
    {
      { "LAYER_LIFE_PROPS", lifeTimeLine },
      { "LAYER_NUMBER_PROPS", numberTimeLine },
      { "LAYER_SIZE_PROPS", sizeTimeLine },
      { "LAYER_SIZE_VARIATION_PROPS", sizeVariationTimeLine },
      { "LAYER_SIZE_OVER_LIFE_PROPS", sizeOverLifeTimeLine },
      { "LAYER_VELOCITY_PROPS", velocityTimeLine },
      { "LAYER_VELOCITY_OVER_LIFE", velocityOverLifeTimeLine },
      { "LAYER_SPIN_PROPS", spinTimeLine },
      { "LAYER_SPIN_OVER_LIFE_PROPS", spinOverLifeTimeLine },
      { "LAYER_ANIM_SPEED_OVER_LIFE_PROPS", animSpeedOverLifeTimeLine },
      { "LAYER_ALPHA_OVER_LIFE_PROPS", alphaOverLifeTimeLine },
      { "LAYER_ANGLE", angleTimeLine },
      { "LAYER_FLOW_SPEED_PROPS", flowSpeedTimeLine },
      { "LAYER_FLOW_SPEED_VARIATION_PROPS", flowSpeedVariationTimeLine },
      { "LAYER_FLOW_OFFSET_PROPS", flowOffsetTimeLine },
      { "LAYER_FLOW_OFFSET_VARIATION_PROPS", flowOffsetVariationTimeLine },
      { "LAYER_NOISE_SCALE_PROPS", noiseScaleTimeLine },
      { "LAYER_NOISE_SCALE_VARIATION_PROPS", noiseScaleVariationTimeLine },
      { "LAYER_NOISE_SCALE_PROPS", noiseScaleOverLifeTimeLine },
      { "LAYER_NOISE_UV_SCROLL_SPEED_PROPS", noiseUVScrollSpeedTimeLine },
      { "LAYER_NOISE_UV_SCROLL_SPEED_VARIATION_PROPS", noiseUVScrollSpeedVariationTimeLine },
      { "LAYER_NOISE_UV_SCROLL_SPEED_OVER_LIFE", noiseUVScrollSpeedOverLifeTimeLine },
      { "LAYER_STRIPE_SIZE_OVER_LIFE_TIME_PROPS", stripeSizeOverLifeTimeLine },
      { "LAYER_STRIPE_TILE_PROPS", stripeTextureTileTimeLine },
      { "LAYER_STRIPE_NOISE_SCROLL_SPEED_OVER_LIFE", stripeNoiseScrollSpeedOverLifeTimeLine },
      { "LAYER_ALPHA_REMAP", alphaRemapOverLifeTimeLine }
    };
}

void EmitterLayerWidget::SetLayerMode(eLayerMode layerMode)
{
    bool isSuperemitter = layerMode == eLayerMode::SUPEREMITTER;
    bool isStripe = layerMode == eLayerMode::STRIPE;
    sizeTimeLine->setVisible(!isStripe);
    sizeVariationTimeLine->setVisible(!isStripe);
    sizeOverLifeTimeLine->setVisible(!isStripe);
    angleTimeLine->setVisible(!isStripe);
    randomSpinDirectionCheckBox->setVisible(!isStripe);

    xFacingCheckBox->setText(isStripe ? "X-Align" : "X-Facing");
    yFacingCheckBox->setText(isStripe ? "Y-Align" : "Y-Facing");
    zFacingCheckBox->setText(isStripe ? "Z-Align" : "Z-Facing");

    spinTimeLine->setVisible(!isStripe);
    spinOverLifeTimeLine->setVisible(!isStripe);

    applyGlobalForcesCheckBox->setVisible(!isSuperemitter);

    enableFlowCheckBox->setVisible(!isSuperemitter && !isStripe);
    flowLayoutWidget->setVisible(!isSuperemitter && enableFlowCheckBox->isChecked() && !isStripe);

    stripeLayoutWidget->setVisible(!isSuperemitter && isStripe);

    enableNoiseCheckBox->setVisible(!isSuperemitter);
    noiseLayoutWidget->setVisible(!isSuperemitter && enableNoiseCheckBox->isChecked());

    enableAlphaRemapCheckBox->setVisible(!isSuperemitter);
    alphaRemapLayoutWidget->setVisible(!isSuperemitter && enableAlphaRemapCheckBox->isChecked());

    fresnelToAlphaCheckbox->setVisible(!isSuperemitter);
    bool fresToAlphaVisible = !isSuperemitter && fresnelToAlphaCheckbox->isChecked();
    fresnelBiasLabel->setVisible(fresToAlphaVisible);
    fresnelBiasSpinBox->setVisible(fresToAlphaVisible);
    fresnelPowerLabel->setVisible(fresToAlphaVisible);
    fresnelPowerSpinBox->setVisible(fresToAlphaVisible);

    // Sprite has no sense for Superemitter.
    spriteBtn->setVisible(!isSuperemitter);
    spriteFolderBtn->setVisible(!isSuperemitter);
    spriteLabel->setVisible(!isSuperemitter);
    spritePathLabel->setVisible(!isSuperemitter);

    // The same is for "Additive" flag, Color, Alpha and Frame.
    colorRandomGradient->setVisible(!isSuperemitter);
    colorOverLifeGradient->setVisible(!isSuperemitter);
    alphaOverLifeTimeLine->setVisible(!isSuperemitter);

    gradientColorForWhitePicker->setVisible(!isSuperemitter && useThreePointGradientBox->isChecked());
    gradientColorForBlackPicker->setVisible(!isSuperemitter && useThreePointGradientBox->isChecked());
    gradientColorForMiddlePicker->setVisible(!isSuperemitter && useThreePointGradientBox->isChecked());
    gradientMiddlePointTimeLine->setVisible(!isSuperemitter && useThreePointGradientBox->isChecked());
    gradientMiddlePointLabel->setVisible(!isSuperemitter && useThreePointGradientBox->isChecked());
    gradientMiddlePointSpin->setVisible(!isSuperemitter && useThreePointGradientBox->isChecked());
    useThreePointGradientBox->setVisible(!isSuperemitter);

    frameOverlifeCheckBox->setVisible(!isSuperemitter && !isStripe);
    frameOverlifeFPSSpin->setVisible(!isSuperemitter && !isStripe);
    frameOverlifeFPSLabel->setVisible(!isSuperemitter && !isStripe);
    randomFrameOnStartCheckBox->setVisible(!isSuperemitter && !isStripe);
    loopSpriteAnimationCheckBox->setVisible(!isSuperemitter && !isStripe);
    animSpeedOverLifeTimeLine->setVisible(!isSuperemitter && !isStripe);

    // The Pivot Point must be hidden for Superemitter mode.
    pivotPointLabel->setVisible(!isSuperemitter && !isStripe);
    pivotPointXSpinBox->setVisible(!isSuperemitter && !isStripe);
    pivotPointXSpinBoxLabel->setVisible(!isSuperemitter && !isStripe);
    pivotPointYSpinBox->setVisible(!isSuperemitter && !isStripe);
    pivotPointYSpinBoxLabel->setVisible(!isSuperemitter && !isStripe);
    pivotPointResetButton->setVisible(!isSuperemitter && !isStripe);

    //particle orientation would be set up in inner emitter layers
    particleOrientationLabel->setVisible(!isSuperemitter);
    cameraFacingCheckBox->setVisible(!isSuperemitter);
    xFacingCheckBox->setVisible(!isSuperemitter);
    yFacingCheckBox->setVisible(!isSuperemitter);
    zFacingCheckBox->setVisible(!isSuperemitter);
    worldAlignCheckBox->setVisible(!isSuperemitter);
    cameraFacingStripeSphericalCheckBox->setVisible(!isSuperemitter && isStripe);

    //blend and fog settings are set in inner emitter layers
    blendOptionsLabel->setVisible(!isSuperemitter);
    presetLabel->setVisible(!isSuperemitter);
    presetComboBox->setVisible(!isSuperemitter);
    fogCheckBox->setVisible(!isSuperemitter);
    frameBlendingCheckBox->setVisible(!isSuperemitter && !isStripe);

    // Some controls are however specific for this mode only - display and update them.
    innerEmitterLabel->setVisible(isSuperemitter);
    innerEmitterPathLabel->setVisible(isSuperemitter);

    if (isSuperemitter && this->layer->innerEmitter)
    {
        innerEmitterPathLabel->setText(QString::fromStdString(layer->innerEmitter->GetEmitter()->configPath.GetAbsolutePathname()));
    }
}

void EmitterLayerWidget::OnPivotPointReset()
{
    blockSignals = true;
    this->pivotPointXSpinBox->setValue(0);
    this->pivotPointYSpinBox->setValue(0);
    blockSignals = false;

    OnValueChanged();
}

void EmitterLayerWidget::OnLayerValueChanged()
{
    // Start/End time and Enabled flag can be changed from external side.
    blockSignals = true;
    if (startTimeSpin->value() != layer->startTime || endTimeSpin->value() != layer->endTime)
    {
        startTimeSpin->setValue(layer->startTime);
        endTimeSpin->setValue(layer->endTime);
    }

    if (deltaSpin->value() != layer->deltaTime || loopEndSpin->value() != layer->loopEndTime)
    {
        deltaSpin->setValue(layer->deltaTime);
        loopEndSpin->setValue(layer->loopEndTime);
    }

    // NOTE: inverse logic here.
    if (enableCheckBox->isChecked() == layer->isDisabled)
    {
        enableCheckBox->setChecked(!layer->isDisabled);
    }

    blockSignals = false;
}
