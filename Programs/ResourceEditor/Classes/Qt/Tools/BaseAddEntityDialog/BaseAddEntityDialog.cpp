#include "BaseAddEntityDialog.h"
#include "ui_BaseAddEntityDialog.h"

#include "Classes/Qt/Scene/SceneSignals.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspMember.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataIntrospection.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataKeyedArchiveMember.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataMetaObject.h"

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Utils/Utils.h>

#include <TArc/Core/Deprecated.h>

#include <QSizePolicy>
#include <QLabel>
#include <QScrollBar>

BaseAddEntityDialog::BaseAddEntityDialog(QWidget* parent, QDialogButtonBox::StandardButtons buttons)
    : QDialog(parent)
    , entity(NULL)
    , ui(new Ui::BaseAddEntityDialog)
{
    ui->setupUi(this);
    setAcceptDrops(false);
    setWindowModality(Qt::NonModal);
    setWindowFlags(DAVA::WINDOWFLAG_ON_TOP_OF_APPLICATION | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_MacAlwaysShowToolWindow); // on top of all applications

    propEditor = ui->propertyEditor;
    propEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    propEditor->setTabKeyNavigation(false);
    propEditor->setAlternatingRowColors(true);
    propEditor->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    propEditor->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    propEditor->setIndentation(16);
    propEditor->SetEditTracking(true);
    connect(propEditor, SIGNAL(PropertyEdited(const QModelIndex&)), this, SLOT(OnItemEdited(const QModelIndex&)));

    ui->buttonBox->setStandardButtons(buttons);

    connect(SceneSignals::Instance(), &SceneSignals::CommandExecuted, this, &BaseAddEntityDialog::CommandExecuted);
}

BaseAddEntityDialog::~BaseAddEntityDialog()
{
    propEditor->RemovePropertyAll();
    SafeRelease(entity);

    for (DAVA::Map<QWidget*, QWidget*>::iterator it = additionalWidgetMap.begin(); it != additionalWidgetMap.end(); ++it)
    {
        delete it->second;
    }
    additionalWidgetMap.clear();

    delete ui;
}

void BaseAddEntityDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    propEditor->expandAll();
    PerformResize();
}

void BaseAddEntityDialog::PerformResize()
{
    QList<int> sizes;
    sizes.push_back(geometry().height() - ui->scrollAreaWidgetContents_2->geometry().height() - ui->buttonBox->height() - layout()->spacing() * 4);
    sizes.push_back(ui->scrollAreaWidgetContents_2->geometry().height());
    ui->splitter->setSizes(sizes);
}

QtPropertyData* BaseAddEntityDialog::AddInspMemberToEditor(void* object, const DAVA::InspMember* member)
{
    QtPropertyData* propData = QtPropertyDataIntrospection::CreateMemberData(member->Name(), object, member);
    propEditor->AppendProperty(std::unique_ptr<QtPropertyData>(propData));
    return propData;
}

QtPropertyData* BaseAddEntityDialog::AddKeyedArchiveMember(DAVA::KeyedArchive* archive_, const DAVA::String& key_, const DAVA::String& rowName)
{
    QtPropertyData* propData = new QtPropertyKeyedArchiveMember(DAVA::FastName(rowName), archive_, key_);
    propEditor->AppendProperty(std::unique_ptr<QtPropertyData>(propData));
    return propData;
}

QtPropertyData* BaseAddEntityDialog::AddMetaObject(void* object_, const DAVA::MetaInfo* meta_, const DAVA::String& rowName)
{
    QtPropertyData* propData = new QtPropertyDataMetaObject(DAVA::FastName(rowName), object_, meta_);
    propEditor->AppendProperty(std::unique_ptr<QtPropertyData>(propData));
    return propData;
}

void BaseAddEntityDialog::SetEntity(DAVA::Entity* entity_)
{
    SafeRelease(entity);
    entity = SafeRetain(entity_);
}

void BaseAddEntityDialog::AddButton(QWidget* widget, eButtonAlign orientation)
{
    switch (orientation)
    {
    case BUTTON_ALIGN_LEFT:
        ui->lowerLayOut->insertWidget(0, widget);
        break;
    case BUTTON_ALIGN_RIGHT:
        ui->lowerLayOut->addWidget(widget);
        break;
    default:
        break;
    }
}

void BaseAddEntityDialog::AddButton(QWidget* widget, DAVA::int32 position)
{
    ui->lowerLayOut->insertWidget(position, widget);
}

DAVA::Entity* BaseAddEntityDialog::GetEntity()
{
    return entity;
}

void BaseAddEntityDialog::AddControlToUserContainer(QWidget* widget)
{
    ui->userContentLayout->addWidget(widget);
}

void BaseAddEntityDialog::AddControlToUserContainer(QWidget* widget, const DAVA::String& labelString)
{
    QLabel* label = new QLabel(labelString.c_str(), this);
    int rowCount = ui->userContentLayout->rowCount();
    ui->userContentLayout->addWidget(label, rowCount, 0);
    ui->userContentLayout->addWidget(widget, rowCount, 1);
    additionalWidgetMap[widget] = label;
}

void BaseAddEntityDialog::RemoveControlFromUserContainer(QWidget* widget)
{
    ui->userContentLayout->removeWidget(widget);
    if (additionalWidgetMap.find(widget) != additionalWidgetMap.end())
    {
        QWidget* additionalWidget = additionalWidgetMap[widget];
        additionalWidgetMap.erase(additionalWidgetMap.find(widget));
        delete additionalWidget;
    }
}

void BaseAddEntityDialog::RemoveAllControlsFromUserContainer()
{
    QLayout* container = ui->userContentLayout;
    while (QLayoutItem* item = container->takeAt(0))
    {
        if (QWidget* widget = item->widget())
        {
            container->removeWidget(widget);
            if (additionalWidgetMap.find(widget) != additionalWidgetMap.end())
            {
                QWidget* additionalWidget = additionalWidgetMap[widget];
                additionalWidgetMap.erase(additionalWidgetMap.find(widget));
                delete additionalWidget;
            }
        }
    }
}

void BaseAddEntityDialog::GetIncludedControls(QList<QWidget*>& includedWidgets)
{
    QLayout* container = ui->userContentLayout;
    for (int i = 0; i < container->count(); ++i)
    {
        QWidget* child = container->itemAt(i)->widget();
        if (child)
        {
            includedWidgets.append(child);
        }
    }
}

void BaseAddEntityDialog::OnItemEdited(const QModelIndex& index)
{
    DAVA::SceneData* sceneData = DAVA::Deprecated::GetActiveDataNode<DAVA::SceneData>();
    if (sceneData == nullptr || sceneData->GetScene().Get() == nullptr)
    {
        return;
    }

    DAVA::SceneEditor2* curScene = sceneData->GetScene().Get();

    QtPropertyData* data = propEditor->GetProperty(index);
    std::unique_ptr<DAVA::Command> command = data->CreateLastCommand();
    curScene->Exec(std::move(command));
}

void BaseAddEntityDialog::CommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
{
    if (propEditor)
    {
        propEditor->Update();
    }
}
