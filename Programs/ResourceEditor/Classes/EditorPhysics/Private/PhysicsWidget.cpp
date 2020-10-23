#include "Classes/EditorPhysics/Private/PhysicsWidget.h"
#include "Classes/EditorPhysics/Private/EditorPhysicsData.h"

#include <TArc/Controls/Widget.h>
#include <TArc/Controls/Label.h>
#include <TArc/Controls/ReflectedButton.h>

#include <Scene3D/Scene.h>
#include <Reflection/ReflectionRegistrator.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QLabel>

PhysicsWidget::PhysicsWidget(DAVA::ContextAccessor* accessor_, DAVA::UI* ui_)
    : accessor(accessor_)
    , ui(ui_)
{
    using namespace DAVA;

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(4);
    layout->setMargin(2);

    DAVA::Reflection model = DAVA::Reflection::Create(DAVA::ReflectedObject(this));
    {
        Label::Params params(accessor, ui, mainWindowKey);
        params.fields[Label::Fields::Text] = "labelText";
        Label* label = new Label(params, accessor, model, this);
        layout->addWidget(label->ToWidgetCast());
    }

    Widget* buttonsWidget = new Widget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setMargin(0);
    buttonsLayout->setSpacing(2);
    buttonsWidget->SetLayout(buttonsLayout);
    layout->addWidget(buttonsWidget->ToWidgetCast());

    {
        ReflectedButton::Params params(accessor, ui, mainWindowKey);
        params.fields[ReflectedButton::Fields::Icon] = "StartPauseIcon";
        params.fields[ReflectedButton::Fields::Clicked] = "OnStartPauseClick";
        params.fields[ReflectedButton::Fields::Enabled] = "isEnabled";
        ReflectedButton* playPauseButton = new ReflectedButton(params, accessor, model, buttonsWidget->ToWidgetCast());
        buttonsWidget->AddControl(playPauseButton);
    }

    {
        ReflectedButton::Params params(accessor, ui, mainWindowKey);
        params.fields[ReflectedButton::Fields::Icon] = "StopIcon";
        params.fields[ReflectedButton::Fields::Clicked] = "OnStopClick";
        params.fields[ReflectedButton::Fields::Enabled] = "isEnabled";
        ReflectedButton* stopButton = new ReflectedButton(params, accessor, model, buttonsWidget->ToWidgetCast());
        buttonsWidget->AddControl(stopButton);
    }

    buttonsLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    startIcon = QIcon(":/TArc/Resources/play.png");
    stopIcon = QIcon(":/TArc/Resources/stop.png");
    pauseIcon = QIcon(":/TArc/Resources/pause.png");
}

void PhysicsWidget::OnStartPauseClick()
{
    DAVA::DataContext* ctx = accessor->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    EditorPhysicsData* data = ctx->GetData<EditorPhysicsData>();
    DVASSERT(data != nullptr);

    EditorPhysicsSystem::eSimulationState state = data->system->GetSimulationState();
    if (state == EditorPhysicsSystem::eSimulationState::PLAYING)
    {
        data->system->SetSimulationState(EditorPhysicsSystem::eSimulationState::PAUSED);
    }
    else
    {
        data->system->SetSimulationState(EditorPhysicsSystem::eSimulationState::PLAYING);
    }
}

void PhysicsWidget::OnStopClick()
{
    DAVA::DataContext* ctx = accessor->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    EditorPhysicsData* data = ctx->GetData<EditorPhysicsData>();
    DVASSERT(data != nullptr);
    if (data->system->GetSimulationState() != EditorPhysicsSystem::eSimulationState::STOPPED)
    {
        data->system->SetSimulationState(EditorPhysicsSystem::eSimulationState::STOPPED);
    }
}

QIcon PhysicsWidget::GetStartPauseIcon() const
{
    DAVA::DataContext* ctx = accessor->GetActiveContext();
    if (ctx == nullptr)
    {
        return startIcon;
    }

    EditorPhysicsData* data = ctx->GetData<EditorPhysicsData>();
    DVASSERT(data != nullptr);

    if (data->system->GetSimulationState() == EditorPhysicsSystem::eSimulationState::PLAYING)
    {
        return pauseIcon;
    }

    return startIcon;
}

QIcon PhysicsWidget::GetStopIcon() const
{
    return stopIcon;
}

QString PhysicsWidget::GetLabelText() const
{
    EditorPhysicsSystem::eSimulationState state = EditorPhysicsSystem::eSimulationState::STOPPED;
    DAVA::DataContext* ctx = accessor->GetActiveContext();
    if (ctx != nullptr)
    {
        EditorPhysicsData* data = ctx->GetData<EditorPhysicsData>();
        DVASSERT(data != nullptr);

        state = data->system->GetSimulationState();
    }

    QString stateText;
    switch (state)
    {
    case EditorPhysicsSystem::eSimulationState::PLAYING:
        stateText = QString("Playing");
        break;
    case EditorPhysicsSystem::eSimulationState::PAUSED:
        stateText = QString("Paused");
        break;
    case EditorPhysicsSystem::eSimulationState::STOPPED:
        stateText = QString("Stopped");
        break;
    default:
        break;
    }

    return QString("Physics simulation state: %1").arg(stateText);
}

bool PhysicsWidget::IsEnabled() const
{
    return accessor->GetActiveContext() != nullptr;
}

DAVA_REFLECTION_IMPL(PhysicsWidget)
{
    DAVA::ReflectionRegistrator<PhysicsWidget>::Begin()
    .Method("OnStartPauseClick", &PhysicsWidget::OnStartPauseClick)
    .Method("OnStopClick", &PhysicsWidget::OnStopClick)
    .Field("StartPauseIcon", &PhysicsWidget::GetStartPauseIcon, nullptr)
    .Field("StopIcon", &PhysicsWidget::GetStopIcon, nullptr)
    .Field("labelText", &PhysicsWidget::GetLabelText, nullptr)
    .Field("isEnabled", &PhysicsWidget::IsEnabled, nullptr)
    .End();
}
