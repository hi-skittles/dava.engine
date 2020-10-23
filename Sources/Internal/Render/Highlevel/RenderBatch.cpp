#include "Render/Highlevel/RenderBatch.h"
#include "Render/3D/PolygonGroup.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Highlevel/RenderLayer.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Render/Highlevel/SpeedTreeObject.h"
#include "Scene3D/SceneFileV2.h"
#include "Debug/DVAssert.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(RenderBatch)
{
    ReflectionRegistrator<RenderBatch>::Begin()
    .Field("dataSource", &RenderBatch::dataSource)[M::DisplayName("Data source")]
    .Field("startIndex", &RenderBatch::startIndex)[M::DisplayName("Start index"), M::ReadOnly(), M::HiddenField()]
    .Field("indexCount", &RenderBatch::indexCount)[M::DisplayName("Index count"), M::ReadOnly(), M::HiddenField()]
    .Field("aabbbox", &RenderBatch::aabbox)[M::DisplayName("Bounding box"), M::DeveloperModeOnly()]
    .Field("material", &RenderBatch::material)[M::DisplayName("Material")]
    .Field("sortingKey", &RenderBatch::GetSortingKey, &RenderBatch::SetSortingKey)[M::DisplayName("Sorting key"), M::DeveloperModeOnly()]
    .End();
}

RenderBatch::RenderBatch()
{
#if defined(__DAVA_USE_OCCLUSION_QUERY__)
    occlusionQuery = new OcclusionQuery();
    queryRequested = -1;
    queryRequestFrame = 0;
    lastFraemDrawn = -10;
#endif
}

RenderBatch::~RenderBatch()
{
#if defined(__DAVA_USE_OCCLUSION_QUERY__)
    SafeDelete(occlusionQuery);
#endif
    SafeRelease(dataSource);

    SafeRelease(material);
}

void RenderBatch::SetPolygonGroup(PolygonGroup* _polygonGroup)
{
    SafeRelease(dataSource);
    dataSource = SafeRetain(_polygonGroup);
    UpdateAABBoxFromSource();
}

void RenderBatch::SetMaterial(NMaterial* _material)
{
    NMaterial* oldMat = material;
    material = SafeRetain(_material);
    SafeRelease(oldMat);
}

void RenderBatch::SetRenderObject(RenderObject* _renderObject)
{
    renderObject = _renderObject;
}

const AABBox3& RenderBatch::GetBoundingBox() const
{
    return aabbox;
}

void RenderBatch::SetSortingKey(uint32 _key)
{
    DVASSERT(_key < 16);
    sortingKey = (sortingKey & ~SORTING_KEY_MASK) + _key;
}

void RenderBatch::SetSortingOffset(uint32 offset)
{
    DVASSERT(offset < 32);
    sortingKey = (sortingKey & ~SORTING_OFFSET_MASK) + (offset << SORTING_OFFSET_SHIFT);
}

void RenderBatch::GetDataNodes(Set<DataNode*>& dataNodes)
{
    NMaterial* curNode = material;
    while (curNode != NULL)
    {
        dataNodes.insert(curNode);
        curNode = curNode->GetParent();
    }

    if (dataSource)
    {
        InsertDataNode(dataSource, dataNodes);
    }
}

void RenderBatch::InsertDataNode(DataNode* node, Set<DataNode*>& dataNodes)
{
    dataNodes.insert(node);

    /*for(int32 i = 0; i < node->GetChildrenNodeCount(); ++i)
	{
		InsertDataNode(node->GetChildNode(i), dataNodes);
	}*/
}

RenderBatch* RenderBatch::Clone(RenderBatch* destination)
{
    RenderBatch* rb = destination;
    if (!rb)
        rb = new RenderBatch();

    SafeRelease(rb->dataSource);
    rb->dataSource = SafeRetain(dataSource);

    if (material)
    {
        NMaterial* mat = material->Clone();
        rb->SetMaterial(mat);
        mat->Release();
    }

    rb->vertexBuffer = vertexBuffer;
    rb->vertexCount = vertexCount;
    rb->vertexBase = vertexBase;

    rb->indexBuffer = indexBuffer;
    rb->startIndex = startIndex;
    rb->indexCount = indexCount;

    rb->primitiveType = primitiveType;
    rb->vertexLayoutId = vertexLayoutId;

    rb->startIndex = startIndex;
    rb->indexCount = indexCount;

    rb->aabbox = aabbox;
    rb->sortingKey = sortingKey;

    return rb;
}

void RenderBatch::Save(KeyedArchive* archive, SerializationContext* serializationContext)
{
    BaseObject::SaveObject(archive);

    if (NULL != archive)
    {
        uint64 dataSourceID = DataNode::INVALID_ID;
        if (nullptr != dataSource)
        {
            dataSourceID = dataSource->GetNodeID();
        }

        archive->SetVariant("rb.aabbox", VariantType(aabbox));
        archive->SetUInt32("rb.sortingKey", sortingKey);
        archive->SetVariant("rb.datasource", VariantType(dataSourceID));

        if (material)
        {
            uint64 matKey = material->GetNodeID();
            archive->SetUInt64("rb.nmatname", matKey);
        }
    }
}

void RenderBatch::Load(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (NULL != archive)
    {
        sortingKey = archive->GetUInt32("rb.sortingKey", SORTING_KEY_DEF_VALUE);

        aabbox = archive->GetVariant("rb.aabbox")->AsAABBox3(); //this is historical "shield" as polygon group data loads after structure

        PolygonGroup* pg = static_cast<PolygonGroup*>(serializationContext->GetDataBlock(archive->GetVariant("rb.datasource")->AsUInt64()));
        if (pg != dataSource)
        {
            SafeRelease(dataSource);
            dataSource = SafeRetain(pg);
        }

        int64 matKey = archive->GetUInt64("rb.nmatname");
        NMaterial* mat = static_cast<NMaterial*>(serializationContext->GetDataBlock(matKey));
        SetMaterial(mat);
        if (material)
            material->PreBuildMaterial(PASS_FORWARD);
    }

    BaseObject::LoadObject(archive);
}

void RenderBatch::UpdateAABBoxFromSource()
{
    if (NULL != dataSource)
    {
        aabbox = dataSource->GetBoundingBox();
        DVASSERT(aabbox.min.x != AABBOX_INFINITY &&
                 aabbox.min.y != AABBOX_INFINITY &&
                 aabbox.min.z != AABBOX_INFINITY);
    }
}
};
