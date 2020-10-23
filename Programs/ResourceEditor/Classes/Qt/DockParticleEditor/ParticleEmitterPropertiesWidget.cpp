#include "ParticleEmitterPropertiesWidget.h"

#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/Commands/EntityAddCommand.h>
#include <REPlatform/Commands/EntityRemoveCommand.h>
#include <REPlatform/Commands/ParticleEditorCommands.h>
#include <REPlatform/Commands/RECommandBatch.h>
#include <REPlatform/Commands/TransformCommand.h>
#include <REPlatform/DataNodes/Selectable.h>

#include <QLineEdit>
#include <QEvent>

#define EMISSION_RANGE_MIN_LIMIT_DEGREES 0.0f
#define EMISSION_RANGE_MAX_LIMIT_DEGREES 360.0f

ParticleEmitterPropertiesWidget::ParticleEmitterPropertiesWidget(QWidget* parent)
    : BaseParticleEditorContentWidget(parent)
{
    mainLayout = new QVBoxLayout();
    this->setLayout(mainLayout);

    emitterNameLineEdit = new QLineEdit();
    mainLayout->addWidget(emitterNameLineEdit);
    connect(emitterNameLineEdit, SIGNAL(editingFinished()), this, SLOT(OnValueChanged()));

    {
        QWidget* group = new QWidget(this);
        QHBoxLayout* layout = new QHBoxLayout(group);
        group->setLayout(layout);
        QLabel* label = new QLabel("Original path: ", this);

        originalEmitterYamlPath = new QLineEdit(this);
        originalEmitterYamlPath->setReadOnly(true);
        layout->addWidget(label);
        layout->addWidget(originalEmitterYamlPath);
        layout->setMargin(0);
        mainLayout->addWidget(group);
    }

    {
        QWidget* group = new QWidget(this);
        QHBoxLayout* layout = new QHBoxLayout(group);
        group->setLayout(layout);
        QLabel* label = new QLabel("Current path: ", this);

        emitterYamlPath = new QLineEdit(this);
        emitterYamlPath->setReadOnly(true);
        layout->addWidget(label);
        layout->addWidget(emitterYamlPath);
        layout->setMargin(0);
        mainLayout->addWidget(group);
        connect(emitterYamlPath, SIGNAL(textChanged(const QString&)), this, SLOT(OnEmitterYamlPathChanged(const QString&)));
    }

    shortEffectCheckBox = new QCheckBox("Short effect");
    mainLayout->addWidget(shortEffectCheckBox);
    connect(shortEffectCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));

    generateOnSurfaceCheckBox = new QCheckBox("Generate on surface");
    mainLayout->addWidget(generateOnSurfaceCheckBox);
    connect(generateOnSurfaceCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnValueChanged()));

    QHBoxLayout* emitterTypeHBox = new QHBoxLayout();
    emitterTypeHBox->addWidget(new QLabel("type"));
    emitterType = new QComboBox(this);
    emitterType->addItem("Point");
    emitterType->addItem("Box");
    emitterType->addItem("Circle - Volume");
    emitterType->addItem("Circle - Edges");
    emitterType->addItem("Sphere");
    emitterTypeHBox->addWidget(emitterType);
    mainLayout->addLayout(emitterTypeHBox);
    connect(emitterType, SIGNAL(currentIndexChanged(int)), this, SLOT(OnValueChanged()));

    QHBoxLayout* shockwaveModeHBox = new QHBoxLayout();
    shockwaveLabel = new QLabel("shockwave mode");
    shockwaveModeHBox->addWidget(shockwaveLabel);
    shockwaveBox = new QComboBox(this);
    shockwaveBox->addItem("Disabled");
    shockwaveBox->addItem("Shockwave");
    shockwaveBox->addItem("Horizontal Shockwave");
    shockwaveModeHBox->addWidget(shockwaveBox);
    mainLayout->addLayout(shockwaveModeHBox);
    connect(shockwaveBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnValueChanged()));

    QHBoxLayout* positionLayout = new QHBoxLayout();

    positionLayout->addWidget(new QLabel("Position"));
    positionLayout->addStretch();
    positionLayout->addWidget(new QLabel("X:"));
    positionXSpinBox = new EventFilterDoubleSpinBox();
    positionXSpinBox->setMinimum(-100);
    positionXSpinBox->setMaximum(100);
    positionXSpinBox->setSingleStep(0.1);
    positionXSpinBox->setDecimals(3);
    positionLayout->addWidget(positionXSpinBox);
    connect(positionXSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnEmitterPositionChanged()));

    positionLayout->addStretch();
    positionLayout->addWidget(new QLabel("Y:"));
    positionYSpinBox = new EventFilterDoubleSpinBox();
    positionYSpinBox->setMinimum(-100);
    positionYSpinBox->setMaximum(100);
    positionYSpinBox->setSingleStep(0.1);
    positionYSpinBox->setDecimals(3);
    positionLayout->addWidget(positionYSpinBox);
    connect(positionYSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnEmitterPositionChanged()));

    positionLayout->addStretch();
    positionLayout->addWidget(new QLabel("Z:"));
    positionZSpinBox = new EventFilterDoubleSpinBox();
    positionZSpinBox->setMinimum(-100);
    positionZSpinBox->setMaximum(100);
    positionZSpinBox->setSingleStep(0.1);
    positionZSpinBox->setDecimals(3);
    positionLayout->addWidget(positionZSpinBox);
    connect(positionZSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnEmitterPositionChanged()));

    mainLayout->addLayout(positionLayout);

    emitterEmissionRange = new TimeLineWidget(this);
    InitWidget(emitterEmissionRange);

    emitterEmissionVector = new TimeLineWidget(this);
    InitWidget(emitterEmissionVector);

    emissionVelocityVector = new TimeLineWidget(this);
    InitWidget(emissionVelocityVector);

    emitterRadius = new TimeLineWidget(this);
    InitWidget(emitterRadius);

    emitterInnerRadius = new TimeLineWidget(this);
    InitWidget(emitterInnerRadius);

    emitterAngle = new TimeLineWidget(this);
    InitWidget(emitterAngle);

    emitterColorWidget = new GradientPickerWidget(this);
    InitWidget(emitterColorWidget);

    emitterSize = new TimeLineWidget(this);
    InitWidget(emitterSize);

    QHBoxLayout* emitterLifeHBox = new QHBoxLayout();
    emitterLifeHBox->addWidget(new QLabel("life"));
    emitterLife = new EventFilterDoubleSpinBox(this);
    emitterLife->setMinimum(0.f);
    emitterLife->setMaximum(10000000);
    emitterLifeHBox->addWidget(emitterLife);
    mainLayout->addLayout(emitterLifeHBox);
    connect(emitterLife, SIGNAL(valueChanged(double)), this, SLOT(OnValueChanged()));

    Q_FOREACH (QAbstractSpinBox* sp, findChildren<QAbstractSpinBox*>())
    {
        sp->installEventFilter(this);
    }
    emitterYamlPath->installEventFilter(this);

    connect(SceneSignals::Instance(), &SceneSignals::CommandExecuted, this, &ParticleEmitterPropertiesWidget::OnCommand);

    blockSignals = false;
}

void ParticleEmitterPropertiesWidget::InitWidget(QWidget* widget, bool connectWidget)
{
    mainLayout->addWidget(widget);
    if (connectWidget)
        connect(widget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
}

void ParticleEmitterPropertiesWidget::OnEmitterPositionChanged()
{
    if (blockSignals)
        return;

    DAVA::SceneEditor2* activeScene = GetActiveScene();
    DVASSERT(GetEffect(activeScene) != nullptr);
    DVASSERT(GetEmitterInstance(activeScene) != nullptr);

    DAVA::Vector3 position;
    position.x = positionXSpinBox->value();
    position.y = positionYSpinBox->value();
    position.z = positionZSpinBox->value();
    auto newTransform = DAVA::Matrix4::MakeTranslation(position);

    DAVA::Selectable wrapper(GetEmitterInstance(activeScene));
    GetActiveScene()->Exec(std::unique_ptr<DAVA::Command>(new DAVA::TransformCommand(wrapper, wrapper.GetLocalTransform(), newTransform)));

    Init(GetActiveScene(), GetEffect(activeScene), GetEmitterInstance(activeScene), false, false);
    emit ValueChanged();
}

namespace ParticleEmitterPropertiesWidgetDetail
{
bool HasInstance(const DAVA::Entity* entity, DAVA::ParticleEmitterInstance* instance)
{
    DAVA::ParticleEffectComponent* effect = entity->GetComponent<DAVA::ParticleEffectComponent>();
    if (effect != nullptr)
    {
        for (DAVA::uint32 i = 0, e = effect->GetEmittersCount(); i < e; ++i)
        {
            if (effect->GetEmitterInstance(i) == instance)
            {
                return true;
            }
        }
    }

    DAVA::uint32 count = entity->GetChildrenCount();
    for (DAVA::uint32 c = 0; c < count; ++c)
    {
        if (HasInstance(entity->GetChild(c), instance))
        {
            return true;
        }
    }

    return false;
}
}

void ParticleEmitterPropertiesWidget::OnCommand(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
{
    if (blockSignals || (GetActiveScene() != scene))
        return;

    commandNotification.ForEach<DAVA::CommandRemoveParticleEmitter>([&](const DAVA::CommandRemoveParticleEmitter* cmd) {
        if (cmd->GetEmitterInstance() == GetEmitterInstance(scene))
        {
            SetObjectsForScene(scene, nullptr, nullptr);
        }
    });

    commandNotification.ForEach<DAVA::EntityAddCommand>([&](const DAVA::EntityAddCommand* cmd) {
        const DAVA::Entity* entity = cmd->GetEntity();
        if (entity != nullptr && ParticleEmitterPropertiesWidgetDetail::HasInstance(entity, GetEmitterInstance(scene)))
        {
            SetObjectsForScene(scene, nullptr, nullptr);
        }
    });

    commandNotification.ForEach<DAVA::EntityAddCommand>([&](const DAVA::EntityAddCommand* cmd) {
        const DAVA::Entity* entity = cmd->GetEntity();
        if (entity != nullptr && ParticleEmitterPropertiesWidgetDetail::HasInstance(entity, GetEmitterInstance(scene)))
        {
            SetObjectsForScene(scene, nullptr, nullptr);
        }
    });

    if ((GetEmitterInstance(scene) != nullptr) && (GetEffect(scene) != nullptr))
    {
        UpdateProperties();
    }
}

void ParticleEmitterPropertiesWidget::OnValueChanged()
{
    if (blockSignals)
        return;

    DAVA::SceneEditor2* activeScene = GetActiveScene();
    DVASSERT(activeScene != 0);

    DAVA::ParticleEmitterInstance* instance = GetEmitterInstance(activeScene);
    DVASSERT(instance != nullptr);

    DVASSERT(emitterType->currentIndex() != -1);
    DAVA::ParticleEmitter::eType type = static_cast<DAVA::ParticleEmitter::eType>(emitterType->currentIndex());

    DAVA::PropLineWrapper<DAVA::float32> emissionRange;
    if (!emitterEmissionRange->GetValue(0, emissionRange.GetPropsPtr()))
        return;

    DAVA::PropLineWrapper<DAVA::Vector3> emissionVector;
    if (!emitterEmissionVector->GetValues(emissionVector.GetPropsPtr()))
        return;

    DAVA::PropLineWrapper<DAVA::Vector3> emissionVelocityVectorProps;
    if (!emissionVelocityVector->GetValues(emissionVelocityVectorProps.GetPropsPtr()))
        return;

    DAVA::PropLineWrapper<DAVA::float32> radius;
    if (!emitterRadius->GetValue(0, radius.GetPropsPtr()))
        return;

    DAVA::PropLineWrapper<DAVA::float32> innerRadius;
    if (!emitterInnerRadius->GetValue(0, innerRadius.GetPropsPtr()))
        return;

    DAVA::PropLineWrapper<DAVA::Color> colorOverLife;
    if (!emitterColorWidget->GetValues(colorOverLife.GetPropsPtr()))
        return;

    DAVA::PropLineWrapper<DAVA::Vector3> size;
    if (!emitterSize->GetValues(size.GetPropsPtr()))
        return;

    DAVA::float32 life = emitterLife->value();
    DAVA::float32 currentLifeTime = instance->GetEmitter()->lifeTime;
    bool initEmittersByDef = FLOAT_EQUAL(life, currentLifeTime) ? false : true;

    bool isShortEffect = shortEffectCheckBox->isChecked();
    bool generateOnSurface = generateOnSurfaceCheckBox->isChecked();
    DAVA::ParticleEmitter::eShockwaveMode shockwave = static_cast<DAVA::ParticleEmitter::eShockwaveMode>(shockwaveBox->currentIndex());

    DAVA::PropLineWrapper<DAVA::float32> propAngle;
    DAVA::PropLineWrapper<DAVA::float32> propAngleVariation;
    emitterAngle->GetValue(0, propAngle.GetPropsPtr());
    emitterAngle->GetValue(1, propAngleVariation.GetPropsPtr());

    std::unique_ptr<DAVA::CommandUpdateEmitter> commandUpdateEmitter(new DAVA::CommandUpdateEmitter(instance));
    commandUpdateEmitter->Init(DAVA::FastName(emitterNameLineEdit->text().toStdString().c_str()),
                               type,
                               emissionRange.GetPropLine(),
                               emissionVector.GetPropLine(),
                               emissionVelocityVectorProps.GetPropLine(),
                               radius.GetPropLine(),
                               innerRadius.GetPropLine(),
                               propAngle.GetPropLine(),
                               propAngleVariation.GetPropLine(),
                               colorOverLife.GetPropLine(),
                               size.GetPropLine(),
                               life,
                               isShortEffect,
                               generateOnSurface,
                               shockwave);

    activeScene->Exec(std::move(commandUpdateEmitter));
    activeScene->MarkAsChanged();

    Init(activeScene, GetEffect(activeScene), instance, false, initEmittersByDef);
    emit ValueChanged();
}

void ParticleEmitterPropertiesWidget::Init(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect_, DAVA::ParticleEmitterInstance* instance_,
                                           bool updateMinimize_, bool needUpdateTimeLimits_)
{
    DVASSERT(instance_ != nullptr);

    updateMinimize = updateMinimize_;
    needUpdateTimeLimits = needUpdateTimeLimits_;
    SetObjectsForScene(scene, effect_, instance_);

    blockSignals = true;
    UpdateProperties();
}

void ParticleEmitterPropertiesWidget::UpdateProperties()
{
    DAVA::SceneEditor2* activeScene = GetActiveScene();
    DVASSERT(activeScene != 0);

    DAVA::ParticleEffectComponent* effect = GetEffect(activeScene);
    DVASSERT(effect != nullptr);
    DAVA::ParticleEmitter* emitter = GetEmitterInstance(activeScene)->GetEmitter();
    DVASSERT(emitter != nullptr);

    emitterNameLineEdit->setText(QString::fromStdString(emitter->name.c_str()));
    shortEffectCheckBox->setChecked(emitter->shortEffect);
    generateOnSurfaceCheckBox->setChecked(emitter->generateOnSurface);
    shockwaveBox->setCurrentIndex(emitter->shockwaveMode);

    DAVA::float32 emitterLifeTime = emitter->lifeTime;
    DAVA::float32 minTime = 0.f;
    DAVA::float32 minTimeLimit = 0.f;
    DAVA::float32 maxTime = emitterLifeTime;
    DAVA::float32 maxTimeLimit = emitterLifeTime;

    QString originalYamlPath;
    if (GetEmitterInstance(activeScene)->GetOwner() == nullptr)
    {
        originalYamlPath = QString::fromStdString(emitter->configPath.GetAbsolutePathname());
    }
    else
    {
        DAVA::int32 emitterId = effect->GetEmitterInstanceIndex(GetEmitterInstance(activeScene));
        if (emitterId != -1)
        {
            originalYamlPath = QString::fromStdString(effect->GetEmitterInstance(emitterId)->GetFilePath().GetAbsolutePathname());
        }
    }

    originalEmitterYamlPath->setText(originalYamlPath);

    emitterYamlPath->setText(QString::fromStdString(emitter->configPath.GetAbsolutePathname()));
    emitterType->setCurrentIndex(emitter->emitterType);

    DAVA::int32 emitterId = effect->GetEmitterInstanceIndex(GetEmitterInstance(activeScene));
    DAVA::Vector3 position = (emitterId == -1) ? DAVA::Vector3(0, 0, 0) : effect->GetSpawnPosition(emitterId);

    {
        QSignalBlocker lockX(positionXSpinBox);
        QSignalBlocker lockY(positionYSpinBox);
        QSignalBlocker lockZ(positionZSpinBox);
        positionXSpinBox->setValue(position.x);
        positionYSpinBox->setValue(position.y);
        positionZSpinBox->setValue(position.z);
    }

    if (!needUpdateTimeLimits)
    {
        minTime = emitterEmissionRange->GetMinBoundary();
        maxTime = emitterEmissionRange->GetMaxBoundary();
    }
    emitterEmissionRange->Init(minTime, maxTime, minTimeLimit, maxTimeLimit, updateMinimize);
    emitterEmissionRange->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(emitter->emissionRange)).GetProps(), Qt::blue, "emission range");
    emitterEmissionRange->SetMinLimits(EMISSION_RANGE_MIN_LIMIT_DEGREES);
    emitterEmissionRange->SetMaxLimits(EMISSION_RANGE_MAX_LIMIT_DEGREES);
    emitterEmissionRange->SetYLegendMark(DEGREE_MARK_CHARACTER);

    if (!needUpdateTimeLimits)
    {
        minTime = emitterEmissionVector->GetMinBoundary();
        maxTime = emitterEmissionVector->GetMaxBoundary();
    }
    emitterEmissionVector->Init(minTime, maxTime, minTimeLimit, maxTimeLimit, updateMinimize, true);
    DAVA::Vector<QColor> vectorColors;
    vectorColors.push_back(Qt::red);
    vectorColors.push_back(Qt::darkGreen);
    vectorColors.push_back(Qt::blue);
    DAVA::Vector<QString> vectorLegends;
    vectorLegends.push_back("emission vector: x");
    vectorLegends.push_back("emission vector: y");
    vectorLegends.push_back("emission vector: z");
    emitterEmissionVector->AddLines(DAVA::PropLineWrapper<DAVA::Vector3>(DAVA::PropertyLineHelper::GetValueLine(emitter->emissionVector)).GetProps(), vectorColors, vectorLegends);

    if (!needUpdateTimeLimits)
    {
        minTime = emissionVelocityVector->GetMinBoundary();
        maxTime = emissionVelocityVector->GetMaxBoundary();
    }
    emissionVelocityVector->Init(minTime, maxTime, minTimeLimit, maxTimeLimit, updateMinimize, true);

    vectorLegends.clear();
    vectorLegends.emplace_back("emission velocity vector: x");
    vectorLegends.emplace_back("emission velocity vector: y");
    vectorLegends.emplace_back("emission velocity vector: z");
    emissionVelocityVector->AddLines(DAVA::PropLineWrapper<DAVA::Vector3>(DAVA::PropertyLineHelper::GetValueLine(emitter->emissionVelocityVector)).GetProps(), vectorColors, vectorLegends);

    if (!needUpdateTimeLimits)
    {
        minTime = emitterRadius->GetMinBoundary();
        maxTime = emitterRadius->GetMaxBoundary();
    }
    emitterRadius->Init(minTime, maxTime, minTimeLimit, maxTimeLimit, updateMinimize);
    emitterRadius->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(emitter->radius)).GetProps(), Qt::blue, "radius");
    // Radius cannot be negative.
    emitterRadius->SetMinLimits(0.0f);

    if (!needUpdateTimeLimits)
    {
        minTime = emitterInnerRadius->GetMinBoundary();
        maxTime = emitterInnerRadius->GetMaxBoundary();
    }
    emitterInnerRadius->Init(minTime, maxTime, minTimeLimit, maxTimeLimit, updateMinimize);
    emitterInnerRadius->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(emitter->innerRadius)).GetProps(), Qt::blue, "inner radius");
    emitterInnerRadius->SetMinLimits(0.0f);

    if (!needUpdateTimeLimits)
    {
        minTime = emitterAngle->GetMinBoundary();
        maxTime = emitterAngle->GetMaxBoundary();
    }
    emitterAngle->Init(minTime, maxTime, minTimeLimit, maxTimeLimit, updateMinimize);
    emitterAngle->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(emitter->emissionAngle)).GetProps(), Qt::blue, "emission angle base");
    emitterAngle->AddLine(1, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(emitter->emissionAngleVariation)).GetProps(), Qt::green, "emission angle spread");
    emitterAngle->SetYLegendMark(DEGREE_MARK_CHARACTER);

    emitterColorWidget->Init(0.f, emitterLifeTime, "color over life");
    emitterColorWidget->SetValues(DAVA::PropLineWrapper<DAVA::Color>(DAVA::PropertyLineHelper::GetValueLine(emitter->colorOverLife)).GetProps());

    if (!needUpdateTimeLimits)
    {
        minTime = emitterSize->GetMinBoundary();
        maxTime = emitterSize->GetMaxBoundary();
    }
    emitterSize->Init(minTime, maxTime, minTimeLimit, maxTimeLimit, updateMinimize, true);
    emitterSize->SetMinLimits(0);
    DAVA::Vector<QColor> sizeColors;
    sizeColors.push_back(Qt::red);
    sizeColors.push_back(Qt::darkGreen);
    sizeColors.push_back(Qt::blue);
    DAVA::Vector<QString> sizeLegends;
    sizeLegends.push_back("size: x");
    sizeLegends.push_back("size: y");
    sizeLegends.push_back("size: z");
    emitterSize->AddLines(DAVA::PropLineWrapper<DAVA::Vector3>(DAVA::PropertyLineHelper::GetValueLine(emitter->size)).GetProps(), sizeColors, sizeLegends);
    emitterSize->EnableLock(true);

    emitterLife->setValue(emitterLifeTime);

    blockSignals = false;
}

void ParticleEmitterPropertiesWidget::OnEmitterYamlPathChanged(const QString& newPath)
{
    UpdateTooltip();
}

void ParticleEmitterPropertiesWidget::RestoreVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;

    emitterEmissionRange->SetVisualState(visualStateProps->GetArchive("EMITTER_EMISSION_RANGE_PROPS"));
    emitterEmissionVector->SetVisualState(visualStateProps->GetArchive("EMITTER_EMISSION_VECTOR_PROPS"));
    emissionVelocityVector->SetVisualState(visualStateProps->GetArchive("EMITTER_EMISSION_VEL_VECTOR_PROPS"));
    emitterRadius->SetVisualState(visualStateProps->GetArchive("EMITTER_RADIUS_PROPS"));
    emitterInnerRadius->SetVisualState(visualStateProps->GetArchive("EMITTER_INNER_RADIUS_PROPS"));
    emitterAngle->SetVisualState(visualStateProps->GetArchive("EMITTER_ANGLE_PROPS"));
    emitterSize->SetVisualState(visualStateProps->GetArchive("EMITTER_SIZE_PROPS"));
}

void ParticleEmitterPropertiesWidget::StoreVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;

    DAVA::KeyedArchive* props = new DAVA::KeyedArchive();

    props->DeleteAllKeys();
    emitterEmissionRange->GetVisualState(props);
    visualStateProps->SetArchive("EMITTER_EMISSION_RANGE_PROPS", props);

    props->DeleteAllKeys();
    emitterEmissionVector->GetVisualState(props);
    visualStateProps->SetArchive("EMITTER_EMISSION_VECTOR_PROPS", props);

    props->DeleteAllKeys();
    emissionVelocityVector->GetVisualState(props);
    visualStateProps->SetArchive("EMITTER_EMISSION_VEL_VECTOR_PROPS", props);

    props->DeleteAllKeys();
    emitterRadius->GetVisualState(props);
    visualStateProps->SetArchive("EMITTER_RADIUS_PROPS", props);

    props->DeleteAllKeys();
    emitterInnerRadius->GetVisualState(props);
    visualStateProps->SetArchive("EMITTER_INNER_RADIUS_PROPS", props);

    props->DeleteAllKeys();
    emitterAngle->GetVisualState(props);
    visualStateProps->SetArchive("EMITTER_ANGLE_PROPS", props);

    props->DeleteAllKeys();
    emitterSize->GetVisualState(props);
    visualStateProps->SetArchive("EMITTER_SIZE_PROPS", props);

    SafeRelease(props);
}

void ParticleEmitterPropertiesWidget::Update()
{
    DAVA::SceneEditor2* activeScene = GetActiveScene();
    Init(activeScene, GetEffect(activeScene), GetEmitterInstance(activeScene), false);
}

bool ParticleEmitterPropertiesWidget::eventFilter(QObject* o, QEvent* e)
{
    if (e->type() == QEvent::Wheel && qobject_cast<QAbstractSpinBox*>(o))
    {
        e->ignore();
        return true;
    }

    if (e->type() == QEvent::Resize && qobject_cast<QLineEdit*>(o))
    {
        UpdateTooltip();
        return true;
    }

    return QWidget::eventFilter(o, e);
}

void ParticleEmitterPropertiesWidget::UpdateTooltip()
{
    QFontMetrics fm = emitterYamlPath->fontMetrics();
    if (fm.width(emitterYamlPath->text()) >= emitterYamlPath->width())
    {
        emitterYamlPath->setToolTip(emitterYamlPath->text());
    }
    else
    {
        emitterYamlPath->setToolTip("");
    }
}