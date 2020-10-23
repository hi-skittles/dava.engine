#ifndef __SOUND_EDITOR_H__
#define __SOUND_EDITOR_H__

#include <REPlatform/Scene/SceneEditor2.h>

#include <QDialog>
#include <QListWidgetItem>

#include <Base/BaseTypes.h>
#include "Sound/SoundEvent.h"

namespace DAVA
{
class Entity;
class SoundComponent;
} // namespace DAVA

namespace Ui
{
class SoundComponentEditor;
}

class SoundComponentEditor : public QDialog
{
    Q_OBJECT

public:
    explicit SoundComponentEditor(DAVA::SceneEditor2* scene, QWidget* parent = 0);
    virtual ~SoundComponentEditor();

    void SetEditableEntity(DAVA::Entity* entity);

private slots:
    void OnEventSelected(QListWidgetItem* item);

    void OnSliderMoved(int value);
    void OnSliderPressed();

    void OnPlay();
    void OnStop();
    void OnAddEvent();
    void OnRemoveEvent();
    void OnAutoTrigger(bool checked);

private:
    struct ParamSliderData : public QObjectUserData
    {
        DAVA::String paramName;
        DAVA::float32 minValue;
        DAVA::float32 maxValue;
    };

    void FillEventsList();

    void AddSliderWidget(const DAVA::SoundEvent::SoundEventParameterInfo& param, DAVA::float32 currentParamValue);
    void ClearParamsFrame();
    void FillEventParamsFrame();

    DAVA::Entity* entity;
    DAVA::SoundComponent* component;
    DAVA::int32 selectedEventIndex;

    DAVA::SceneEditor2* scene;

    Ui::SoundComponentEditor* ui;
};

#endif // __SOUND_EDITOR_H__
