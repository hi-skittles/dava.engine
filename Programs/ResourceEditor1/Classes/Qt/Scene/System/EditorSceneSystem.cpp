#include "Classes/Qt/Scene/System/EditorSceneSystem.h"
#include "Classes/Qt/Scene/SceneEditor2.h"

#include <Scene3D/Scene.h>

void EditorSceneSystem::EnableSystem()
{
    systemIsEnabled = true;
}

void EditorSceneSystem::DisableSystem()
{
    systemIsEnabled = false;
}

bool EditorSceneSystem::IsSystemEnabled() const
{
    return systemIsEnabled;
}

bool EditorSceneSystem::AcquireInputLock(DAVA::Scene* scene)
{
    return static_cast<SceneEditor2*>(scene)->AcquireInputLock(this);
}

void EditorSceneSystem::ReleaseInputLock(DAVA::Scene* scene)
{
    static_cast<SceneEditor2*>(scene)->ReleaseInputLock(this);
}
