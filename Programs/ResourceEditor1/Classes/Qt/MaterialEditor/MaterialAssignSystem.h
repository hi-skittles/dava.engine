#ifndef __MATERIALS_DROP_SYSTEM_H__
#define __MATERIALS_DROP_SYSTEM_H__

#include "Scene3D/Entity.h"
#include "Render/Material/NMaterial.h"
#include "Render/Highlevel/RenderBatch.h"

class SceneEditor2;
class MaterialAssignSystem
{
public:
    static void AssignMaterial(SceneEditor2* scene, DAVA::NMaterial* instance, DAVA::NMaterial* newMaterialParent);

protected:
    static DAVA::NMaterial* SelectMaterial(const DAVA::Set<DAVA::NMaterial*>& materials);
};

#endif // __MATERIALS_DROP_SYSTEM_H__
