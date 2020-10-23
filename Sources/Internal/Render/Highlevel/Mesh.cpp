#include "Render/Highlevel/Mesh.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/3D/PolygonGroup.h"
#include "Render/Highlevel/ShadowVolume.h"
#include "Render/Material/NMaterial.h"

namespace DAVA
{
Mesh::Mesh()
{
    type = TYPE_MESH;
}

Mesh::~Mesh()
{
}

void Mesh::AddPolygonGroup(PolygonGroup* polygonGroup, NMaterial* material)
{
    RenderBatch* batch = new RenderBatch();
    batch->SetPolygonGroup(polygonGroup);
    batch->SetMaterial(material);
    batch->SetStartIndex(0);
    batch->SetIndexCount(polygonGroup->GetPrimitiveCount() * 3);
    AddRenderBatch(batch);

    batch->Release();
    //polygonGroups.push_back(polygonGroup);
}

uint32 Mesh::GetPolygonGroupCount()
{
    return uint32(renderBatchArray.size());
}

PolygonGroup* Mesh::GetPolygonGroup(uint32 index)
{
    return renderBatchArray[index].renderBatch->GetPolygonGroup();
}

RenderObject* Mesh::Clone(RenderObject* newObject)
{
    if (!newObject)
    {
        DVASSERT(IsPointerToExactClass<Mesh>(this), "Can clone only Mesh");
        newObject = new Mesh();
    }

    return RenderObject::Clone(newObject);
}

void Mesh::Save(KeyedArchive* archive, SerializationContext* serializationContext)
{
    RenderObject::Save(archive, serializationContext);
}

void Mesh::Load(KeyedArchive* archive, SerializationContext* serializationContext)
{
    RenderObject::Load(archive, serializationContext);
}

void Mesh::BakeGeometry(const Matrix4& transform)
{
    uint32 size = static_cast<uint32>(renderBatchArray.size());
    for (uint32 i = 0; i < size; ++i)
    {
        PolygonGroup* pg = renderBatchArray[i].renderBatch->GetPolygonGroup();
        DVASSERT(pg);
        pg->ApplyMatrix(transform);
        pg->BuildBuffers();

        renderBatchArray[i].renderBatch->UpdateAABBoxFromSource();
    }

    RecalcBoundingBox();
}
};
