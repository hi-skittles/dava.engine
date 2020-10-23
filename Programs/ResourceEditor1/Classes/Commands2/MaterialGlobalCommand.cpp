#include "MaterialGlobalCommand.h"
#include "Scene3D/Scene.h"
#include "Commands2/RECommandIDs.h"

MaterialGlobalSetCommand::MaterialGlobalSetCommand(DAVA::Scene* _scene, DAVA::NMaterial* global)
    : RECommand(CMDID_MATERIAL_GLOBAL_SET, "Set global material")
    , scene(_scene)
{
    DVASSERT(nullptr != scene);

    newGlobal = SafeRetain(global);
    oldGlobal = SafeRetain(scene->GetGlobalMaterial());
}

MaterialGlobalSetCommand::~MaterialGlobalSetCommand()
{
    SafeRelease(oldGlobal);
    SafeRelease(newGlobal);
}

void MaterialGlobalSetCommand::Redo()
{
    scene->SetGlobalMaterial(newGlobal);
}

void MaterialGlobalSetCommand::Undo()
{
    scene->SetGlobalMaterial(oldGlobal);
}

DAVA::Entity* MaterialGlobalSetCommand::GetEntity() const
{
    return scene;
}
