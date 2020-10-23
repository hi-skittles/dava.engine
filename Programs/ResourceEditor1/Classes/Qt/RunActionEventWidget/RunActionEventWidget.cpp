#include "RunActionEventWidget.h"
#include "ui_RunActionEventWidget.h"

#include "DAVAEngine.h"
#include "Scene3D/Components/ActionComponent.h"

#include "Classes/Qt/Main/mainwindow.h"
#include "Scene/SceneEditor2.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/Selection/Selection.h"
#include "Classes/Selection/SelectionData.h"

#include "TArc/Core/FieldBinder.h"

#include <QStringList>
#include <QSet>
#include <QCompleter>
#include "Application/RESettings.h"

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

    selectionFieldBinder.reset(new DAVA::TArc::FieldBinder(REGlobal::GetAccessor()));
    {
        DAVA::TArc::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, DAVA::MakeFunction(this, &RunActionEventWidget::OnSelectionChanged));
    }

    connect(SceneSignals::Instance(), SIGNAL(Activated(SceneEditor2*)), this, SLOT(sceneActivated(SceneEditor2*)));
    connect(SceneSignals::Instance(), SIGNAL(Deactivated(SceneEditor2*)), this, SLOT(sceneDeactivated(SceneEditor2*)));

    const DAVA::ActionComponent::Action::eEvent eventType = REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->runActionEventType;
    ui->eventType->setCurrentIndex(editorIdMap[eventType]);
}

RunActionEventWidget::~RunActionEventWidget() = default;

void RunActionEventWidget::OnTypeChanged()
{
    const DAVA::uint32 eventTypeId = ui->eventType->itemData(ui->eventType->currentIndex()).toUInt();
    const int editorindex = editorIdMap[eventTypeId];
    DVASSERT(editorindex < ui->stackedWidget->count());

    ui->stackedWidget->setCurrentIndex(editorindex);
    REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->runActionEventType = static_cast<DAVA::ActionComponent::Action::eEvent>(eventTypeId);
}

void RunActionEventWidget::OnInvoke()
{
    const uint eventTypeId = ui->eventType->itemData(ui->eventType->currentIndex()).toUInt();
    if (scene == nullptr)
        return;

    const DAVA::uint32 switchIndex = ui->switchIndex->value();
    const DAVA::FastName name(ui->name->currentText().toStdString().c_str());

    const SelectableGroup& selection = Selection::GetSelection();
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

void RunActionEventWidget::sceneActivated(SceneEditor2* scene_)
{
    scene = scene_;
}

void RunActionEventWidget::sceneDeactivated(SceneEditor2* scene)
{
    scene = nullptr;
}

void RunActionEventWidget::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<SelectableGroup>() == false)
    {
        autocompleteModel->setStringList(QStringList());
        return;
    }

    const SelectableGroup& selection = selectionAny.Get<SelectableGroup>();

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
