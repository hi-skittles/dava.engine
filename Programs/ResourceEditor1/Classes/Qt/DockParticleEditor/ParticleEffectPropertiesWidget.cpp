#include "ParticleEffectPropertiesWidget.h"
#include "Classes/Commands2/ParticleEditorCommands.h"

#include <TArc/Utils/Utils.h>

#include <Scene3D/Systems/ParticleEffectSystem.h>

#include <QLineEdit>
#include <QEvent>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>

static const int TreeItemTypeEmitter = QTreeWidgetItem::UserType + 1;
static const int TreeItemTypeLayer = QTreeWidgetItem::UserType + 2;
static const int TreeItemTypeForce = QTreeWidgetItem::UserType + 3;
static const int TreeItemTypeExternal = QTreeWidgetItem::UserType + 4;

ParticleEffectPropertiesWidget::ParticleEffectPropertiesWidget(QWidget* parent)
    : BaseParticleEditorContentWidget(parent)
{
    using namespace DAVA::TArc;
    mainLayout = new QVBoxLayout();
    mainLayout->setAlignment(Qt::AlignTop);
    this->setLayout(mainLayout);

    effectPlaybackSpeedLabel = new QLabel("effect playback speed");
    mainLayout->addWidget(effectPlaybackSpeedLabel);

    effectPlaybackSpeed = new QSlider(Qt::Horizontal, this);
    effectPlaybackSpeed->setTracking(true);
    effectPlaybackSpeed->setRange(0, 4); // 25%, 50%, 100%, 200%, 400% - 5 values total.
    effectPlaybackSpeed->setTickPosition(QSlider::TicksBelow);
    effectPlaybackSpeed->setTickInterval(1);
    effectPlaybackSpeed->setSingleStep(1);
    mainLayout->addWidget(effectPlaybackSpeed);

    QHBoxLayout* playerBox = new QHBoxLayout();
    playBtn = new QPushButton(SharedIcon(":/QtIcons/play.png"), "");
    playBtn->setToolTip("Play");
    playerBox->addWidget(playBtn);
    stopBtn = new QPushButton(SharedIcon(":/QtIcons/stop.png"), "");
    stopBtn->setToolTip("Stop");
    playerBox->addWidget(stopBtn);
    stopAndDeleteBtn = new QPushButton(SharedIcon(":/QtIcons/stop_clear.png"), "");
    stopAndDeleteBtn->setToolTip("Stop and delete particles");
    playerBox->addWidget(stopAndDeleteBtn);
    pauseBtn = new QPushButton(SharedIcon(":/QtIcons/pause.png"), "");
    pauseBtn->setToolTip("Pause");
    playerBox->addWidget(pauseBtn);
    restartBtn = new QPushButton(SharedIcon(":/QtIcons/restart.png"), "");
    restartBtn->setToolTip("Restart");
    playerBox->addWidget(restartBtn);
    stepForwardBtn = new QPushButton(SharedIcon(":/QtIcons/step_forward.png"), "");
    stepForwardBtn->setToolTip("Step forward");
    playerBox->addWidget(stepForwardBtn);
    stepForwardFPSSpin = new QSpinBox(this);
    stepForwardFPSSpin->setMinimum(1);
    stepForwardFPSSpin->setMaximum(100);
    stepForwardFPSSpin->setValue(30);
    playerBox->addWidget(stepForwardFPSSpin);
    playerBox->addWidget(new QLabel("step FPS"));
    playerBox->addStretch();

    connect(playBtn, SIGNAL(clicked(bool)), this, SLOT(OnPlay()));
    connect(stopBtn, SIGNAL(clicked(bool)), this, SLOT(OnStop()));
    connect(stopAndDeleteBtn, SIGNAL(clicked(bool)), this, SLOT(OnStopAndDelete()));
    connect(pauseBtn, SIGNAL(clicked(bool)), this, SLOT(OnPause()));
    connect(restartBtn, SIGNAL(clicked(bool)), this, SLOT(OnRestart()));
    connect(stepForwardBtn, SIGNAL(clicked(bool)), this, SLOT(OnStepForward()));

    mainLayout->addLayout(playerBox);

    connect(effectPlaybackSpeed, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged()));

    effectTree = new QTreeWidget();
    mainLayout->addWidget(effectTree);
    effectTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(effectTree, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenuForEffectTree(const QPoint&)));
    connect(effectTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(OnTreeItemDoubleClck(QTreeWidgetItem*, int)));
    iconEmitter = SharedIcon(":/QtIcons/emitter_particle.png");
    iconLayer = SharedIcon(":/QtIcons/layer_particle.png");
    iconForce = SharedIcon(":/QtIcons/force.png");
    iconExternal = SharedIcon(":/QtIcons/external.png");

    mainLayout->addWidget(new QLabel("Effect Variables"));
    effectVariables = new QTableWidget(this);
    effectVariables->setColumnCount(2);
    effectVariables->setRowCount(0);
    effectEditDelegate = new VariableEditDelegate(effectVariables, effectVariables);
    effectVariables->setItemDelegate(effectEditDelegate);
    mainLayout->addWidget(effectVariables);
    connect(effectVariables, SIGNAL(cellChanged(int, int)), this, SLOT(OnVariableValueChanged(int, int)));

    mainLayout->addWidget(new QLabel("Global variables"));
    globalVariables = new QTableWidget(this);
    globalVariables->setColumnCount(2);
    globalVariables->setRowCount(0);
    globalEditDelegate = new VariableEditDelegate(globalVariables, globalVariables);
    globalVariables->setItemDelegate(globalEditDelegate);
    mainLayout->addWidget(globalVariables);
    connect(globalVariables, SIGNAL(cellChanged(int, int)), this, SLOT(OnGlobalVariableValueChanged(int, int)));

    QHBoxLayout* addGlobalBox = new QHBoxLayout();
    addGlobalBox->addStretch();
    QPushButton* addGlobal = new QPushButton(this);
    addGlobal->setText("+");
    addGlobal->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    addGlobalBox->addWidget(addGlobal);
    mainLayout->addLayout(addGlobalBox);

    connect(addGlobal, SIGNAL(clicked(bool)), this, SLOT(OnAddGlobalExternal()));

    mainLayout->addStretch();

    blockSignals = false;
    blockTables = false;
}

void ParticleEffectPropertiesWidget::InitWidget(QWidget* widget, bool connectWidget)
{
    mainLayout->addWidget(widget);
    if (connectWidget)
        connect(widget, SIGNAL(ValueChanged()), this, SLOT(OnValueChanged()));
}

void ParticleEffectPropertiesWidget::UpdateVaribleTables()
{
    blockTables = true;
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
    DAVA::Set<DAVA::String> variablesSet = effect->EnumerateVariables();
    effectVariables->clearContents();
    effectVariables->setRowCount(static_cast<int>(variablesSet.size()));
    DAVA::int32 i = 0;
    for (DAVA::Set<DAVA::String>::iterator it = variablesSet.begin(), e = variablesSet.end(); it != e; ++it)
    {
        QTableWidgetItem* varName = new QTableWidgetItem(QString((*it).c_str()));
        varName->setFlags(Qt::NoItemFlags);
        effectVariables->setItem(i, 0, varName);
        QTableWidgetItem* varValue = new QTableWidgetItem(QString::number(effect->GetExternalValue(*it)));
        effectVariables->setItem(i, 1, varValue);
        i++;
    }

    DAVA::Map<DAVA::String, DAVA::float32> globalVariablesSet = effect->GetEntity()->GetScene()->particleEffectSystem->GetGlobalExternals();
    globalVariables->clearContents();
    globalVariables->setRowCount(static_cast<int>(globalVariablesSet.size()));
    i = 0;
    for (DAVA::Map<DAVA::String, DAVA::float32>::iterator it = globalVariablesSet.begin(), e = globalVariablesSet.end(); it != e; ++it)
    {
        QTableWidgetItem* varName = new QTableWidgetItem(QString((*it).first.c_str()));
        varName->setFlags(Qt::NoItemFlags);
        globalVariables->setItem(i, 0, varName);
        QTableWidgetItem* varValue = new QTableWidgetItem(QString::number((*it).second));
        globalVariables->setItem(i, 1, varValue);
        i++;
    }

    blockTables = false;
}

void ParticleEffectPropertiesWidget::OnVariableValueChanged(int row, int col)
{
    if (blockTables)
        return;
    DAVA::String varNam = effectVariables->item(row, 0)->text().toStdString();
    float varValue = effectVariables->item(row, 1)->text().toFloat();
    GetEffect(GetActiveScene())->SetExtertnalValue(varNam, varValue);
}

void ParticleEffectPropertiesWidget::OnGlobalVariableValueChanged(int row, int col)
{
    if (blockTables)
        return;
    DAVA::String varNam = globalVariables->item(row, 0)->text().toStdString();
    float varValue = globalVariables->item(row, 1)->text().toFloat();
    GetEffect(GetActiveScene())->GetEntity()->GetScene()->particleEffectSystem->SetGlobalExtertnalValue(varNam, varValue);
    UpdateVaribleTables();
}

void ParticleEffectPropertiesWidget::OnAddGlobalExternal()
{
    AddGlobalExternalDialog dialog(this);
    if (dialog.exec())
    {
        GetEffect(GetActiveScene())->GetEntity()->GetScene()->particleEffectSystem->SetGlobalExtertnalValue(dialog.GetVariableName(), dialog.GetVariableValue());
        UpdateVaribleTables();
    }
}

void ParticleEffectPropertiesWidget::ShowContextMenuForEffectTree(const QPoint& pos)
{
    QMenu contextMenu;
    QTreeWidgetItem* target = effectTree->itemAt(pos);
    if (!target)
        return;
    currSelectedTreeItem = target;
    EffectTreeData treeData = currSelectedTreeItem->data(0, Qt::UserRole).value<EffectTreeData>();
    int i;
    switch (currSelectedTreeItem->type())
    {
    case TreeItemTypeEmitter:
        for (i = 0; i < EE_TOTAL; ++i)
        {
            if (!GetEmitterLine(treeData.emmiter, EmitterExternals(i)))
            {
                contextMenu.addAction(QString("Add External ") + QString(EXTERNAL_NAMES[i].c_str()))->setData(QVariant(i));
            }
        }
        break;
    case TreeItemTypeLayer:
        for (i = EE_TOTAL; i < EL_TOTAL; ++i)
        {
            if (!GetLayerLine(treeData.layer, LayerExternals(i)))
            {
                contextMenu.addAction(QString("Add External ") + QString(EXTERNAL_NAMES[i].c_str()))->setData(QVariant(i));
            }
        }
        break;
    case TreeItemTypeForce:
        for (i = EL_TOTAL; i < EF_TOTAL; ++i)
        {
            if (!GetForceLine(treeData.force, ForceExternals(i)))
            {
                contextMenu.addAction(QString("Add External ") + QString(EXTERNAL_NAMES[i].c_str()))->setData(QVariant(i));
            }
        }
        break;
    case TreeItemTypeExternal:
        contextMenu.addAction(QString("Remove External ") + QString(EXTERNAL_NAMES[treeData.externalParamId].c_str()))->setData(QVariant(-1));
    }
    connect(&contextMenu, SIGNAL(triggered(QAction*)), this, SLOT(OnContextMenuCommand(QAction*)));
    contextMenu.exec(effectTree->viewport()->mapToGlobal(pos));
}

void ParticleEffectPropertiesWidget::OnTreeItemDoubleClck(QTreeWidgetItem* treeItem, int column)
{
    EffectTreeData data = treeItem->data(0, Qt::UserRole).value<EffectTreeData>();
    if (treeItem->type() == TreeItemTypeExternal) //edit external on double click
    {
        int externalId = data.externalParamId;
        if (externalId < EE_TOTAL)
        {
            EditEmitterModifiable(data.emmiter, EmitterExternals(externalId));
        }
        else if (externalId < EL_TOTAL)
        {
            EditLayerModifiable(data.layer, LayerExternals(externalId));
        }
        else
        {
            EditForceModifiable(data.force, ForceExternals(externalId));
        }
    }
}

void ParticleEffectPropertiesWidget::OnContextMenuCommand(QAction* action)
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());

    int commandId = action->data().toInt();
    if (commandId == -1)
    {
        EffectTreeData data = currSelectedTreeItem->data(0, Qt::UserRole).value<EffectTreeData>();
        int externalId = data.externalParamId;
        if (externalId < EE_TOTAL)
        {
            effect->UnRegisterModifiable(GetEmitterLine(data.emmiter, EmitterExternals(externalId)));
            RemoveEmitterLineModifiable(data.emmiter, EmitterExternals(externalId));
            UpdateVaribleTables();
        }
        else if (externalId < EL_TOTAL)
        {
            effect->UnRegisterModifiable(GetLayerLine(data.layer, LayerExternals(externalId)));
            RemoveLayerLineModifiable(data.layer, LayerExternals(externalId));
            UpdateVaribleTables();
        }
        else
        {
            effect->UnRegisterModifiable(GetForceLine(data.force, ForceExternals(externalId)));
            RemoveForceLineModifiable(data.force, ForceExternals(externalId));
            UpdateVaribleTables();
        }
        delete currSelectedTreeItem;
    }
    else if (commandId < EE_TOTAL)
    {
        EffectTreeData data = currSelectedTreeItem->data(0, Qt::UserRole).value<EffectTreeData>();
        SetEmitterLineModifiable(data.emmiter, EmitterExternals(commandId));
        if (EditEmitterModifiable(data.emmiter, EmitterExternals(commandId), true))
        {
            data.externalParamId = commandId;
            QTreeWidgetItem* externalItem = new QTreeWidgetItem(currSelectedTreeItem, TreeItemTypeExternal);
            externalItem->setText(0, QString("External ") + QString(EXTERNAL_NAMES[commandId].c_str()));
            externalItem->setIcon(0, iconExternal);
            externalItem->setData(0, Qt::UserRole, QVariant::fromValue(data));
        }
        else
        {
            RemoveEmitterLineModifiable(data.emmiter, EmitterExternals(commandId));
        }
    }
    else if (commandId < EL_TOTAL)
    {
        EffectTreeData data = currSelectedTreeItem->data(0, Qt::UserRole).value<EffectTreeData>();
        SetLayerLineModifiable(data.layer, LayerExternals(commandId));
        if (EditLayerModifiable(data.layer, LayerExternals(commandId), true))
        {
            data.externalParamId = commandId;
            QTreeWidgetItem* externalItem = new QTreeWidgetItem(currSelectedTreeItem, TreeItemTypeExternal);
            externalItem->setText(0, QString("External ") + QString(EXTERNAL_NAMES[commandId].c_str()));
            externalItem->setIcon(0, iconExternal);
            externalItem->setData(0, Qt::UserRole, QVariant::fromValue(data));
        }
        else
        {
            RemoveLayerLineModifiable(data.layer, LayerExternals(commandId));
        }
    }
    else
    {
        EffectTreeData data = currSelectedTreeItem->data(0, Qt::UserRole).value<EffectTreeData>();
        SetForceLineModifiable(data.force, ForceExternals(commandId));

        effect->RegisterModifiable(GetForceLine(data.force, ForceExternals(commandId)));
        UpdateVaribleTables();

        if (EditForceModifiable(data.force, ForceExternals(commandId), true))
        {
            data.externalParamId = commandId;
            QTreeWidgetItem* externalItem = new QTreeWidgetItem(currSelectedTreeItem, TreeItemTypeExternal);
            externalItem->setText(0, QString("External ") + QString(EXTERNAL_NAMES[commandId].c_str()));
            externalItem->setIcon(0, iconExternal);
            externalItem->setData(0, Qt::UserRole, QVariant::fromValue(data));
        }
        else
        {
            RemoveForceLineModifiable(data.force, ForceExternals(commandId));
        }
    }
}

void ParticleEffectPropertiesWidget::OnValueChanged()
{
    if (blockSignals)
        return;

    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());

    DVASSERT(effect != nullptr);
    DAVA::float32 playbackSpeed = ConvertFromSliderValueToPlaybackSpeed(effectPlaybackSpeed->value());

    std::unique_ptr<CommandUpdateEffect> commandUpdateEffect(new CommandUpdateEffect(effect));
    commandUpdateEffect->Init(playbackSpeed);

    DVASSERT(GetActiveScene() != nullptr);
    GetActiveScene()->Exec(std::move(commandUpdateEffect));
    GetActiveScene()->MarkAsChanged();

    Init(GetActiveScene(), effect);
}

void ParticleEffectPropertiesWidget::OnPlay()
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
    DVASSERT(effect != nullptr);
    effect->Start();
}

void ParticleEffectPropertiesWidget::OnStop()
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
    DVASSERT(effect != nullptr);
    effect->Stop(false);
}

void ParticleEffectPropertiesWidget::OnStopAndDelete()
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
    DVASSERT(effect != nullptr);
    effect->Stop(true);
}

void ParticleEffectPropertiesWidget::OnPause()
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
    DVASSERT(effect != nullptr);
    effect->Pause(!effect->IsPaused());
}

void ParticleEffectPropertiesWidget::OnRestart()
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
    DVASSERT(effect != nullptr);
    effect->Restart();
}

void ParticleEffectPropertiesWidget::OnStepForward()
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
    DVASSERT(effect != nullptr);
    DAVA::float32 step = 1.0f / static_cast<DAVA::float32>(stepForwardFPSSpin->value());
    effect->Step(step);
}

void ParticleEffectPropertiesWidget::Init(SceneEditor2* scene, DAVA::ParticleEffectComponent* effect)
{
    DVASSERT(effect != nullptr);
    SetObjectsForScene(scene, effect, nullptr);

    blockSignals = true;

    // Normalize Playback Speed to the UISlider range.
    DAVA::float32 playbackSpeed = effect->GetPlaybackSpeed();
    effectPlaybackSpeed->setValue(ConvertFromPlaybackSpeedToSliderValue(playbackSpeed));
    UpdatePlaybackSpeedLabel();
    BuildEffectTree();
    UpdateVaribleTables();
    blockSignals = false;
}

DAVA::ModifiablePropertyLineBase* ParticleEffectPropertiesWidget::GetEmitterLine(DAVA::ParticleEmitter* emitter, EmitterExternals lineId)
{
    switch (lineId)
    {
    case EE_EMISSION_VECTOR:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(emitter->emissionVector.Get());
    case EE_EMISSION_RANGE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(emitter->emissionRange.Get());
    case EE_RADUS:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(emitter->radius.Get());
    case EE_SIZE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(emitter->size.Get());
    case EE_COLOR_OVER_LIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(emitter->colorOverLife.Get());

    default:
        break;
    }
    return NULL;
}
DAVA::ModifiablePropertyLineBase* ParticleEffectPropertiesWidget::GetLayerLine(DAVA::ParticleLayer* layer, LayerExternals lineId)
{
    switch (lineId)
    {
    case EL_LIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->life.Get());
    case EL_LIFE_VARIATION:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->lifeVariation.Get());
    case EL_NUMBER:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->number.Get());
    case EL_NUMBER_VARIATION:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->numberVariation.Get());
    case EL_SIZE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->size.Get());
    case EL_SIZE_VARIATION:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->sizeVariation.Get());
    case EL_SIZE_OVERLIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->sizeOverLifeXY.Get());
    case EL_VELOCITY:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->velocity.Get());
    case EL_VELOCITY_VARIATON:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->velocityVariation.Get());
    case EL_VELOCITY_OVERLIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->velocityOverLife.Get());
    case EL_SPIN:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->spin.Get());
    case EL_SPIN_VARIATION:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->spinVariation.Get());
    case EL_SPIN_OVERLIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->spinOverLife.Get());
    case EL_COLOR:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->colorRandom.Get());
    case EL_ALPHA_OVERLIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->alphaOverLife.Get());
    case EL_COLOR_OVERLIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->colorOverLife.Get());
    case EL_ANGLE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->angle.Get());
    case EL_ANGLE_VARIATION:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->angleVariation.Get());
    case EL_ANIM_SPEED_OVERLIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(layer->animSpeedOverLife.Get());

    default:
        break;
    }
    return NULL;
}
DAVA::ModifiablePropertyLineBase* ParticleEffectPropertiesWidget::GetForceLine(DAVA::ParticleForceSimplified* force, ForceExternals lineId)
{
    switch (lineId)
    {
    case EF_FORCE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(force->force.Get());
    case EF_FORCE_OVERLIFE:
        return dynamic_cast<DAVA::ModifiablePropertyLineBase*>(force->forceOverLife.Get());
    default:
        break;
    }
    return NULL;
}

void ParticleEffectPropertiesWidget::SetEmitterLineModifiable(DAVA::ParticleEmitter* emitter, EmitterExternals lineId)
{
    switch (lineId)
    {
    case EE_EMISSION_VECTOR:
        DAVA::PropertyLineHelper::MakeModifiable(emitter->emissionVector);
        break;
    case EE_EMISSION_RANGE:
        DAVA::PropertyLineHelper::MakeModifiable(emitter->emissionRange);
        break;
    case EE_RADUS:
        DAVA::PropertyLineHelper::MakeModifiable(emitter->radius);
        break;
    case EE_SIZE:
        DAVA::PropertyLineHelper::MakeModifiable(emitter->size);
        break;
    case EE_COLOR_OVER_LIFE:
        DAVA::PropertyLineHelper::MakeModifiable(emitter->colorOverLife);
        break;

    default:
        break;
    }
}
void ParticleEffectPropertiesWidget::SetLayerLineModifiable(DAVA::ParticleLayer* layer, LayerExternals lineId)
{
    switch (lineId)
    {
    case EL_LIFE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->life);
        break;
    case EL_LIFE_VARIATION:
        DAVA::PropertyLineHelper::MakeModifiable(layer->lifeVariation);
        break;
    case EL_NUMBER:
        DAVA::PropertyLineHelper::MakeModifiable(layer->number);
        break;
    case EL_NUMBER_VARIATION:
        DAVA::PropertyLineHelper::MakeModifiable(layer->numberVariation);
        break;
    case EL_SIZE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->size);
        break;
    case EL_SIZE_VARIATION:
        DAVA::PropertyLineHelper::MakeModifiable(layer->sizeVariation);
        break;
    case EL_SIZE_OVERLIFE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->sizeOverLifeXY);
        break;
    case EL_VELOCITY:
        DAVA::PropertyLineHelper::MakeModifiable(layer->velocity);
        break;
    case EL_VELOCITY_VARIATON:
        DAVA::PropertyLineHelper::MakeModifiable(layer->velocityVariation);
        break;
    case EL_VELOCITY_OVERLIFE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->velocityOverLife);
        break;
    case EL_SPIN:
        DAVA::PropertyLineHelper::MakeModifiable(layer->spin);
        break;
    case EL_SPIN_VARIATION:
        DAVA::PropertyLineHelper::MakeModifiable(layer->spinVariation);
        break;
    case EL_SPIN_OVERLIFE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->spinOverLife);
        break;
    case EL_COLOR:
        DAVA::PropertyLineHelper::MakeModifiable(layer->colorRandom);
        break;
    case EL_ALPHA_OVERLIFE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->alphaOverLife);
        break;
    case EL_COLOR_OVERLIFE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->colorOverLife);
        break;
    case EL_ANGLE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->angle);
        break;
    case EL_ANGLE_VARIATION:
        DAVA::PropertyLineHelper::MakeModifiable(layer->angleVariation);
        break;
    case EL_ANIM_SPEED_OVERLIFE:
        DAVA::PropertyLineHelper::MakeModifiable(layer->animSpeedOverLife);
        break;

    default:
        break;
    }
}
void ParticleEffectPropertiesWidget::SetForceLineModifiable(DAVA::ParticleForceSimplified* force, ForceExternals lineId)
{
    switch (lineId)
    {
    case EF_FORCE:
        DAVA::PropertyLineHelper::MakeModifiable(force->force);
        break;
    case EF_FORCE_OVERLIFE:
        DAVA::PropertyLineHelper::MakeModifiable(force->forceOverLife);
        break;

    default:
        break;
    }
}

void ParticleEffectPropertiesWidget::RemoveEmitterLineModifiable(DAVA::ParticleEmitter* emitter, EmitterExternals lineId)
{
    switch (lineId)
    {
    case EE_EMISSION_VECTOR:
        DAVA::PropertyLineHelper::RemoveModifiable(emitter->emissionVector);
        break;
    case EE_EMISSION_RANGE:
        DAVA::PropertyLineHelper::RemoveModifiable(emitter->emissionRange);
        break;
    case EE_RADUS:
        DAVA::PropertyLineHelper::RemoveModifiable(emitter->radius);
        break;
    case EE_SIZE:
        DAVA::PropertyLineHelper::RemoveModifiable(emitter->size);
        break;
    case EE_COLOR_OVER_LIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(emitter->colorOverLife);
        break;

    default:
        break;
    }
}
void ParticleEffectPropertiesWidget::RemoveLayerLineModifiable(DAVA::ParticleLayer* layer, LayerExternals lineId)
{
    switch (lineId)
    {
    case EL_LIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->life);
        break;
    case EL_LIFE_VARIATION:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->lifeVariation);
        break;
    case EL_NUMBER:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->number);
        break;
    case EL_NUMBER_VARIATION:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->numberVariation);
        break;
    case EL_SIZE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->size);
        break;
    case EL_SIZE_VARIATION:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->sizeVariation);
        break;
    case EL_SIZE_OVERLIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->sizeOverLifeXY);
        break;
    case EL_VELOCITY:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->velocity);
        break;
    case EL_VELOCITY_VARIATON:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->velocityVariation);
        break;
    case EL_VELOCITY_OVERLIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->velocityOverLife);
        break;
    case EL_SPIN:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->spin);
        break;
    case EL_SPIN_VARIATION:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->spinVariation);
        break;
    case EL_SPIN_OVERLIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->spinOverLife);
        break;
    case EL_COLOR:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->colorRandom);
        break;
    case EL_ALPHA_OVERLIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->alphaOverLife);
        break;
    case EL_COLOR_OVERLIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->colorOverLife);
        break;
    case EL_ANGLE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->angle);
        break;
    case EL_ANGLE_VARIATION:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->angleVariation);
        break;
    case EL_ANIM_SPEED_OVERLIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(layer->animSpeedOverLife);
        break;

    default:
        break;
    }
}
void ParticleEffectPropertiesWidget::RemoveForceLineModifiable(DAVA::ParticleForceSimplified* force, ForceExternals lineId)
{
    switch (lineId)
    {
    case EF_FORCE:
        DAVA::PropertyLineHelper::RemoveModifiable(force->force);
        break;
    case EF_FORCE_OVERLIFE:
        DAVA::PropertyLineHelper::RemoveModifiable(force->forceOverLife);
        break;

    default:
        break;
    }
}

bool ParticleEffectPropertiesWidget::EditEmitterModifiable(DAVA::ParticleEmitter* emitter, EmitterExternals lineId, bool onAdd)
{
    switch (lineId)
    {
    case EE_EMISSION_VECTOR:
        return EditModificationLine(emitter->emissionVector, onAdd);
    case EE_EMISSION_RANGE:
        return EditModificationLine(emitter->emissionRange, onAdd);
    case EE_RADUS:
        return EditModificationLine(emitter->radius, onAdd);
    case EE_SIZE:
        return EditModificationLine(emitter->size, onAdd);
    case EE_COLOR_OVER_LIFE:
        return EditModificationLine(emitter->colorOverLife, onAdd);
    default:
        break;
    }
    return false;
}
bool ParticleEffectPropertiesWidget::EditLayerModifiable(DAVA::ParticleLayer* layer, LayerExternals lineId, bool onAdd)
{
    switch (lineId)
    {
    case EL_LIFE:
        return EditModificationLine(layer->life, onAdd);
    case EL_LIFE_VARIATION:
        return EditModificationLine(layer->lifeVariation, onAdd);
    case EL_NUMBER:
        return EditModificationLine(layer->number, onAdd);
    case EL_NUMBER_VARIATION:
        return EditModificationLine(layer->numberVariation, onAdd);
    case EL_SIZE:
        return EditModificationLine(layer->size, onAdd);
    case EL_SIZE_VARIATION:
        return EditModificationLine(layer->sizeVariation, onAdd);
    case EL_SIZE_OVERLIFE:
        return EditModificationLine(layer->sizeOverLifeXY, onAdd);
    case EL_VELOCITY:
        return EditModificationLine(layer->velocity, onAdd);
    case EL_VELOCITY_VARIATON:
        return EditModificationLine(layer->velocityVariation, onAdd);
    case EL_VELOCITY_OVERLIFE:
        return EditModificationLine(layer->velocityOverLife, onAdd);
    case EL_SPIN:
        return EditModificationLine(layer->spin, onAdd);
    case EL_SPIN_VARIATION:
        return EditModificationLine(layer->spinVariation, onAdd);
    case EL_SPIN_OVERLIFE:
        return EditModificationLine(layer->spinOverLife, onAdd);
    case EL_COLOR:
        return EditModificationLine(layer->colorRandom, onAdd);
    case EL_ALPHA_OVERLIFE:
        return EditModificationLine(layer->alphaOverLife, onAdd);
    case EL_COLOR_OVERLIFE:
        return EditModificationLine(layer->colorOverLife, onAdd);
    case EL_ANGLE:
        return EditModificationLine(layer->angle, onAdd);
    case EL_ANGLE_VARIATION:
        return EditModificationLine(layer->angleVariation, onAdd);
    case EL_ANIM_SPEED_OVERLIFE:
        return EditModificationLine(layer->animSpeedOverLife, onAdd);
    default:
        break;
    }
    return false;
}
bool ParticleEffectPropertiesWidget::EditForceModifiable(DAVA::ParticleForceSimplified* force, ForceExternals lineId, bool onAdd)
{
    switch (lineId)
    {
    case EF_FORCE:
        return EditModificationLine(force->force, onAdd);
    case EF_FORCE_OVERLIFE:
        return EditModificationLine(force->forceOverLife, onAdd);
    default:
        break;
    }
    return false;
}

void ParticleEffectPropertiesWidget::BuildEffectTree()
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());

    currSelectedTreeItem = nullptr;
    effectTree->clear();
    effectTree->setHeaderLabel(QString(effect->GetEntity()->GetName().c_str()));
    QTreeWidgetItem* root = effectTree->invisibleRootItem();
    EffectTreeData data;
    DAVA::int32 childrenCount = effect->GetEmittersCount();
    for (DAVA::int32 emitterId = 0; emitterId < childrenCount; emitterId++)
    {
        auto instance = effect->GetEmitterInstance(emitterId);
        auto emitter = instance->GetEmitter();
        data.emmiter = emitter;
        QTreeWidgetItem* emitterItem = new QTreeWidgetItem(root, TreeItemTypeEmitter);
        emitterItem->setText(0, QString(emitter->name.c_str()));
        emitterItem->setIcon(0, iconEmitter);
        emitterItem->setData(0, Qt::UserRole, QVariant::fromValue(data));
        //externals
        for (DAVA::int32 externalId = 0; externalId < EE_TOTAL; ++externalId)
        {
            if (GetEmitterLine(emitter, EmitterExternals(externalId)))
            {
                data.externalParamId = externalId;
                QTreeWidgetItem* externalItem = new QTreeWidgetItem(emitterItem, TreeItemTypeExternal);
                externalItem->setText(0, QString("External ") + QString(EXTERNAL_NAMES[externalId].c_str()));
                externalItem->setIcon(0, iconExternal);
                externalItem->setData(0, Qt::UserRole, QVariant::fromValue(data));
            }
        }
        data.externalParamId = 0;

        // layers
        for (auto layer : emitter->layers)
        {
            QTreeWidgetItem* layerItem = new QTreeWidgetItem(emitterItem, TreeItemTypeLayer);
            data.layer = layer;
            layerItem->setText(0, QString(layer->layerName.c_str()));
            layerItem->setIcon(0, iconLayer);
            layerItem->setData(0, Qt::UserRole, QVariant::fromValue(data));

            // externals
            for (DAVA::int32 externalId = EE_TOTAL; externalId < EL_TOTAL; ++externalId)
            {
                if (GetLayerLine(layer, LayerExternals(externalId)))
                {
                    data.externalParamId = externalId;
                    QTreeWidgetItem* externalItem = new QTreeWidgetItem(layerItem, TreeItemTypeExternal);
                    externalItem->setText(0, QString("External ") + QString(EXTERNAL_NAMES[externalId].c_str()));
                    externalItem->setIcon(0, iconExternal);
                    externalItem->setData(0, Qt::UserRole, QVariant::fromValue(data));
                }
            }
            data.externalParamId = 0;

            // forces
            for (auto force : layer->GetSimplifiedParticleForces())
            {
                data.force = force;
                QTreeWidgetItem* forceItem = new QTreeWidgetItem(layerItem, TreeItemTypeForce);
                forceItem->setText(0, QString("force"));
                forceItem->setIcon(0, iconForce);
                forceItem->setData(0, Qt::UserRole, QVariant::fromValue(data));
                //externals
                for (DAVA::int32 externalId = EL_TOTAL; externalId < EF_TOTAL; ++externalId)
                {
                    if (GetForceLine(force, ForceExternals(externalId)))
                    {
                        data.externalParamId = externalId;
                        QTreeWidgetItem* externalItem = new QTreeWidgetItem(forceItem, TreeItemTypeExternal);
                        externalItem->setText(0, QString("External ") + QString(EXTERNAL_NAMES[externalId].c_str()));
                        externalItem->setIcon(0, iconExternal);
                        externalItem->setData(0, Qt::UserRole, QVariant::fromValue(data));
                    }
                }
                data.externalParamId = 0;
            }
            data.force = NULL;
        }
        data.layer = NULL;
    }
    effect->RebuildEffectModifiables();
}

void ParticleEffectPropertiesWidget::UpdatePlaybackSpeedLabel()
{
    DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
    if (effect != nullptr)
    {
        DAVA::float32 playbackSpeedValue = effect->GetPlaybackSpeed();
        effectPlaybackSpeedLabel->setText(QString("playback speed: %1x").arg(playbackSpeedValue));
    }
}

void ParticleEffectPropertiesWidget::StoreVisualState(DAVA::KeyedArchive* /* visualStateProps */)
{
    // Nothing to store for now.
}

void ParticleEffectPropertiesWidget::RestoreVisualState(DAVA::KeyedArchive* /* visualStateProps */)
{
    // Nothing to restore for now.
}

AddGlobalExternalDialog::AddGlobalExternalDialog(QWidget* parent)
    : QDialog(parent)
{
    setMinimumWidth(200);
    QVBoxLayout* dialogLayout = new QVBoxLayout();
    setLayout(dialogLayout);
    QHBoxLayout* nameLayot = new QHBoxLayout();
    nameLayot->addWidget(new QLabel("Name"));
    variableName = new QLineEdit();
    variableName->setText("Variable");
    nameLayot->addWidget(variableName);
    nameLayot->addStretch();
    nameLayot->addWidget(new QLabel("Value"));
    variableValue = new QDoubleSpinBox();
    variableValue->setMinimum(0.0f);
    variableValue->setMaximum(1.0f);
    variableValue->setDecimals(3);
    variableValue->setSingleStep(0.001f);
    variableValue->setValue(0);
    nameLayot->addWidget(variableValue);
    dialogLayout->addLayout(nameLayot);
    dialogLayout->addStretch();
    variableName->setFocus();
    variableName->selectAll();

    QHBoxLayout* btnBox = new QHBoxLayout();
    QPushButton* btnCancel = new QPushButton("Cancel", this);
    QPushButton* btnOk = new QPushButton("Ok", this);
    btnOk->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    btnCancel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    btnBox->addWidget(btnCancel);
    btnBox->addStretch();
    btnBox->addWidget(btnOk);
    dialogLayout->addLayout(btnBox);
    connect(btnOk, SIGNAL(clicked(bool)), this, SLOT(accept()));
    connect(btnCancel, SIGNAL(clicked(bool)), this, SLOT(reject()));
    btnOk->setDefault(true);
}

DAVA::String AddGlobalExternalDialog::GetVariableName()
{
    return variableName->text().toStdString();
}
DAVA::float32 AddGlobalExternalDialog::GetVariableValue()
{
    return static_cast<DAVA::float32>(variableValue->value());
}

void EditModificationLineDialog::InitName(const DAVA::String& name, bool onAdd)
{
    QHBoxLayout* nameLayot = new QHBoxLayout();
    QLabel* varNameLabel = new QLabel("External Variable Name");
    nameLayot->addWidget(varNameLabel);
    variableName = new QLineEdit();
    variableName->setText(QString(name.c_str()));
    nameLayot->addWidget(variableName);
    dialogLayout->addLayout(nameLayot);
    if (onAdd)
    {
        variableName->setFocus();
        variableName->selectAll();
    }
}

DAVA::String EditModificationLineDialog::GetVariableName()
{
    return variableName->text().toStdString();
}

void EditModificationLineDialog::InitButtons()
{
    dialogLayout->addStretch();
    QHBoxLayout* btnBox = new QHBoxLayout();
    QPushButton* btnCancel = new QPushButton("Cancel", this);
    QPushButton* btnOk = new QPushButton("Ok", this);
    btnOk->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    btnCancel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    btnBox->addWidget(btnCancel);
    btnBox->addStretch();
    btnBox->addWidget(btnOk);
    dialogLayout->addLayout(btnBox);
    connect(btnOk, SIGNAL(clicked(bool)), this, SLOT(accept()));
    connect(btnCancel, SIGNAL(clicked(bool)), this, SLOT(reject()));
    btnOk->setDefault(true);
}

template <>
void EditModificationLineDialog::Init<DAVA::float32>(DAVA::ModifiablePropertyLine<DAVA::float32>* line, bool onAdd)
{
    dialogLayout = new QVBoxLayout();
    setLayout(dialogLayout);
    InitName(line->GetValueName(), onAdd);
    timeLine = new TimeLineWidget(this);
    timeLine->Init(0.0f, 1.0f, false);
    timeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(line->GetModificationLine()).GetProps(), Qt::blue, "");
    dialogLayout->addWidget(timeLine);
    InitButtons();
}

template <>
void EditModificationLineDialog::Init<DAVA::Vector2>(DAVA::ModifiablePropertyLine<DAVA::Vector2>* line, bool onAdd)
{
    dialogLayout = new QVBoxLayout();
    setLayout(dialogLayout);
    InitName(line->GetValueName(), onAdd);
    timeLine = new TimeLineWidget(this);
    timeLine->Init(0.0f, 1.0f, false, true);
    DAVA::Vector<QColor> vectorColors;
    vectorColors.push_back(Qt::red);
    vectorColors.push_back(Qt::darkGreen);
    DAVA::Vector<QString> vectorLegends;
    vectorLegends.push_back("x");
    vectorLegends.push_back("y");
    timeLine->AddLines(DAVA::PropLineWrapper<DAVA::Vector2>(line->GetModificationLine()).GetProps(), vectorColors, vectorLegends);
    dialogLayout->addWidget(timeLine);
    InitButtons();
}

template <>
void EditModificationLineDialog::Init<DAVA::Vector3>(DAVA::ModifiablePropertyLine<DAVA::Vector3>* line, bool onAdd)
{
    dialogLayout = new QVBoxLayout();
    setLayout(dialogLayout);
    InitName(line->GetValueName(), onAdd);
    timeLine = new TimeLineWidget(this);
    timeLine->Init(0.0f, 1.0f, false, true);
    DAVA::Vector<QColor> vectorColors;
    vectorColors.push_back(Qt::red);
    vectorColors.push_back(Qt::darkGreen);
    vectorColors.push_back(Qt::blue);
    DAVA::Vector<QString> vectorLegends;
    vectorLegends.push_back("x");
    vectorLegends.push_back("y");
    vectorLegends.push_back("z");
    timeLine->AddLines(DAVA::PropLineWrapper<DAVA::Vector3>(line->GetModificationLine()).GetProps(), vectorColors, vectorLegends);
    dialogLayout->addWidget(timeLine);
    InitButtons();
}

template <>
void EditModificationLineDialog::Init<DAVA::Color>(DAVA::ModifiablePropertyLine<DAVA::Color>* line, bool onAdd)
{
    dialogLayout = new QVBoxLayout();
    setLayout(dialogLayout);
    InitName(line->GetValueName(), onAdd);
    gradientLine = new GradientPickerWidget(this);
    gradientLine->Init(0, 1);
    gradientLine->SetValues(DAVA::PropLineWrapper<DAVA::Color>(line->GetModificationLine()).GetProps());
    dialogLayout->addWidget(gradientLine);
    InitButtons();
}

template <>
void EditModificationLineDialog::UpdateLine<DAVA::float32>(DAVA::ModifiablePropertyLine<DAVA::float32>* line)
{
    DAVA::PropLineWrapper<DAVA::float32> lineWrap;
    if (!timeLine->GetValue(0, lineWrap.GetPropsPtr()))
        return;
    line->SetModificationLine(lineWrap.GetPropLine());
}

template <>
void EditModificationLineDialog::UpdateLine<DAVA::Vector2>(DAVA::ModifiablePropertyLine<DAVA::Vector2>* line)
{
    DAVA::PropLineWrapper<DAVA::Vector2> lineWrap;
    if (!timeLine->GetValues(lineWrap.GetPropsPtr()))
        return;
    line->SetModificationLine(lineWrap.GetPropLine());
}

template <>
void EditModificationLineDialog::UpdateLine<DAVA::Vector3>(DAVA::ModifiablePropertyLine<DAVA::Vector3>* line)
{
    DAVA::PropLineWrapper<DAVA::Vector3> lineWrap;
    if (!timeLine->GetValues(lineWrap.GetPropsPtr()))
        return;
    line->SetModificationLine(lineWrap.GetPropLine());
}

template <>
void EditModificationLineDialog::UpdateLine<DAVA::Color>(DAVA::ModifiablePropertyLine<DAVA::Color>* line)
{
    DAVA::PropLineWrapper<DAVA::Color> lineWrap;
    if (!gradientLine->GetValues(lineWrap.GetPropsPtr()))
        return;
    line->SetModificationLine(lineWrap.GetPropLine());
}

QWidget* VariableEditDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QDoubleSpinBox* spinBox = new QDoubleSpinBox(parent);
    spinBox->setMinimum(0.0f);
    spinBox->setMaximum(1.0f);
    spinBox->setDecimals(3);
    spinBox->setSingleStep(0.1f);
    return spinBox;
}
void VariableEditDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    static_cast<QDoubleSpinBox*>(editor)->setValue(editTable->item(index.row(), 1)->text().toDouble());
}
void VariableEditDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    editTable->item(index.row(), 1)->setText(QString::number(static_cast<QDoubleSpinBox*>(editor)->value()));
}
