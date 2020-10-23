#include "LayerForceSimplifiedWidget.h"
#include "TimeLineWidget.h"
#include "Commands2/ParticleEditorCommands.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QSizePolicy>

LayerForceSimplifiedWidget::LayerForceSimplifiedWidget(QWidget* parent)
    : BaseParticleEditorContentWidget(parent)
{
    mainBox = new QVBoxLayout;
    this->setLayout(mainBox);

    forceTimeLine = new TimeLineWidget(this);
    InitWidget(forceTimeLine);
    forceOverLifeTimeLine = new TimeLineWidget(this);
    InitWidget(forceOverLifeTimeLine);

    blockSignals = false;
}

LayerForceSimplifiedWidget::~LayerForceSimplifiedWidget()
{
}

void LayerForceSimplifiedWidget::InitWidget(QWidget* widget)
{
    mainBox->addWidget(widget);
    connect(widget,
            SIGNAL(ValueChanged()),
            this,
            SLOT(OnValueChanged()));
}

void LayerForceSimplifiedWidget::Init(SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::uint32 forceIndex, bool updateMinimized)
{
    if (!layer || layer->GetSimplifiedParticleForces().size() <= forceIndex)
    {
        return;
    }

    this->layer = layer;
    this->forceIndex = forceIndex;

    blockSignals = true;

    DAVA::float32 lifeTime = layer->endTime;
    DAVA::ParticleForceSimplified* curForce = layer->GetSimplifiedParticleForces()[forceIndex];

    DAVA::Vector<QColor> colors;
    colors.push_back(Qt::red);
    colors.push_back(Qt::darkGreen);
    colors.push_back(Qt::blue);
    DAVA::Vector<QString> legends;
    legends.push_back("force x");
    legends.push_back("force y");
    legends.push_back("force z");
    forceTimeLine->Init(layer->startTime, lifeTime, updateMinimized, true, false);
    forceTimeLine->AddLines(DAVA::PropLineWrapper<DAVA::Vector3>(DAVA::PropertyLineHelper::GetValueLine(curForce->force)).GetProps(), colors, legends);
    forceTimeLine->EnableLock(true);

    legends.clear();
    legends.push_back("force variable x");
    legends.push_back("force variable y");
    legends.push_back("force variable z");

    forceOverLifeTimeLine->Init(0, 1, updateMinimized, true, false);
    forceOverLifeTimeLine->AddLine(0, DAVA::PropLineWrapper<DAVA::float32>(DAVA::PropertyLineHelper::GetValueLine(curForce->forceOverLife)).GetProps(), Qt::blue, "forces over life");

    blockSignals = false;
}

void LayerForceSimplifiedWidget::RestoreVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;

    forceTimeLine->SetVisualState(visualStateProps->GetArchive("FORCE_PROPS"));
    forceOverLifeTimeLine->SetVisualState(visualStateProps->GetArchive("FORCE_OVER_LIFE_PROPS"));
}

void LayerForceSimplifiedWidget::StoreVisualState(DAVA::KeyedArchive* visualStateProps)
{
    if (!visualStateProps)
        return;

    DAVA::KeyedArchive* props = new DAVA::KeyedArchive();

    forceTimeLine->GetVisualState(props);
    visualStateProps->SetArchive("FORCE_PROPS", props);

    props->DeleteAllKeys();
    forceOverLifeTimeLine->GetVisualState(props);
    visualStateProps->SetArchive("FORCE_OVER_LIFE_PROPS", props);

    DAVA::SafeRelease(props);
}

void LayerForceSimplifiedWidget::OnValueChanged()
{
    if (blockSignals)
        return;

    DAVA::PropLineWrapper<DAVA::Vector3> propForce;
    forceTimeLine->GetValues(propForce.GetPropsPtr());
    DAVA::PropLineWrapper<DAVA::float32> propForceOverLife;
    forceOverLifeTimeLine->GetValue(0, propForceOverLife.GetPropsPtr());

    std::unique_ptr<CommandUpdateParticleSimplifiedForce> updateForceCmd(new CommandUpdateParticleSimplifiedForce(layer, forceIndex));
    updateForceCmd->Init(propForce.GetPropLine(), propForceOverLife.GetPropLine());

    SceneEditor2* activeScene = GetActiveScene();
    DVASSERT(activeScene != nullptr);
    activeScene->Exec(std::move(updateForceCmd));
    activeScene->MarkAsChanged();

    Init(activeScene, layer, forceIndex, false);
    emit ValueChanged();
}

void LayerForceSimplifiedWidget::Update()
{
    Init(GetActiveScene(), layer, forceIndex, false);
}
