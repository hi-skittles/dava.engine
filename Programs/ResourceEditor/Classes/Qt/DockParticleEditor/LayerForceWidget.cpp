#include "Classes/Qt/DockParticleEditor/LayerForceWidget.h"

#include "Classes/Qt/DockParticleEditor/ParticleVector3Widget.h"
#include "Classes/Qt/DockParticleEditor/TimeLineWidget.h"
#include "Classes/Qt/DockParticleEditor/WheellIgnorantComboBox.h"
#include "Classes/Qt/Tools/EventFilterDoubleSpinBox/EventFilterDoubleSpinBox.h"

#include <REPlatform/Commands/ParticleEditorCommands.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <Base/Array.h>
#include <Base/Map.h>

#include <QVBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>

#include <limits>

namespace LayerDragForceWidgetDetail
{
struct ShapeMap
{
    DAVA::ParticleForce::eShape elemType;
    QString name;
};
const DAVA::Array<ShapeMap, 2> shapeMap =
{ {
{ DAVA::ParticleForce::eShape::BOX, "Box" },
{ DAVA::ParticleForce::eShape::SPHERE, "Sphere" }
} };

struct TimingMap
{
    DAVA::ParticleForce::eTimingType elemType;
    QString name;
};
const DAVA::Array<TimingMap, 4> timingMap =
{ {
{ DAVA::ParticleForce::eTimingType::CONSTANT, "Constant" },
{ DAVA::ParticleForce::eTimingType::OVER_LAYER_LIFE, "Over layer life" },
{ DAVA::ParticleForce::eTimingType::OVER_PARTICLE_LIFE, "Over particle life" },
{ DAVA::ParticleForce::eTimingType::SECONDS_PARTICLE_LIFE, "Seconds particle life" }
} };

DAVA::Map<DAVA::ParticleForce::eType, QString> forceTypes =
{
  { DAVA::ParticleForce::eType::DRAG_FORCE, "Drag Force" },
  { DAVA::ParticleForce::eType::VORTEX, "Vortex" },
  { DAVA::ParticleForce::eType::GRAVITY, "Gravity" },
  { DAVA::ParticleForce::eType::WIND, "Wind" },
  { DAVA::ParticleForce::eType::POINT_GRAVITY, "Point Gravity" },
  { DAVA::ParticleForce::eType::PLANE_COLLISION, "Plane Collision" }
};

template <typename T, typename U, size_t sz>
int ElementToIndex(T elem, const DAVA::Array<U, sz> map)
{
    for (size_t i = 0; i < map.size(); ++i)
    {
        if (map[i].elemType == elem)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}
}

LayerForceWidget::LayerForceWidget(QWidget* parent /* = nullptr */)
    : BaseParticleEditorContentWidget(parent)
{
    mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    BuildCommonSection();
    BuildShapeSection();
    BuildTimingSection();
    BuilDirectionSection();
    BuildWindSection();
    BuildPointGravitySection();
    BuildPlaneCollisionSection();
    mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    blockSignals = false;
}

void LayerForceWidget::BuildTimingSection()
{
    using namespace LayerDragForceWidgetDetail;

    timingTypeSeparator = new QFrame();
    timingTypeSeparator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(timingTypeSeparator);

    timingLabel = new QLabel("Timing type:");
    mainLayout->addWidget(timingLabel);

    timingTypeComboBox = new WheellIgnorantComboBox();
    for (size_t i = 0; i < timingMap.size(); ++i)
        timingTypeComboBox->addItem(timingMap[i].name);

    connect(timingTypeComboBox, SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(OnValueChanged()));

    mainLayout->addWidget(timingTypeComboBox);

    forcePowerLabel = new QLabel("Force power:");
    mainLayout->addWidget(forcePowerLabel);

    forcePowerTimeLine = new TimeLineWidget(this);
    connect(forcePowerTimeLine, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(forcePowerTimeLine);

    forcePower = new ParticleVector3Widget("Force power", DAVA::Vector3::Zero);
    connect(forcePower, SIGNAL(valueChanged()), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(forcePower);

    forcePowerSpin = new EventFilterDoubleSpinBox();
    SetupSpin(forcePowerSpin);
    mainLayout->addWidget(forcePowerSpin);

    QHBoxLayout* startEndTimeLayout = new QHBoxLayout();
    startTimeLabel = new QLabel("Start time:");
    startTimeSpin = new EventFilterDoubleSpinBox();
    SetupSpin(startTimeSpin);
    startEndTimeLayout->addWidget(startTimeLabel);
    startEndTimeLayout->addWidget(startTimeSpin);
    endTimeLabel = new QLabel("End time:");
    endTimeSpin = new EventFilterDoubleSpinBox();
    SetupSpin(endTimeSpin);
    startEndTimeLayout->addWidget(endTimeLabel);
    startEndTimeLayout->addWidget(endTimeSpin);
    mainLayout->addLayout(startEndTimeLayout);

    randomizeReflectionForce = new QCheckBox("Randomize reflection force");
    connect(randomizeReflectionForce, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(randomizeReflectionForce);
    QHBoxLayout* rndForceLayout = new QHBoxLayout();
    rndReflectionForceMinLabel = new QLabel("Force random min multiplier:");
    rndReflectionForceMinSpin = new EventFilterDoubleSpinBox();
    SetupSpin(rndReflectionForceMinSpin);
    rndReflectionForceMaxLabel = new QLabel("Force random max multiplier:");
    rndReflectionForceMaxSpin = new EventFilterDoubleSpinBox();
    SetupSpin(rndReflectionForceMaxSpin);
    rndForceLayout->addWidget(rndReflectionForceMinLabel);
    rndForceLayout->addWidget(rndReflectionForceMinSpin);
    rndForceLayout->addWidget(rndReflectionForceMaxLabel);
    rndForceLayout->addWidget(rndReflectionForceMaxSpin);
    mainLayout->addLayout(rndForceLayout);

    QHBoxLayout* freqLayout = new QHBoxLayout();
    windFreqLabel = new QLabel("Wind frequency:");
    windFreqSpin = new EventFilterDoubleSpinBox();
    SetupSpin(windFreqSpin);
    freqLayout->addWidget(windFreqLabel);
    freqLayout->addWidget(windFreqSpin);
    mainLayout->addLayout(freqLayout);

    QHBoxLayout* biasLayout = new QHBoxLayout();
    windBiasLabel = new QLabel("Wind bias:");
    windBiasSpin = new EventFilterDoubleSpinBox();
    SetupSpin(windBiasSpin);
    biasLayout->addWidget(windBiasLabel);
    biasLayout->addWidget(windBiasSpin);
    mainLayout->addLayout(biasLayout);
}

void LayerForceWidget::BuildShapeSection()
{
    using namespace LayerDragForceWidgetDetail;

    shapeSeparator = new QFrame();
    shapeSeparator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(shapeSeparator);

    shapeLabel = new QLabel("Shape:");
    mainLayout->addWidget(shapeLabel);

    shapeComboBox = new WheellIgnorantComboBox();
    for (size_t i = 0; i < shapeMap.size(); ++i)
        shapeComboBox->addItem(shapeMap[i].name);
    connect(shapeComboBox, SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(OnValueChanged()));
    mainLayout->addWidget(shapeComboBox);

    boxSize = new ParticleVector3Widget("Box size", DAVA::Vector3::Zero);
    connect(boxSize, SIGNAL(valueChanged()), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(boxSize);

    radiusWidget = new QWidget();
    mainLayout->addWidget(radiusWidget);
    QHBoxLayout* layout = new QHBoxLayout(radiusWidget);
    QLabel* radiusLabel = new QLabel("Radius");

    radiusSpin = new EventFilterDoubleSpinBox();
    radiusSpin->setSingleStep(0.01);
    radiusSpin->setDecimals(3);
    connect(radiusSpin, SIGNAL(valueChanged(double)), this, SLOT(OnValueChanged()));
    radiusSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    layout->addWidget(radiusLabel);
    layout->addWidget(radiusSpin);
}

void LayerForceWidget::BuildCommonSection()
{
    forceTypeLabel = new QLabel("OLOLABEL");
    forceTypeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    forceTypeLabel->setContentsMargins(0, 15, 0, 15);
    forceTypeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(forceTypeLabel);

    isActive = new QCheckBox("Is active");
    connect(isActive, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(isActive);

    forceNameEdit = new QLineEdit();
    mainLayout->addWidget(forceNameEdit);
    connect(forceNameEdit, SIGNAL(editingFinished()), this, SLOT(OnValueChanged()));

    worldAlign = new QCheckBox("World align");
    connect(worldAlign, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(worldAlign);

    infinityRange = new QCheckBox("Use infinity range");
    connect(infinityRange, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(infinityRange);

    isGlobal = new QCheckBox("Is global force");
    connect(isGlobal, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(isGlobal);

    isGlobalWarning = new QLabel("NOTE: If you rotate particle effect with global force, force position should be (0, 0, 0)");
    mainLayout->addWidget(isGlobalWarning);
}

void LayerForceWidget::UpdateVisibility(DAVA::ParticleForce::eShape shape, DAVA::ParticleForce::eTimingType timingType, DAVA::ParticleForce::eType forceType, bool isInfinityRange, bool isGlobalForce)
{
    using Shape = DAVA::ParticleForce::eShape;
    using TimingType = DAVA::ParticleForce::eTimingType;
    using ForceType = DAVA::ParticleForce::eType;
    bool isGravity = forceType == ForceType::GRAVITY;
    bool isWind = forceType == ForceType::WIND;
    bool isDirectionalForce = forceType == ForceType::VORTEX || forceType == ForceType::WIND || forceType == ForceType::PLANE_COLLISION;
    bool isPointGravity = forceType == ForceType::POINT_GRAVITY;

    isGlobalWarning->setVisible(isGlobalForce);

    boxSize->setVisible(shape == Shape::BOX && !isInfinityRange && !isGravity);
    radiusWidget->setVisible(shape == Shape::SPHERE && !isInfinityRange && !isGravity);
    shapeComboBox->setVisible(!isInfinityRange && !isGravity);
    shapeLabel->setVisible(!isInfinityRange && !isGravity);
    shapeSeparator->setVisible(!isInfinityRange && !isGravity);

    forcePower->setVisible(timingType == TimingType::CONSTANT && !isGravity && !isWind);
    forcePowerTimeLine->setVisible(timingType != TimingType::CONSTANT);
    forcePowerLabel->setVisible(timingType != TimingType::CONSTANT || (timingType == TimingType::CONSTANT && isWind) || (timingType == TimingType::CONSTANT && isGravity));
    forcePowerSpin->setVisible(timingType == TimingType::CONSTANT && (isWind || isGravity));

    startTimeLabel->setVisible(timingType == TimingType::SECONDS_PARTICLE_LIFE);
    startTimeSpin->setVisible(timingType == TimingType::SECONDS_PARTICLE_LIFE);
    endTimeLabel->setVisible(timingType == TimingType::SECONDS_PARTICLE_LIFE);
    endTimeSpin->setVisible(timingType == TimingType::SECONDS_PARTICLE_LIFE);

    direction->setVisible(isDirectionalForce);
    directionSeparator->setVisible(isDirectionalForce);

    // Gravity
    infinityRange->setVisible(!isGravity);
    worldAlign->setVisible(!isGravity);

    // Wind
    windSeparator->setVisible(isWind);
    windFreqLabel->setVisible(isWind);
    windFreqSpin->setVisible(isWind);
    windTurbLabel->setVisible(timingType == TimingType::CONSTANT && isWind);
    windTurbSpin->setVisible(timingType == TimingType::CONSTANT && isWind);
    windTurbFreqLabel->setVisible(isWind);
    windTurbFreqSpin->setVisible(isWind);
    windBiasLabel->setVisible(isWind);
    windBiasSpin->setVisible(isWind);
    turbulenceTimeLine->setVisible(timingType != TimingType::CONSTANT && isWind);
    backTurbLabel->setVisible(isWind);
    backTurbSpin->setVisible(isWind);

    // PointGravity
    pointGravitySeparator->setVisible(isPointGravity);
    pointGravityRadiusLabel->setVisible(isPointGravity);
    pointGravityRadiusSpin->setVisible(isPointGravity);
    pointGravityUseRnd->setVisible(isPointGravity);
    killParticles->setVisible(isPointGravity);

    // Plane collision
    bool isPlaneCollision = forceType == ForceType::PLANE_COLLISION;
    planeCollisionSeparator->setVisible(isPlaneCollision);
    planeScaleLabel->setVisible(isPlaneCollision);
    planeScaleSpin->setVisible(isPlaneCollision);
    reflectionChaosLabel->setVisible(isPlaneCollision);
    reflectionChaosSpin->setVisible(isPlaneCollision);
    randomizeReflectionForce->setVisible(isPlaneCollision);
    rndReflectionForceMinLabel->setVisible(isPlaneCollision);
    rndReflectionForceMinSpin->setVisible(isPlaneCollision);
    rndReflectionForceMaxLabel->setVisible(isPlaneCollision);
    rndReflectionForceMaxSpin->setVisible(isPlaneCollision);
    reflectionPercentLabel->setVisible(isPlaneCollision);
    reflectionPercentSpin->setVisible(isPlaneCollision);
    normalAsReflectionVector->setVisible(isPlaneCollision);
    killParticlesAfterCollision->setVisible(isPlaneCollision);
    velocityThresholdLabel->setVisible(isPlaneCollision);
    velocityThresholdSpin->setVisible(isPlaneCollision);
}

void LayerForceWidget::SetupSpin(EventFilterDoubleSpinBox* spin, DAVA::float32 singleStep /*= 0.0001*/, DAVA::int32 decimals /*= 4*/)
{
    spin->setMinimum(-std::numeric_limits<DAVA::float32>::max());
    spin->setMaximum(std::numeric_limits<DAVA::float32>::max());
    spin->setSingleStep(singleStep);
    spin->setDecimals(decimals);
    connect(spin, SIGNAL(valueChanged(double)), this, SLOT(OnValueChanged()));
    spin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void LayerForceWidget::BuilDirectionSection()
{
    directionSeparator = new QFrame();
    directionSeparator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(directionSeparator);
    direction = new ParticleVector3Widget("Force direction", DAVA::Vector3::Zero);
    connect(direction, SIGNAL(valueChanged()), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(direction);
}

void LayerForceWidget::BuildWindSection()
{
    windSeparator = new QFrame();
    windSeparator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(windSeparator);

    QHBoxLayout* turbLayout = new QHBoxLayout();
    windTurbLabel = new QLabel("Wind turbulence:");
    windTurbSpin = new EventFilterDoubleSpinBox();
    SetupSpin(windTurbSpin);
    turbLayout->addWidget(windTurbLabel);
    turbLayout->addWidget(windTurbSpin);
    mainLayout->addLayout(turbLayout);

    turbulenceTimeLine = new TimeLineWidget(this);
    connect(turbulenceTimeLine, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(turbulenceTimeLine);

    QHBoxLayout* turbFreqLayout = new QHBoxLayout();
    windTurbFreqLabel = new QLabel("Wind turbulence frequency:");
    windTurbFreqSpin = new EventFilterDoubleSpinBox();
    SetupSpin(windTurbFreqSpin);
    turbFreqLayout->addWidget(windTurbFreqLabel);
    turbFreqLayout->addWidget(windTurbFreqSpin);
    mainLayout->addLayout(turbFreqLayout);

    QHBoxLayout* backTurbLayout = new QHBoxLayout();
    backTurbLabel = new QLabel("Backward turbulence probability:");
    backTurbSpin = new EventFilterDoubleSpinBox();
    SetupSpin(backTurbSpin, 1, 0);
    backTurbLayout->addWidget(backTurbLabel);
    backTurbLayout->addWidget(backTurbSpin);
    mainLayout->addLayout(backTurbLayout);
}

void LayerForceWidget::BuildPointGravitySection()
{
    pointGravitySeparator = new QFrame();
    pointGravitySeparator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(pointGravitySeparator);

    QHBoxLayout* pointGravityRadLayout = new QHBoxLayout();
    pointGravityRadiusLabel = new QLabel("Point gravity radius:");
    pointGravityRadiusSpin = new EventFilterDoubleSpinBox();
    SetupSpin(pointGravityRadiusSpin);
    pointGravityRadLayout->addWidget(pointGravityRadiusLabel);
    pointGravityRadLayout->addWidget(pointGravityRadiusSpin);
    mainLayout->addLayout(pointGravityRadLayout);
    pointGravityUseRnd = new QCheckBox("Random points on sphere");
    connect(pointGravityUseRnd, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(pointGravityUseRnd);
    killParticles = new QCheckBox("Kill particles");
    connect(killParticles, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(killParticles);
}

void LayerForceWidget::BuildPlaneCollisionSection()
{
    planeCollisionSeparator = new QFrame();
    planeCollisionSeparator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(planeCollisionSeparator);
    QHBoxLayout* planeScaleLayout = new QHBoxLayout();
    planeScaleLabel = new QLabel("Plane scale (editor visualization):");
    planeScaleSpin = new EventFilterDoubleSpinBox();
    SetupSpin(planeScaleSpin);
    planeScaleLayout->addWidget(planeScaleLabel);
    planeScaleLayout->addWidget(planeScaleSpin);
    mainLayout->addLayout(planeScaleLayout);

    normalAsReflectionVector = new QCheckBox("Use normal as reflection vector");
    connect(normalAsReflectionVector, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(normalAsReflectionVector);

    QHBoxLayout* reflectionChaosLayout = new QHBoxLayout();
    reflectionChaosLabel = new QLabel("Reflection chaos:");
    reflectionChaosSpin = new EventFilterDoubleSpinBox();
    SetupSpin(reflectionChaosSpin);
    reflectionChaosLayout->addWidget(reflectionChaosLabel);
    reflectionChaosLayout->addWidget(reflectionChaosSpin);
    mainLayout->addLayout(reflectionChaosLayout);

    QHBoxLayout* velocityThresholdLayout = new QHBoxLayout();
    velocityThresholdLabel = new QLabel("Velocity threshold:");
    velocityThresholdSpin = new EventFilterDoubleSpinBox();
    SetupSpin(velocityThresholdSpin);
    velocityThresholdLayout->addWidget(velocityThresholdLabel);
    velocityThresholdLayout->addWidget(velocityThresholdSpin);
    mainLayout->addLayout(velocityThresholdLayout);

    QHBoxLayout* reflectionPercentLayout = new QHBoxLayout();
    reflectionPercentLabel = new QLabel("Reflection percent:");
    reflectionPercentSpin = new EventFilterDoubleSpinBox();
    SetupSpin(reflectionPercentSpin, 1.0f, 0);
    reflectionPercentLayout->addWidget(reflectionPercentLabel);
    reflectionPercentLayout->addWidget(reflectionPercentSpin);
    mainLayout->addLayout(reflectionPercentLayout);

    killParticlesAfterCollision = new QCheckBox("Kill not reflected particles");
    connect(killParticlesAfterCollision, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));
    mainLayout->addWidget(killParticlesAfterCollision);
}

void LayerForceWidget::Init(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer_, DAVA::uint32 forceIndex_, bool updateMinimized)
{
    using namespace DAVA;
    using namespace LayerDragForceWidgetDetail;

    using LineWrapper = PropLineWrapper<Vector3>;
    using LineHelper = PropertyLineHelper;

    if (!layer_ || layer_->GetParticleForces().size() <= forceIndex_ || blockSignals)
        return;

    layer = layer_;
    forceIndex = forceIndex_;
    blockSignals = true;

    selectedForce = layer->GetParticleForces()[forceIndex];
    infinityRange->setChecked(selectedForce->isInfinityRange);
    worldAlign->setChecked(selectedForce->worldAlign);
    isActive->setChecked(selectedForce->isActive);
    boxSize->SetValue(selectedForce->GetBoxSize());
    forcePower->SetValue(selectedForce->forcePower);
    radiusSpin->setValue(selectedForce->GetRadius());
    forceNameEdit->setText(QString::fromStdString(selectedForce->forceName));
    forceTypeLabel->setText(forceTypes[selectedForce->type]);
    direction->SetValue(selectedForce->direction);
    windTurbSpin->setValue(selectedForce->windTurbulence);
    windTurbFreqSpin->setValue(selectedForce->windTurbulenceFrequency);
    windFreqSpin->setValue(selectedForce->windFrequency);
    windBiasSpin->setValue(selectedForce->windBias);
    backTurbSpin->setValue(selectedForce->backwardTurbulenceProbability);
    forcePowerSpin->setValue(selectedForce->forcePower.x);
    pointGravityRadiusSpin->setValue(selectedForce->pointGravityRadius);
    pointGravityUseRnd->setChecked(selectedForce->pointGravityUseRandomPointsOnSphere);
    isGlobal->setChecked(selectedForce->isGlobal);
    killParticles->setChecked(selectedForce->killParticles);
    killParticlesAfterCollision->setChecked(selectedForce->killParticles);
    normalAsReflectionVector->setChecked(selectedForce->normalAsReflectionVector);
    planeScaleSpin->setValue(selectedForce->planeScale);
    reflectionChaosSpin->setValue(selectedForce->reflectionChaos);
    randomizeReflectionForce->setChecked(selectedForce->randomizeReflectionForce);
    rndReflectionForceMinSpin->setValue(selectedForce->rndReflectionForceMin);
    rndReflectionForceMaxSpin->setValue(selectedForce->rndReflectionForceMax);
    velocityThresholdSpin->setValue(selectedForce->velocityThreshold);
    reflectionPercentSpin->setValue(selectedForce->reflectionPercent);
    startTimeSpin->setValue(selectedForce->startTime);
    endTimeSpin->setValue(selectedForce->endTime);

    UpdateVisibility(selectedForce->GetShape(), selectedForce->timingType, selectedForce->type, selectedForce->isInfinityRange, selectedForce->isGlobal);

    static const Vector<QColor> colors{ Qt::red, Qt::darkGreen, Qt::blue };
    static const Vector<QString> legends{ "Force x", "Force y", "Force z" };
    static const Vector<QString> windLegends{ "Wind force", "none", "none" };
    static const Vector<QString> gravityLegends{ "Gravity force", "none", "none" };
    const Vector<QString>* currLegends = nullptr;
    if (selectedForce->type == ParticleForce::eType::WIND)
        currLegends = &windLegends;
    else if (selectedForce->type == ParticleForce::eType::GRAVITY)
        currLegends = &gravityLegends;
    else
        currLegends = &legends;

    float32 start = 0.0f;
    float32 end = 1.0f;
    if (selectedForce->timingType == ParticleForce::eTimingType::SECONDS_PARTICLE_LIFE)
    {
        start = selectedForce->startTime;
        end = selectedForce->endTime;
    }

    forcePowerTimeLine->Init(start, end, updateMinimized, true, false);
    forcePowerTimeLine->AddLines(LineWrapper(LineHelper::GetValueLine(selectedForce->forcePowerLine)).GetProps(), colors, *currLegends);

    turbulenceTimeLine->Init(start, end, updateMinimized, true, false);
    turbulenceTimeLine->AddLine(0, PropLineWrapper<float32>(LineHelper::GetValueLine(selectedForce->turbulenceLine)).GetProps(), Qt::red, "Turbulence");

    shapeComboBox->setCurrentIndex(ElementToIndex(selectedForce->GetShape(), shapeMap));
    timingTypeComboBox->setCurrentIndex(ElementToIndex(selectedForce->timingType, timingMap));

    blockSignals = false;
}

void LayerForceWidget::Update()
{
    Init(GetActiveScene(), layer, forceIndex, false);
}

void LayerForceWidget::StoreVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;

    DAVA::KeyedArchive* props = new DAVA::KeyedArchive();
    forcePowerTimeLine->GetVisualState(props);
    visualStateProps->SetArchive("FORCE_PROPS", props);

    props->DeleteAllKeys();
    turbulenceTimeLine->SetVisualState(props);
    visualStateProps->SetArchive("TURB_PROPS", props);
    DAVA::SafeRelease(props);
}

void LayerForceWidget::RestoreVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;
    forcePowerTimeLine->SetVisualState(visualStateProps->GetArchive("FORCE_PROPS"));
    turbulenceTimeLine->SetVisualState(visualStateProps->GetArchive("TURB_PROPS"));
}

void LayerForceWidget::OnValueChanged()
{
    using namespace DAVA;
    using namespace LayerDragForceWidgetDetail;
    using Shape = ParticleForce::eShape;
    using TimingType = ParticleForce::eTimingType;
    using ForceType = ParticleForce::eType;

    if (blockSignals)
        return;

    Shape shape = shapeMap[shapeComboBox->currentIndex()].elemType;
    TimingType timingType = timingMap[timingTypeComboBox->currentIndex()].elemType;

    PropLineWrapper<Vector3> propForce;
    forcePowerTimeLine->GetValues(propForce.GetPropsPtr());

    PropLineWrapper<float32> propTurb;
    turbulenceTimeLine->GetValue(0, propTurb.GetPropsPtr());

    CommandUpdateParticleForce::ForceParams params;
    params.isActive = isActive->isChecked();
    params.forceName = forceNameEdit->text().toStdString();
    params.shape = shape;
    params.timingType = timingType;
    params.boxSize = boxSize->GetValue();
    if (selectedForce->type == ForceType::WIND || selectedForce->type == ForceType::GRAVITY)
        params.forcePower = Vector3(forcePowerSpin->value(), 0.0f, 0.0f);
    else
        params.forcePower = forcePower->GetValue();

    params.direction = direction->GetValue();
    params.useInfinityRange = infinityRange->isChecked();
    params.worldAlign = selectedForce->type == ForceType::GRAVITY ? true : worldAlign->isChecked();
    params.radius = radiusSpin->value();
    params.forcePowerLine = propForce.GetPropLine();
    params.turbulenceLine = propTurb.GetPropLine();
    params.windFrequency = windFreqSpin->value();
    params.windTurbulence = windTurbSpin->value();
    params.windTurbulenceFrequency = windTurbFreqSpin->value();
    params.windBias = windBiasSpin->value();
    params.backwardTurbulenceProbability = static_cast<uint32>(Clamp(backTurbSpin->value(), 0.0, 100.0));
    params.pointGravityRadius = pointGravityRadiusSpin->value();
    params.pointGravityUseRandomPointsOnSphere = pointGravityUseRnd->isChecked();
    params.randomizeReflectionForce = randomizeReflectionForce->isChecked();
    params.reflectionPercent = static_cast<uint32>(Clamp(reflectionPercentSpin->value(), 0.0, 100.0));

    params.isGlobal = isGlobal->isChecked();
    if (selectedForce->type != ForceType::PLANE_COLLISION)
        params.killParticles = killParticles->isChecked();
    else
        params.killParticles = killParticlesAfterCollision->isChecked();
    params.planeScale = planeScaleSpin->value();
    params.reflectionChaos = Clamp(static_cast<float32>(reflectionChaosSpin->value()), 0.0f, 360.0f);
    params.normalAsReflectionVector = normalAsReflectionVector->isChecked();

    float32 rndReflForceMinMult = rndReflectionForceMinSpin->value();
    float32 rndReflForceMaxMult = rndReflectionForceMaxSpin->value();
    rndReflForceMinMult = Clamp(rndReflForceMinMult, 0.0f, 99.9f);
    rndReflForceMaxMult = Max(rndReflForceMaxMult, rndReflForceMinMult + 0.1f);
    params.rndReflectionForceMin = rndReflForceMinMult;
    params.rndReflectionForceMax = rndReflForceMaxMult;
    rndReflectionForceMinSpin->setValue(rndReflForceMinMult);
    rndReflectionForceMaxSpin->setValue(rndReflForceMaxMult);

    float32 startTimeVal = startTimeSpin->value();
    float32 endTimeVal = endTimeSpin->value();
    startTimeVal = Max(startTimeVal, 0.0f);
    endTimeVal = Max(startTimeVal + 0.0001f, endTimeVal);
    params.startTime = startTimeVal;
    params.endTime = endTimeVal;

    reflectionPercentSpin->setValue(params.reflectionPercent);

    backTurbSpin->setValue(params.backwardTurbulenceProbability);
    reflectionChaosSpin->setValue(params.reflectionChaos);

    params.velocityThreshold = velocityThresholdSpin->value();

    UpdateVisibility(shape, timingType, selectedForce->type, params.useInfinityRange, params.isGlobal);

    shapeComboBox->setCurrentIndex(ElementToIndex(shape, shapeMap));
    timingTypeComboBox->setCurrentIndex(ElementToIndex(timingType, timingMap));

    std::unique_ptr<CommandUpdateParticleForce> updateDragForceCmd(new CommandUpdateParticleForce(layer, forceIndex, std::move(params)));

    SceneEditor2* activeScene = GetActiveScene();
    DVASSERT(activeScene != nullptr);
    activeScene->Exec(std::move(updateDragForceCmd));
    activeScene->MarkAsChanged();

    float32 startTime = 0.0f;
    float32 endTime = 1.0f;
    if (timingType == TimingType::SECONDS_PARTICLE_LIFE)
    {
        startTime = startTimeVal;
        endTime = endTimeVal;
    }

    UpdateKeys(DAVA::PropertyLineHelper::GetValueLine(selectedForce->forcePowerLine).Get(), startTime, endTime);
    UpdateKeys(DAVA::PropertyLineHelper::GetValueLine(selectedForce->turbulenceLine).Get(), startTime, endTime);

    Init(GetActiveScene(), layer, forceIndex, false);
    emit ValueChanged();
}
