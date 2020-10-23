#include "ConvertToShadowCommand.h"

#include "Qt/Scene/SceneEditor2.h"

ConvertToShadowCommand::ConvertToShadowCommand(DAVA::Entity* entity_, DAVA::RenderBatch* batch)
    : RECommand(CMDID_CONVERT_TO_SHADOW, "Convert To Shadow")
    , entity(SafeRetain(entity_))
    , oldBatch(batch)
    , newBatch(NULL)
{
    DVASSERT(entity);
    DVASSERT(oldBatch);

    renderObject = DAVA::SafeRetain(oldBatch->GetRenderObject());
    DVASSERT(renderObject);

    oldBatch->Retain();

    newBatch = new DAVA::RenderBatch();
    DAVA::PolygonGroup* shadowPg = DAVA::MeshUtils::CreateShadowPolygonGroup(oldBatch->GetPolygonGroup());
    shadowPg->BuildBuffers();
    newBatch->SetPolygonGroup(shadowPg);
    shadowPg->Release();

    DAVA::ScopedPtr<DAVA::NMaterial> shadowMaterialInst(new DAVA::NMaterial());
    shadowMaterialInst->SetMaterialName(DAVA::FastName("Shadow_Material_Instance"));

    newBatch->SetMaterial(shadowMaterialInst);

    SceneEditor2* scene = static_cast<SceneEditor2*>(entity->GetScene());
    DVASSERT(scene != nullptr);
    const DAVA::Set<DAVA::NMaterial*> topLevelMaterials = scene->materialSystem->GetTopParents();
    DAVA::Set<DAVA::NMaterial*>::iterator iter = std::find_if(topLevelMaterials.begin(), topLevelMaterials.end(), [](DAVA::NMaterial* material)
                                                              {
                                                                  DVASSERT(material->HasLocalFXName());
                                                                  return material->GetLocalFXName() == DAVA::NMaterialName::SHADOW_VOLUME;
                                                              });

    if (iter != topLevelMaterials.end())
    {
        shadowMaterialInst->SetParent(*iter);
    }
    else
    {
        DAVA::ScopedPtr<DAVA::NMaterial> shadowMaterial(new DAVA::NMaterial());
        shadowMaterial->SetMaterialName(DAVA::FastName("Shadow_Material"));
        shadowMaterial->SetFXName(DAVA::NMaterialName::SHADOW_VOLUME);

        shadowMaterialInst->SetParent(shadowMaterial.get());
    }
}

ConvertToShadowCommand::~ConvertToShadowCommand()
{
    DAVA::SafeRelease(oldBatch);
    DAVA::SafeRelease(newBatch);
    DAVA::SafeRelease(renderObject);
    DAVA::SafeRelease(entity);
}

void ConvertToShadowCommand::Redo()
{
    renderObject->ReplaceRenderBatch(oldBatch, newBatch);
}

void ConvertToShadowCommand::Undo()
{
    renderObject->ReplaceRenderBatch(newBatch, oldBatch);
}

DAVA::Entity* ConvertToShadowCommand::GetEntity() const
{
    return entity;
}

bool ConvertToShadowCommand::CanConvertBatchToShadow(DAVA::RenderBatch* renderBatch)
{
    if (renderBatch && renderBatch->GetMaterial() && renderBatch->GetPolygonGroup())
    {
        return renderBatch->GetMaterial()->GetEffectiveFXName() != DAVA::NMaterialName::SHADOW_VOLUME;
    }

    return false;
}
