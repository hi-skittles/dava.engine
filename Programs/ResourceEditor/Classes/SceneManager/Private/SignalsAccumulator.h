#pragma once

#include <REPlatform/DataNodes/SelectableGroup.h>

#include <Functional/TrackedObject.h>
#include <Base/BaseTypes.h>

namespace DAVA
{
class SceneEditor2;
class RECommandNotificationObject;
} // namespace DAVA

class SignalsAccumulator final : public DAVA::TrackedObject
{
public:
    SignalsAccumulator(DAVA::SceneEditor2* scene);

private:
    void OnCommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification);
    void OnStructureChanged(DAVA::SceneEditor2* scene);

    void OnMouseOverSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup* objects);

    void OnDropperHeightChanged(DAVA::SceneEditor2* scene, DAVA::float32 height);
    void OnRulerToolLengthChanged(DAVA::SceneEditor2* scene, DAVA::float64 length, DAVA::float64 previewLength);
    void OnLandscapeEditorToggled(DAVA::SceneEditor2* scene);
};