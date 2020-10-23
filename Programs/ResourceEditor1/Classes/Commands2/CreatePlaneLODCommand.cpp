#include "Classes/Commands2/CreatePlaneLODCommand.h"
#include <Classes/Commands2/RECommandIDs.h>
#include "Classes/Qt/Scene/SceneHelper.h"
#include "Classes/Utils/TextureDescriptor/RETextureDescriptorUtils.h"

#include <Render/Material/NMaterialNames.h>
#include <Scene3D/Lod/LodComponent.h>

CreatePlaneLODCommand::CreatePlaneLODCommand(const CreatePlaneLODCommandHelper::RequestPointer& request_)
    : RECommand(CMDID_LOD_CREATE_PLANE, "Create Plane LOD")
    , request(request_)
{
    DVASSERT(GetRenderObject(GetEntity()));
}

void CreatePlaneLODCommand::Redo()
{
    CreateTextureFiles();

    DAVA::ScopedPtr<DAVA::Texture> fileTexture(DAVA::Texture::CreateFromFile(request->texturePath));
    DAVA::NMaterial* material = request->planeBatch->GetMaterial();
    if (material != nullptr)
    {
        if (material->HasLocalTexture(DAVA::NMaterialTextureName::TEXTURE_ALBEDO))
        {
            material->SetTexture(DAVA::NMaterialTextureName::TEXTURE_ALBEDO, fileTexture);
        }
        else
        {
            material->AddTexture(DAVA::NMaterialTextureName::TEXTURE_ALBEDO, fileTexture);
        }
        fileTexture->Reload();
    }

    DAVA::Entity* entity = GetEntity();
    DAVA::RenderObject* renderObject = DAVA::GetRenderObject(entity);
    renderObject->AddRenderBatch(request->planeBatch, request->newLodIndex, -1);
}

void CreatePlaneLODCommand::Undo()
{
    DAVA::RenderObject* ro = DAVA::GetRenderObject(GetEntity());

    //restore batches
    ro->RemoveRenderBatch(request->planeBatch);

    DeleteTextureFiles();
}

DAVA::Entity* CreatePlaneLODCommand::GetEntity() const
{
    return request->lodComponent->GetEntity();
}

void CreatePlaneLODCommand::CreateTextureFiles()
{
    DVASSERT(request->planeImage);

    DAVA::FilePath folder = request->texturePath.GetDirectory();
    DAVA::FileSystem::Instance()->CreateDirectory(folder, true);
    DAVA::ImageSystem::Save(request->texturePath, request->planeImage);
    RETextureDescriptorUtils::CreateOrUpdateDescriptor(request->texturePath);
}

void CreatePlaneLODCommand::DeleteTextureFiles()
{
    DAVA::FileSystem::Instance()->DeleteFile(request->texturePath);
    DAVA::FileSystem::Instance()->DeleteFile(DAVA::TextureDescriptor::GetDescriptorPathname(request->texturePath));
}

DAVA::RenderBatch* CreatePlaneLODCommand::GetRenderBatch() const
{
    return request->planeBatch;
}
