#pragma once

#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyEditor.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataMetaObject.h"

#include <REPlatform/Commands/RECommandNotificationObject.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <Scene3D/Entity.h>

#include <QDialog>
#include <QDialogButtonBox>

namespace Ui
{
class BaseAddEntityDialog;
}

class BaseAddEntityDialog : public QDialog
{
    Q_OBJECT

public:
    enum eButtonAlign
    {
        BUTTON_ALIGN_LEFT = 0,
        BUTTON_ALIGN_RIGHT
    };

    explicit BaseAddEntityDialog(QWidget* parent = 0, QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Close);
    virtual ~BaseAddEntityDialog();

    void GetIncludedControls(QList<QWidget*>& includedWidgets);

    virtual DAVA::Entity* GetEntity();
    void virtual SetEntity(DAVA::Entity*);

    void AddButton(QWidget* widget, eButtonAlign orientation = BUTTON_ALIGN_LEFT);
    void AddButton(QWidget* widget, DAVA::int32 position);

protected slots:
    virtual void OnItemEdited(const QModelIndex&);
    virtual void CommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification);

protected:
    virtual QtPropertyData* AddInspMemberToEditor(void* object, const DAVA::InspMember*);
    virtual QtPropertyData* AddKeyedArchiveMember(DAVA::KeyedArchive* archive, const DAVA::String& key, const DAVA::String& rowName);
    virtual QtPropertyData* AddMetaObject(void* object, const DAVA::MetaInfo* meta, const DAVA::String& rowName);

    void AddControlToUserContainer(QWidget* widget);
    void AddControlToUserContainer(QWidget* widget, const DAVA::String& labelString);
    void RemoveControlFromUserContainer(QWidget* widget);
    void RemoveAllControlsFromUserContainer();

    void showEvent(QShowEvent* event);

    void PerformResize();

    DAVA::Entity* entity;
    QtPropertyEditor* propEditor;
    Ui::BaseAddEntityDialog* ui;

    DAVA::Map<QWidget*, QWidget*> additionalWidgetMap;
};
