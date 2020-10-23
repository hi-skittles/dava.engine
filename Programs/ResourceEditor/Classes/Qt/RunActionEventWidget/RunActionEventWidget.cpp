#include "RunActionEventWidget.h"
#include "ui_RunActionEventWidget.h"

#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <TArc/Core/Deprecated.h>
#include <TArc/Core/FieldBinder.h>

#include "Classes/Qt/Scene/SceneSignals.h"

#include <QCompleter>
#include <QSet>
#include <QStringList>

namespace
{
const DAVA::String settingsType("Internal/RunActionEventWidget/CurrentType");
}

RunActionEventWidget::RunActionEventWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::RunActionEventWidget())
    , scene(NULL)
{
    ui->setupUi(this);
    setObjectName("RunActionEventWidget");

    ui->eventType->addItem("Switch", DAVA::ActionComponent::Action::EVENT_SWITCH_CHANGED);
    ui->eventType->addItem("Added", DAVA::ActionComponent::Action::EVENT_ADDED_TO_SCENE);
    ui->eventType->addItem("User", DAVA::ActionComponent::Action::EVENT_CUSTOM);

    editorIdMap[DAVA::ActionComponent::Action::EVENT_SWITCH_CHANGED] = 0;
    editorIdMap[DAVA::ActionComponent::Action::EVENT_ADDED_TO_SCENE] = 1;
    editorIdMap[DAVA::ActionComponent::Action::EVENT_CUSTOM] = 2;

    autocompleteModel = new QStringListModel(this);
    ui->name->setModel(autocompleteModel);
    if (ui->name->completer())
    {
        ui->name->completer()->setCompletionMode(QCompleter::PopupCompletion);
    }

    connect(ui->eventType, SIGNAL(currentIndexChanged(int)), SLOT(OnTypeChanged()));
    connect(ui->run, SIGNAL(clicked()), SLOT(OnInvoke()));

    selectionFieldBinder.reset(new DAVA::FieldBinder(DAVA::Deprecated::GetAccessor()));
    {
        DAVA::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<DAVA::SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(DAVA::SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, DAVA::MakeFunction(this, &RunActionEventWidget::OnSelectionChanged));
    }

    connect(SceneSignals::Instance(), &SceneSignals::Activated, this, &RunActionEventWidget::sceneActivated);
    connect(SceneSignals::Instance(), &SceneSignals::Deactivated, this, &RunActionEventWidget::sceneDeactivated);

    const DAVA::ActionComponent::Action::eEvent eventType = DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->runActionEventType;
    ui->eventType->setCurrentIndex(editorIdMap[eventType]);
}

RunActionEventWidget::~RunActionEventWidget() = default;

void RunActionEventWidget::OnTypeChanged()
{
    const DAVA::uint32 eventTypeId = ui->eventType->itemData(ui->eventType->currentIndex()).toUInt();
    const int editorindex = editorIdMap[eventTypeId];
    DVASSERT(editorindex < ui->stackedWidget->count());

    ui->stackedWidget->setCurrentIndex(editorindex);
    DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->runActionEventType = static_cast<DAVA::ActionComponent::Action::eEvent>(eventTypeId);
}

void RunActionEventWidget::OnInvoke()
{
    const uint eventTypeId = ui->eventType->itemData(ui->eventType->currentIndex()).toUInt();
    if (scene == nullptr)
        return;

    const DAVA::uint32 switchIndex = ui->switchIndex->value();
    const DAVA::FastName name(ui->name->currentText().toStdString().c_str());

    DAVA::SelectionData* selectionData = DAVA::Deprecated::GetActiveContext()->GetData<DAVA::SelectionData>();
    if (selectionData == nullptr)
    {
        return;
    }

    const DAVA::SelectableGroup& selection = selectionData->GetSelection();
    for (auto entity : selection.ObjectsOfType<DAVA::Entity>())
    {
        DAVA::ActionComponent* component = entity->GetComponent<DAVA::ActionComponent>();
        if (component == nullptr)
            continue;

        const DAVA::uint32 nEvents = component->GetCount();
        for (DAVA::uint32 componentIdx = 0; componentIdx < nEvents; componentIdx++)
        {
            DAVA::ActionComponent::Action& act = component->Get(componentIdx);
            if (act.eventType == eventTypeId)
            {
                switch (eventTypeId)
                {
                case DAVA::ActionComponent::Action::EVENT_SWITCH_CHANGED:
                    if (act.switchIndex == switchIndex)
                    {
                        component->StartSwitch(switchIndex);
                    }
                    break;
                case DAVA::ActionComponent::Action::EVENT_ADDED_TO_SCENE:
                    component->StartAdd();
                    break;
                case DAVA::ActionComponent::Action::EVENT_CUSTOM:
                    if (act.userEventId == name)
                    {
                        component->StartUser(name);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void RunActionEventWidget::sceneActivated(DAVA::SceneEditor2* scene_)
{
    scene = scene_;
}

void RunActionEventWidget::sceneDeactivated(DAVA::SceneEditor2* scene_)
{
    scene = nullptr;
}

void RunActionEventWidget::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<DAVA::SelectableGroup>() == false)
    {
        autocompleteModel->setStringList(QStringList());
        return;
    }

    const DAVA::SelectableGroup& selection = selectionAny.Get<DAVA::SelectableGroup>();

    QSet<QString> nameSet;
    for (auto entity : selection.ObjectsOfType<DAVA::Entity>())
    {
        DAVA::ActionComponent* component = entity->GetComponent<DAVA::ActionComponent>();
        if (component == nullptr)
            continue;

        const DAVA::uint32 nEvents = component->GetCount();
        for (DAVA::uint32 componentIdx = 0; componentIdx < nEvents; componentIdx++)
        {
            DAVA::ActionComponent::Action& act = component->Get(componentIdx);
            if (act.eventType == DAVA::ActionComponent::Action::EVENT_CUSTOM)
            {
                nameSet.insert(QString(act.userEventId.c_str()));
            }
        }
    }

    autocompleteModel->setStringList(nameSet.toList());
    setEnabled(!selection.IsEmpty());
}
